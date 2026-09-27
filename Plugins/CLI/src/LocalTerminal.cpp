#include <CLI/include/LocalTerminal.hpp>

#include <iostream>
#include <mutex>
#include <string>
#include <vector>

#include <Lattice/Tools/TextFormatter.hpp>

#if defined(_WIN32)
#include <conio.h>
#include <io.h>
#include <windows.h>
#else
#include <poll.h>
#include <termios.h>
#include <unistd.h>
#endif

namespace CLIPlugin {
namespace {

std::string styled(std::string_view text, Lattice::TextStyle style) {
    Lattice::TextFormatter formatted;
    formatted.append(text, style);
    return formatted.render();
}

size_t previousCharacter(std::string_view text, size_t position) {
    if (position == 0)
        return 0;
    --position;
    while (position != 0 && (static_cast<unsigned char>(text[position]) & 0xc0) == 0x80)
        --position;
    return position;
}

size_t nextCharacter(std::string_view text, size_t position) {
    if (position >= text.size())
        return text.size();
    ++position;
    while (position < text.size() && (static_cast<unsigned char>(text[position]) & 0xc0) == 0x80)
        ++position;
    return position;
}

size_t characterCount(std::string_view text) {
    size_t count = 0;
    for (const unsigned char byte : text)
        if ((byte & 0xc0) != 0x80)
            ++count;
    return count;
}

}

struct LocalTerminal::Impl {
    static constexpr int NoInput = -1;
    static constexpr int EndOfInput = -2;

    LocalTerminal& owner;

    explicit Impl(LocalTerminal& owner)
        : owner(owner) {}

