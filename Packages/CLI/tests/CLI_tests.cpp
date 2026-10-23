// CLI over a plain Context runtime with scripted terminals; no package loading.
#include <thread>
#include <atomic>
#include <deque>
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
protected:
    void onAttach() override {}
    void onDetach() override {}
    Input onPoll() override {
        if (lines.empty()) return {.closed = !keepOpen};
        Input input{.line = lines.front()};
        lines.pop_front();
        return input;
    }
    void onWrite(std::string_view text) override { output += text; }
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
        cli->run([&] { ++idles; });
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
        check(second->attached() && !second->path.empty(), "added terminal attaches at the root");
        cli->setTerminals({second.get()});
        check(!first->attached(), "removed terminal is detached and no longer served");

        second->keepOpen = false;
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
        cli->run([] {});
        check(gain == 4 && sets == 1 && resets == 1, "CLI records edits and reset without rewriting state");
        runtime.nodes.exports.unobserveParameters(observer);
    }
    runtime.nodes.ops.destroyBranch(rootId);
}

}

int main() {
    parsesArguments();
    resolvesTreePaths();
    servesScriptedTerminal();
    terminalsChangeWhileServing();
    explicitAssignmentsAndReset();
    std::cout << "CLI: arguments, tree paths and scripted terminal passed\n";
}
