#pragma once

#include <algorithm>
#include <cstdint>
#include <format>
#include <string>
#include <string_view>
#include <vector>
#include <utility>
#include <cctype>

#include <Lattice/Kernel/Exception.hpp>


namespace Color {

inline constexpr std::string_view reset = "\033[0m";
inline constexpr std::string_view bold  = "\033[1m";

// Base colors
inline constexpr std::string_view black   = "\033[30m";
inline constexpr std::string_view red     = "\033[31m";
inline constexpr std::string_view green   = "\033[32m";
inline constexpr std::string_view yellow  = "\033[33m";
inline constexpr std::string_view blue    = "\033[34m";
inline constexpr std::string_view magenta = "\033[35m";
inline constexpr std::string_view cyan    = "\033[36m";
inline constexpr std::string_view white   = "\033[37m";

// Bright colors
inline constexpr std::string_view gray          = "\033[90m";
inline constexpr std::string_view brightRed     = "\033[91m";
inline constexpr std::string_view brightGreen   = "\033[92m";
inline constexpr std::string_view brightYellow  = "\033[93m";
inline constexpr std::string_view brightBlue    = "\033[94m";
inline constexpr std::string_view brightMagenta = "\033[95m";
inline constexpr std::string_view brightCyan    = "\033[96m";
inline constexpr std::string_view brightWhite   = "\033[97m";

// Semantic styles
inline constexpr std::string_view error   = brightRed;
inline constexpr std::string_view ok      = brightGreen;
inline constexpr std::string_view warning = brightYellow;
inline constexpr std::string_view prompt  = brightMagenta;


inline std::string paint(std::string_view text, std::string_view color) {
    std::string out;

    out.reserve(color.size() + text.size() + reset.size());

    out += color;
    out += text;
    out += reset;

    return out;
}

} // namespace Color


namespace Lattice {


enum class TextStyle : uint32_t {
    None = 0,

    // Modifiers
    Bold = 1u << 0,
    Dim  = 1u << 1,

    // Base colors
    Black   = 1u << 2,
    Red     = 1u << 3,
    Green   = 1u << 4,
    Yellow  = 1u << 5,
    Blue    = 1u << 6,
    Magenta = 1u << 7,
    Cyan    = 1u << 8,
    White   = 1u << 9,

    // Bright colors
    Gray          = 1u << 10,
    BrightRed     = 1u << 11,
    BrightGreen   = 1u << 12,
    BrightYellow  = 1u << 13,
    BrightBlue    = 1u << 14,
    BrightMagenta = 1u << 15,
    BrightCyan    = 1u << 16,
    BrightWhite   = 1u << 17
};


constexpr TextStyle operator|(TextStyle a, TextStyle b) noexcept {
    return static_cast<TextStyle>(
        static_cast<uint32_t>(a) |
        static_cast<uint32_t>(b)
    );
}


constexpr TextStyle operator&(TextStyle a, TextStyle b) noexcept {
    return static_cast<TextStyle>(
        static_cast<uint32_t>(a) &
        static_cast<uint32_t>(b)
    );
}


constexpr TextStyle operator~(TextStyle value) noexcept {
    return static_cast<TextStyle>(
        ~static_cast<uint32_t>(value)
    );
}


constexpr bool hasStyle(TextStyle value, TextStyle style) noexcept {
    return (value & style) != TextStyle::None;
}


struct TextSpan {
    std::string text;
    TextStyle style = TextStyle::None;
};


class Text {
public:

    Text() = default;

    explicit Text(std::string_view text) {
        parse(text);
    }


    template<typename... TArgs>
    static Text format(
        std::format_string<TArgs...> fmt,
        TArgs&&... args
    ) {
        return Text(
            std::format(
                fmt,
                std::forward<TArgs>(args)...
            )
        );
    }


    static Text format(const Text& text) {
        return text;
    }


    Text& operator+=(const Text& other) {
        append(other);
        return *this;
    }


    friend Text operator+(Text lhs, const Text& rhs) {
        lhs += rhs;
        return lhs;
    }


    Text& operator+=(std::string_view text) {
        append(text);
        return *this;
    }


    friend Text operator+(Text lhs, std::string_view rhs) {
        lhs += rhs;
        return lhs;
    }


    friend Text operator+(std::string_view lhs, Text rhs) {
        Text result(lhs);
        result += rhs;
        return result;
    }