    void start() {
        std::lock_guard lock(mutex);
        if (started)
            return;

    #if defined(_WIN32)
        interactive = _isatty(_fileno(stdin)) && _isatty(_fileno(stdout));
        if (interactive) {
            inputHandle = GetStdHandle(STD_INPUT_HANDLE);
            outputHandle = GetStdHandle(STD_OUTPUT_HANDLE);
            inputConfigured = GetConsoleMode(inputHandle, &oldInputMode) != 0;
            outputConfigured = GetConsoleMode(outputHandle, &oldOutputMode) != 0;
            if (inputConfigured)
                SetConsoleMode(inputHandle, oldInputMode & ~(ENABLE_LINE_INPUT | ENABLE_ECHO_INPUT | ENABLE_PROCESSED_INPUT));
            if (outputConfigured)
                SetConsoleMode(outputHandle, oldOutputMode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
        }
    #else
        interactive = isatty(STDIN_FILENO) && isatty(STDOUT_FILENO);
        if (interactive && tcgetattr(STDIN_FILENO, &oldTermios) == 0) {
            termios raw = oldTermios;
            raw.c_lflag &= static_cast<tcflag_t>(~(ICANON | ECHO | IEXTEN | ISIG));
            raw.c_iflag &= static_cast<tcflag_t>(~(IXON | ICRNL));
            raw.c_cc[VMIN] = 0;
            raw.c_cc[VTIME] = 0;
            terminalConfigured = tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == 0;
        }
    #endif
    
        started = true;
        if (interactive)
            redrawUnlocked();
    }

    void finish() {
        std::lock_guard lock(mutex);
        if (!started)
            return;

        if (interactive) {
            clearPromptUnlocked();
            std::cout << "\033[0 q" << std::flush;
        }

        #if defined(_WIN32)
            if (inputConfigured)
                SetConsoleMode(inputHandle, oldInputMode);
            if (outputConfigured)
                SetConsoleMode(outputHandle, oldOutputMode);
            inputConfigured = false;
            outputConfigured = false;
        #else
            if (terminalConfigured)
                tcsetattr(STDIN_FILENO, TCSAFLUSH, &oldTermios);
            terminalConfigured = false;
        #endif

        interactive = false;
        started = false;
    }

    void write(std::string_view text) {
        std::lock_guard lock(mutex);
        if (started && interactive)
            clearPromptUnlocked();
        std::cout << text;
        if (started && interactive)
            redrawUnlocked();
        else
            std::cout << std::flush;
    }

    Terminal::Input pollInput() {
        const int byte = readByte(0);
        if (byte < 0)
            return {.closed = byte == EndOfInput};

        std::lock_guard lock(mutex);
        if (byte == 3 || byte == 4)
            return {.closed = true};

        if (byte == '\r' || byte == '\n') {
            std::string line = std::move(input);
            if (interactive) {
                const auto& style = owner.style();
                std::cout
                    << "\r\033[2K"
                    << styled(style.prompt, style.promptColor) << ' '
                    << line
                    << '\n'
                    << std::flush;
                promptVisible = false;
                if (!line.empty()) {
                    if (history.empty() || history.back() != line)
                        history.push_back(line);
                    historyIndex = history.size();
                }
            }
            input.clear();
            cursor = 0;
            redrawUnlocked();
            return {.line = std::move(line)};
        }

        if (byte == 8 || byte == 127) {
            if (cursor != 0) {
                const size_t previous = previousCharacter(input, cursor);
                input.erase(previous, cursor - previous);
                cursor = previous;
            }
            redrawUnlocked();
            return {};
        }

        if (byte == 27) {
            handleEscapeUnlocked();
            redrawUnlocked();
            return {};
        }

        if (byte >= 32) {
            input.insert(cursor, 1, static_cast<char>(byte));
            ++cursor;
            redrawUnlocked();
        }
        return {};
    }

    int readByte(int timeoutMs) {
    #if defined(_WIN32)
        if (timeoutMs == 0 && !_kbhit())
            return NoInput;
        const auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(timeoutMs);
        while (!_kbhit()) {
            if (std::chrono::steady_clock::now() >= deadline)
                return NoInput;
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        return _getch();
    #else
        pollfd descriptor{.fd = STDIN_FILENO, .events = POLLIN, .revents = 0};
        const int result = ::poll(&descriptor, 1, timeoutMs);
        if (result == 0)
            return NoInput;
        if (result < 0 || (descriptor.revents & (POLLERR | POLLNVAL)))
            return NoInput;
        if (descriptor.revents & POLLIN) {
            unsigned char byte = 0;
            const ssize_t count = ::read(STDIN_FILENO, &byte, 1);
            return count == 1 ? byte : (count == 0 ? EndOfInput : NoInput);
        }
        return descriptor.revents & POLLHUP ? EndOfInput : NoInput;
    #endif
    }

    void handleEscapeUnlocked() {
        if (readByte(10) != '[')
            return;
        switch (readByte(10)) {
        case 'A':
            if (!history.empty() && historyIndex != 0) {
                input = history[--historyIndex];
                cursor = input.size();
            }
            break;
        case 'B':
            if (historyIndex < history.size()) {
                ++historyIndex;
                input = historyIndex == history.size() ? std::string{} : history[historyIndex];
                cursor = input.size();
            }
            break;
        case 'C': cursor = nextCharacter(input, cursor); break;
        case 'D': cursor = previousCharacter(input, cursor); break;
        case 'H': cursor = 0; break;
        case 'F': cursor = input.size(); break;
        case '3':
            if (readByte(10) == '~' && cursor < input.size())
                input.erase(cursor, nextCharacter(input, cursor) - cursor);
            break;
        default: break;
        }
    }

    void redrawUnlocked() {
        if (!started || !interactive)
            return;

        clearPromptUnlocked();

        const auto& style = owner.style();

        std::cout
            << "\033[" << static_cast<int>(style.cursor) << " q"
            << "\r\033[2K"
            << styled(path, style.pathColor)
            << '\n'
            << styled(style.prompt, style.promptColor) << ' '
            << input;

        const size_t tail = characterCount(std::string_view(input).substr(cursor));
        if (tail != 0)
            std::cout << "\033[" << tail << 'D';

        std::cout << std::flush;
        promptVisible = true;
    }

    void clearPromptUnlocked() {
        if (!promptVisible)
            return;

        std::cout << "\r\033[2K\033[1A\r\033[2K";
        promptVisible = false;
    }

    void setPath(std::string value) {
        std::lock_guard lock(mutex);
        path = std::move(value);
        redrawUnlocked();
    }

    std::mutex mutex;
    std::string input;
    size_t cursor = 0;
    std::vector<std::string> history;
    size_t historyIndex = 0;
    std::string path = "Root";
    bool interactive = false;
    bool started = false;
    bool promptVisible = false;

    #if defined(_WIN32)
        HANDLE inputHandle = INVALID_HANDLE_VALUE;
        HANDLE outputHandle = INVALID_HANDLE_VALUE;
        DWORD oldInputMode = 0;
        DWORD oldOutputMode = 0;
        bool inputConfigured = false;
        bool outputConfigured = false;
    #else
        termios oldTermios{};
        bool terminalConfigured = false;
    #endif
};

LocalTerminal::LocalTerminal(NodeBuild branch)
    : Terminal(branch),
      impl_(std::make_unique<Impl>(*this)) {}

LocalTerminal::~LocalTerminal() {
    detach();
}

void LocalTerminal::onAttach() { impl_->start(); }
void LocalTerminal::onDetach() { impl_->finish(); }
Terminal::Input LocalTerminal::onPoll() { return impl_->pollInput(); }
void LocalTerminal::onWrite(std::string_view text) { impl_->write(text); }
void LocalTerminal::onPathChanged(std::string path) { impl_->setPath(std::move(path)); }

}
