// CLI over a plain Context runtime with scripted terminals; no package loading.
#include <thread>
#include <atomic>
#include <deque>
#include <chrono>
#include <iostream>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <Lattice/Composition.hpp>
#include <Lattice/Kernel/Context.hpp>

#include <CLI/CLI.hpp>
#include <CLI/Command.hpp>
#include <CLI/ValueParser.hpp>

using namespace Lattice;
static_assert(!std::is_base_of_v<ServiceAPI, Console::CLI>);

namespace {
void check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

class ScriptTerminal final : public Console::Terminal {
public:
    std::deque<std::string> lines;
    std::atomic<bool> keepOpen{false};
    std::string output;
    std::string path;
    std::thread::id wroteOn;
protected:
    void onAttach() override {}
    void onDetach() override {}
    Input onPoll() override {
        if (lines.empty()) return {.closed = !keepOpen};
        Input input{.line = lines.front()};
        lines.pop_front();
        return input;
    }
    void onWrite(std::string_view text) override { wroteOn = std::this_thread::get_id(); output += text; }
    void onPathChanged(std::string value) override { path = std::move(value); }
};

struct PathComponent {};

void parsesArguments() {
    const auto parsed = Console::parseArguments("one \"two words\" three\\ four");
    check(parsed && parsed->size() == 3, "argument count");
    check((*parsed)[0] == "one" && (*parsed)[1] == "two words" && (*parsed)[2] == "three four", "arguments");
    check(!Console::parseArguments("\"unfinished").has_value(), "unfinished quote");
}

void resolvesTreePaths() {
    Context runtime;
    const NodeId rootId = runtime.nodes.factory.folder(InvalidNodeId, "Root");
    {
        ::NodeBuild root{rootId, runtime.nodes};
        auto folder = root.branch("folder");
        auto component = folder.emplace<PathComponent>("one");
        const TreeView tree = root.tree();
        const NodeId folderId = folder.id();
        check(Console::resolveTreePath(tree, rootId, "folder") == folderId, "child path");
        check(Console::resolveTreePath(tree, folderId, "PathComponent:one") == component.id(), "typed name");
        check(Console::resolveTreePath(tree, component.id(), "..") == folderId, "parent");
        check(Console::resolveTreePath(tree, component.id(), "/") == rootId, "root");
        check(Console::resolveTreePath(tree, component.id(), "Root/folder") == folderId, "absolute");
        check(Console::resolveTreePath(tree, rootId, "missing") == InvalidNodeId, "missing");
    }
    runtime.nodes.ops.destroyBranch(rootId);
}

void servesScriptedTerminal() {
    Context runtime;
    const NodeId rootId = runtime.nodes.factory.folder(InvalidNodeId, "Root");
    {
        ::NodeBuild root{rootId, runtime.nodes};
        root.branch("folder");
        auto terminal = root.emplace<ScriptTerminal, Console::Terminal>("script");
        terminal->lines = {"cd folder", "ls", "nope", "cd missing"};
        auto cli = root.emplace<Console::CLI>("cli", root.exports(), root.tree(), root.id());
        cli->setTerminals({terminal.get()});
        cli.globalAction("ls", &Console::CLI::showList);
        cli.globalAction("cd", &Console::CLI::changeDirectory);
        check(!terminal->path.empty(), "initial path is set");

        cli->theme().style(cli->theme().find("err")) = TextStyle::rgb(0x336688);
        int idles = 0;
        cli->run([&] { ++idles; cli->processCommands(); });
        check(idles > 0, "idle is called while serving");
        check(terminal->current() != rootId && terminal->path.find("folder") != std::string::npos, "cd moved terminal");
        check(terminal->output.find("Unknown command 'nope'") != std::string::npos, "unknown command reported");
        check(terminal->output.find("38;2;51;102;136m") != std::string::npos,
              "CLI errors must use configured theme.err");
        check(!terminal->attached(), "closed terminal detached");
    }
    runtime.nodes.ops.destroyBranch(rootId);
}
void terminalsChangeWhileServing() {
    Context runtime;
    const NodeId rootId = runtime.nodes.factory.folder(InvalidNodeId, "Root");
    {
        ::NodeBuild root{rootId, runtime.nodes};
        auto first = root.emplace<ScriptTerminal, Console::Terminal>("first");
        auto second = root.emplace<ScriptTerminal, Console::Terminal>("second");
        first->keepOpen = true;
        second->keepOpen = true;
        auto cli = root.emplace<Console::CLI>("cli", root.exports(), root.tree(), root.id());
        cli->setTerminals({first.get()});
        std::thread worker([&] { cli->run([] {}); });
        while (!first->attached())
            std::this_thread::yield();

        cli->setTerminals({first.get(), second.get()});
        for (int i = 0; i < 1000 && !second->attached(); ++i)
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        check(second->attached() && !second->path.empty(), "added terminal attaches at the root");
        cli->setTerminals({second.get()});
        check(!first->attached(), "removed terminal is detached and no longer served");

        second->keepOpen = false;
        for (int i = 0; i < 1000 && second->attached(); ++i) {
            cli->processCommands();
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        cli->requestStop();
        worker.join();
        check(!second->attached(), "closed terminal detached");
    }
    runtime.nodes.ops.destroyBranch(rootId);
}

void explicitAssignmentsAndReset() {
    Context runtime;
    const auto rootId = runtime.nodes.factory.folder(InvalidNodeId, "Root");
    int gain = 1, sets = 0, resets = 0;
    {
        NodeBuild root{rootId, runtime.nodes};
        const auto id = root.param("gain", gain);
        const auto observer = runtime.nodes.exports.observeParameters([&](ExportId target, ParamChange change) {
            check(target == id, "assignment identity");
            if (change == ParamChange::Set) ++sets;
            if (change == ParamChange::Reset) ++resets;
        });
        auto terminal = root.emplace<ScriptTerminal, Console::Terminal>("script");
        terminal->lines = {"gain 4", "reset gain"};
        auto cli = root.emplace<Console::CLI>("cli", root.exports(), root.tree(), root.id());
        cli->setTerminals({terminal.get()});
        cli->run([&] { cli->processCommands(); });
        check(gain == 4 && sets == 1 && resets == 1, "CLI records edits and reset without rewriting state");
        runtime.nodes.exports.unobserveParameters(observer);
    }
    runtime.nodes.ops.destroyBranch(rootId);
}

struct RunningCLI {
    Console::CLI& cli;
    std::thread thread;
    explicit RunningCLI(Console::CLI& cli) : cli(cli), thread([&cli] { cli.run([] {}); }) {}
    ~RunningCLI() { cli.requestStop(); thread.join(); }
};
template<class Predicate>
void waitFor(Predicate done, const char* message) {
    for (int i = 0; i < 2000; ++i) {
        if (done()) return;
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    throw std::runtime_error(message);
}

void commandsRunOnHostAndSurviveErrors() {
    Context runtime;
    const auto id = runtime.nodes.factory.folder(InvalidNodeId, "Root");
    NodeBuild root{id, runtime.nodes};
    root.branch("folder");
    auto terminal = root.emplace<ScriptTerminal, Console::Terminal>("script");
    terminal->lines = {"cd folder", "fail", "mark"};
    auto cli = root.emplace<Console::CLI>("cli", root.exports(), root.tree(), root.id());
    cli->setTerminals({terminal.get()});
    cli.onMaintain(&Console::CLI::processCommands);
    cli.globalAction("cd", &Console::CLI::changeDirectory);
    const auto host = std::this_thread::get_id();
    std::thread::id executed;
    root.globalAction("fail", [] { throw std::runtime_error("expected failure"); });
    root.globalAction("mark", [&] { executed = std::this_thread::get_id(); });
    {
        RunningCLI worker{*cli};
        waitFor([&] { return terminal->attached(); }, "terminal did not attach");
        std::this_thread::sleep_for(std::chrono::milliseconds(70));
        check(executed == std::thread::id{}, "worker must not execute commands without a host checkpoint");
        waitFor([&] {
            runtime.nodes.ops.maintain();
            return executed == host && !terminal->attached();
        }, "host did not execute queued commands or drain EOF");
    }
    check(terminal->path.find("folder") != std::string::npos, "navigation replies preserve command order");
    check(terminal->output.find("expected failure") != std::string::npos, "action exception returned to terminal");
    check(terminal->wroteOn != host, "physical output belongs to the I/O thread");
    runtime.nodes.ops.destroyBranch(id);
}

void removingTerminalCancelsRequestsAndStopDoesNotWait() {
    Context runtime;
    const auto id = runtime.nodes.factory.folder(InvalidNodeId, "Root");
    NodeBuild root{id, runtime.nodes};
    int calls = 0;
    root.globalAction("mark", [&] { ++calls; });
    auto terminal = root.emplace<ScriptTerminal, Console::Terminal>("script");
    terminal->lines = {"mark"};
    terminal->keepOpen = true;
    auto cli = root.emplace<Console::CLI>("cli", root.exports(), root.tree(), root.id());
    cli->setTerminals({terminal.get()});
    cli.onMaintain(&Console::CLI::processCommands);
    {
        RunningCLI worker{*cli};
        waitFor([&] { return terminal->attached(); }, "terminal did not attach");
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        cli->setTerminals({});
        runtime.nodes.ops.destroyBranch(terminal.id());
        runtime.nodes.ops.maintain();
        check(calls == 0, "requests of a removed terminal must not execute");
    }
    runtime.nodes.ops.destroyBranch(id);

    Context secondRuntime;
    const auto secondId = secondRuntime.nodes.factory.folder(InvalidNodeId, "Root");
    NodeBuild secondRoot{secondId, secondRuntime.nodes};
    auto second = secondRoot.emplace<ScriptTerminal, Console::Terminal>("script");
    second->lines = {"mark"};
    second->keepOpen = true;
    auto secondCLI = secondRoot.emplace<Console::CLI>("cli", secondRoot.exports(), secondRoot.tree(), secondId);
    secondCLI->setTerminals({second.get()});
    {
        RunningCLI worker{*secondCLI};
        waitFor([&] { return second->attached(); }, "terminal did not attach");
        // No host pumping: requestStop and join must still finish.
    }
    secondRuntime.nodes.ops.destroyBranch(secondId);
}

}

int main() {
    parsesArguments();
    resolvesTreePaths();
    servesScriptedTerminal();
    terminalsChangeWhileServing();
    explicitAssignmentsAndReset();
    commandsRunOnHostAndSurviveErrors();
    removingTerminalCancelsRequestsAndStopDoesNotWait();
    std::cout << "CLI: arguments, tree paths and scripted terminal passed\n";
}