    void parse(std::string_view text) {
        spans_.clear();

        std::vector<TextStyle> stack;
        stack.push_back(TextStyle::None);

        size_t position = 0;
        size_t textStart = 0;

        while (position < text.size()) {

            if (text[position] != '<') {
                ++position;
                continue;
            }

            const size_t end = text.find('>', position);

            if (end == std::string_view::npos) {
                ++position;
                continue;
            }

            const std::string_view tag =
                text.substr(
                    position + 1,
                    end - position - 1
                );

            // Closing tag: </>, <//>, <///>, ...
            if (isCloseTag(tag)) {

                append(
                    text.substr(
                        textStart,
                        position - textStart
                    ),
                    stack.back()
                );

                const size_t count = tag.size();

                if (count >= stack.size()) {
                    throw Exception(
                        "Text",
                        "Too many closing tags '<{}>'",
                        tag
                    );
                }

                for (size_t i = 0; i < count; ++i)
                    stack.pop_back();

                position = end + 1;
                textStart = position;

                continue;
            }

            // Opening style tag
            if (isStyleTag(tag)) {

                append(
                    text.substr(
                        textStart,
                        position - textStart
                    ),
                    stack.back()
                );

                const TextStyle style = styleFromTag(tag);

                stack.push_back(
                    combineStyle(
                        stack.back(),
                        style
                    )
                );

                position = end + 1;
                textStart = position;

                continue;
            }

            // Not a markup tag. Treat it as normal text.
            ++position;
        }

        append(
            text.substr(textStart),
            stack.back()
        );

        if (stack.size() != 1) {
            throw Exception(
                "Text",
                "Unclosed text style tag"
            );
        }
    }

    void append(std::string_view text, TextStyle style = TextStyle::None) {
        if (text.empty())
            return;

        if (!spans_.empty() && spans_.back().style == style) {
            spans_.back().text += text;
            return;
        }

        spans_.push_back({
            .text = std::string(text),
            .style = style
        });
    }

    void append(const Text& text) {
        for (const auto& span : text.spans_)
            append(span.text, span.style);
    }


    void append(
        const Text& text,
        TextStyle style
    ) {
        for (const auto& span : text.spans_) {
            append(
                span.text,
                combineStyle(span.style, style)
            );
        }
    }


    std::string plain() const {
        std::string result;

        for (const auto& span : spans_)
            result += span.text;

        return result;
    }


    std::string markup() const {
        std::string result;

        for (const auto& span : spans_) {
            std::string_view tags[3];
            const size_t count = collectTags(span.style, tags);

            for (size_t i = 0; i < count; ++i) {
                result += '<';
                result += tags[i];
                result += '>';
            }

            result += span.text;

            if (count != 0) {
                result += '<';
                result.append(count, '/');
                result += '>';
            }
        }

        return result;
    }


    std::string render() const {
        std::string result;

        for (const auto& span : spans_) {

            if (span.style == TextStyle::None)
                result += Color::gray;
            else
                result += ansi(span.style);

            result += span.text;
            result += Color::reset;
        }

        return result;
    }


    size_t length() const noexcept {
        size_t result = 0;

        for (const auto& span : spans_)
            result += span.text.size();

        return result;
    }


    size_t lines() const noexcept {
        if (spans_.empty())
            return 0;

        size_t result = 1;

        for (const auto& span : spans_) {
            result += std::count(
                span.text.begin(),
                span.text.end(),
                '\n'
            );
        }

        return result;
    }


    Text wrap(
        size_t width,
        size_t continuationIndent = 0
    ) const {

        if (width == 0)
            throw Exception(
                "Text",
                "Wrap width cannot be zero"
            );

        if (continuationIndent >= width)
            throw Exception(
                "Text",
                "Continuation indent must be less than wrap width"
            );

        Text result;

        size_t lineLength = 0;

        auto newWrappedLine = [&]() {
            result.append("\n");

            if (continuationIndent > 0) {
                result.append(
                    std::string(
                        continuationIndent,
                        ' '
                    ),
                    TextStyle::None
                );
            }

            lineLength = continuationIndent;
        };


        auto newExplicitLine = [&]() {
            result.append("\n");
            lineLength = 0;
        };


        for (const auto& span : spans_) {

            size_t position = 0;

            while (position < span.text.size()) {

                const size_t newline =
                    span.text.find('\n', position);

                const size_t lineEnd =
                    newline == std::string::npos
                        ? span.text.size()
                        : newline;

                std::string_view line =
                    std::string_view(span.text).substr(
                        position,
                        lineEnd - position
                    );

                while (!line.empty()) {

                    while (
                        !line.empty() &&
                        isSpace(line.front())
                    ) {
                        if (line.front() == ' ')
                            ++lineLength;

                        line.remove_prefix(1);
                    }

                    if (line.empty())
                        break;

                    size_t wordEnd = 0;

                    while (
                        wordEnd < line.size() &&
                        !isSpace(line[wordEnd])
                    ) {
                        ++wordEnd;
                    }

                    const std::string_view word =
                        line.substr(0, wordEnd);

                    const size_t wordLength =
                        utf8Length(word);

                    if (
                        lineLength != 0 &&
                        lineLength + 1 + wordLength > width
                    ) {
                        newWrappedLine();
                    }

                    if (
                        wordLength >
                        width - lineLength
                    ) {

                        size_t consumed = 0;

                        while (consumed < word.size()) {

                            size_t bytes = 0;
                            size_t chars = 0;

                            while (
                                consumed + bytes < word.size() &&
                                lineLength + chars < width
                            ) {

                                const unsigned char c =
                                    static_cast<unsigned char>(
                                        word[consumed + bytes]
                                    );

                                const size_t charBytes =
                                    (c & 0x80) == 0 ? 1 :
                                    (c & 0xE0) == 0xC0 ? 2 :
                                    (c & 0xF0) == 0xE0 ? 3 :
                                    (c & 0xF8) == 0xF0 ? 4 :
                                    1;

                                bytes += charBytes;
                                ++chars;
                            }

                            if (
                                lineLength != 0 &&
                                consumed == 0 &&
                                chars == 0
                            ) {
                                newWrappedLine();
                                continue;
                            }

                            result.append(
                                word.substr(
                                    consumed,
                                    bytes
                                ),
                                span.style
                            );

                            consumed += bytes;
                            lineLength += chars;

                            if (consumed < word.size())
                                newWrappedLine();
                        }

                    } else {

                        if (lineLength != 0) {
                            result.append(
                                " ",
                                TextStyle::None
                            );

                            ++lineLength;
                        }

                        result.append(
                            word,
                            span.style
                        );

                        lineLength += wordLength;
                    }

                    line.remove_prefix(wordEnd);
                }

                if (newline != std::string::npos) {
                    newExplicitLine();
                    position = newline + 1;
                } else {
                    position = span.text.size();
                }
            }
        }

        return result;
    }


private:

    // All colors occupy one logical category.
    static constexpr TextStyle colorMask =
        TextStyle::Black |
        TextStyle::Red |
        TextStyle::Green |
        TextStyle::Yellow |
        TextStyle::Blue |
        TextStyle::Magenta |
        TextStyle::Cyan |
        TextStyle::White |
        TextStyle::Gray |
        TextStyle::BrightRed |
        TextStyle::BrightGreen |
        TextStyle::BrightYellow |
        TextStyle::BrightBlue |
        TextStyle::BrightMagenta |
        TextStyle::BrightCyan |
        TextStyle::BrightWhite;


    static bool isColor(TextStyle style) noexcept {
        return (style & colorMask) != TextStyle::None;
    }


    static TextStyle combineStyle(
        TextStyle current,
        TextStyle added
    ) noexcept {

        // A color replaces the inherited color.
        if (isColor(added)) {
            current = current & ~colorMask;
        }

        // Modifiers such as Bold and Dim are accumulated.
        return current | added;
    }


    static bool isCloseTag(
        std::string_view tag
    ) noexcept {

        if (tag.empty())
            return false;

        for (const char c : tag) {
            if (c != '/')
                return false;
        }

        return true;
    }


    static size_t collectTags(
        TextStyle style,
        std::string_view* tags
    ) noexcept {

        size_t count = 0;

        if (hasStyle(style, TextStyle::Bold))
            tags[count++] = "b";

        if (hasStyle(style, TextStyle::Dim))
            tags[count++] = "d";

        if (hasStyle(style, TextStyle::Black))
            tags[count++] = "k";
        else if (hasStyle(style, TextStyle::Red))
            tags[count++] = "r";
        else if (hasStyle(style, TextStyle::Green))
            tags[count++] = "g";
        else if (hasStyle(style, TextStyle::Yellow))
            tags[count++] = "y";
        else if (hasStyle(style, TextStyle::Blue))
            tags[count++] = "bl";
        else if (hasStyle(style, TextStyle::Magenta))
            tags[count++] = "m";
        else if (hasStyle(style, TextStyle::Cyan))
            tags[count++] = "c";
        else if (hasStyle(style, TextStyle::White))
            tags[count++] = "w";
        else if (hasStyle(style, TextStyle::Gray))
            tags[count++] = "gr";
        else if (hasStyle(style, TextStyle::BrightRed))
            tags[count++] = "br";
        else if (hasStyle(style, TextStyle::BrightGreen))
            tags[count++] = "bg";
        else if (hasStyle(style, TextStyle::BrightYellow))
            tags[count++] = "by";
        else if (hasStyle(style, TextStyle::BrightBlue))
            tags[count++] = "bbl";
        else if (hasStyle(style, TextStyle::BrightMagenta))
            tags[count++] = "bm";
        else if (hasStyle(style, TextStyle::BrightCyan))
            tags[count++] = "bc";
        else if (hasStyle(style, TextStyle::BrightWhite))
            tags[count++] = "bw";

        return count;
    }


    static TextStyle styleFromTag(
        std::string_view tag
    ) {

        if (tag == "b")  return TextStyle::Bold;
        if (tag == "d")  return TextStyle::Dim;

        if (tag == "k")  return TextStyle::Black;
        if (tag == "r")  return TextStyle::Red;
        if (tag == "g")  return TextStyle::Green;
        if (tag == "y")  return TextStyle::Yellow;
        if (tag == "bl") return TextStyle::Blue;
        if (tag == "m")  return TextStyle::Magenta;
        if (tag == "c")  return TextStyle::Cyan;
        if (tag == "w")  return TextStyle::White;

        if (tag == "gr")  return TextStyle::Gray;
        if (tag == "br")  return TextStyle::BrightRed;
        if (tag == "bg")  return TextStyle::BrightGreen;
        if (tag == "by")  return TextStyle::BrightYellow;
        if (tag == "bbl") return TextStyle::BrightBlue;
        if (tag == "bm")  return TextStyle::BrightMagenta;
        if (tag == "bc")  return TextStyle::BrightCyan;
        if (tag == "bw")  return TextStyle::BrightWhite;

        throw Exception(
            "Text",
            "Unknown style tag '<{}>'",
            tag
        );
    }


    static std::string ansi(
        TextStyle style
    ) {

        std::string result;

        if (hasStyle(style, TextStyle::Bold))
            result += Color::bold;

        if (hasStyle(style, TextStyle::Dim))
            result += Color::gray;

        if (hasStyle(style, TextStyle::Black))
            result += Color::black;

        else if (hasStyle(style, TextStyle::Red))
            result += Color::red;

        else if (hasStyle(style, TextStyle::Green))
            result += Color::green;

        else if (hasStyle(style, TextStyle::Yellow))
            result += Color::yellow;

        else if (hasStyle(style, TextStyle::Blue))
            result += Color::blue;

        else if (hasStyle(style, TextStyle::Magenta))
            result += Color::magenta;

        else if (hasStyle(style, TextStyle::Cyan))
            result += Color::cyan;

        else if (hasStyle(style, TextStyle::White))
            result += Color::white;

        else if (hasStyle(style, TextStyle::Gray))
            result += Color::gray;

        else if (hasStyle(style, TextStyle::BrightRed))
            result += Color::brightRed;

        else if (hasStyle(style, TextStyle::BrightGreen))
            result += Color::brightGreen;

        else if (hasStyle(style, TextStyle::BrightYellow))
            result += Color::brightYellow;

        else if (hasStyle(style, TextStyle::BrightBlue))
            result += Color::brightBlue;

        else if (hasStyle(style, TextStyle::BrightMagenta))
            result += Color::brightMagenta;

        else if (hasStyle(style, TextStyle::BrightCyan))
            result += Color::brightCyan;

        else if (hasStyle(style, TextStyle::BrightWhite))
            result += Color::brightWhite;

        return result;
    }


    static bool isStyleTag(
        std::string_view tag
    ) noexcept {

        return
            tag == "b"   ||
            tag == "d"   ||

            tag == "k"   ||
            tag == "r"   ||
            tag == "g"   ||
            tag == "y"   ||
            tag == "bl"  ||
            tag == "m"   ||
            tag == "c"   ||
            tag == "w"   ||

            tag == "gr"  ||
            tag == "br"  ||
            tag == "bg"  ||
            tag == "by"  ||
            tag == "bbl" ||
            tag == "bm"  ||
            tag == "bc"  ||
            tag == "bw";
    }


    static size_t utf8Length(
        std::string_view text
    ) {

        size_t length = 0;

        for (size_t i = 0; i < text.size();) {

            const unsigned char c =
                static_cast<unsigned char>(text[i]);

            if ((c & 0x80) == 0)
                i += 1;

            else if ((c & 0xE0) == 0xC0)
                i += 2;

            else if ((c & 0xF0) == 0xE0)
                i += 3;

            else if ((c & 0xF8) == 0xF0)
                i += 4;

            else
                i += 1;

            ++length;
        }

        return length;
    }


    static bool isSpace(char c) {
        return std::isspace(
            static_cast<unsigned char>(c)
        );
    }


    std::vector<TextSpan> spans_;
};


} // namespace Lattice


using Lattice::Text;
using Lattice::TextStyle;