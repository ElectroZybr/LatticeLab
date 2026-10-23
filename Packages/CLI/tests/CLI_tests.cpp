// CLI over a plain Context runtime with scripted terminals; no plugin loading.
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
static_assert(!std::is_base_of_v<ServiceAPI, CLIPlugin::CLI>);
static_assert(!std::is_base_of_v<Component, CLIPlugin::Terminal>);

namespace {
void check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

class ScriptTerminal final : public CLIPlugin::Terminal {
public:
    std::deque<std::string> lines;
    std::string output;
    std::string path;
protected:
    void onAttach() override {}
    void onDetach() override {}
    Input onPoll() override {
        if (lines.empty()) return {.closed = true};
        Input input{.line = lines.front()};
        lines.pop_front();
        return input;
    }
    void onWrite(std::string_view text) override { output += text; }
    void onPathChanged(std::string value) override { path = std::move(value); }
};

struct PathComponent {};

void parsesArguments() {
    const auto parsed = CLIPlugin::parseArguments("one \"two words\" three\\ four");
    check(parsed && parsed->size() == 3, "argument count");
    check((*parsed)[0] == "one" && (*parsed)[1] == "two words" && (*parsed)[2] == "three four", "arguments");
    check(!CLIPlugin::parseArguments("\"unfinished").has_value(), "unfinished quote");
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
        check(CLIPlugin::resolveTreePath(tree, rootId, "folder") == folderId, "child path");
        check(CLIPlugin::resolveTreePath(tree, folderId, "PathComponent:one") == component.id(), "typed name");
        check(CLIPlugin::resolveTreePath(tree, component.id(), "..") == folderId, "parent");
        check(CLIPlugin::resolveTreePath(tree, component.id(), "/") == rootId, "root");
        check(CLIPlugin::resolveTreePath(tree, component.id(), "Root/folder") == folderId, "absolute");
        check(CLIPlugin::resolveTreePath(tree, rootId, "missing") == InvalidNodeId, "missing");
    }
    runtime.nodes.ops.destroyBranch(rootId);
}

void servesScriptedTerminal() {
    Context runtime;
    const NodeId rootId = runtime.nodes.factory.folder(InvalidNodeId, "Root");
    {
        ::NodeBuild root{rootId, runtime.nodes};
        root.branch("folder");
        auto terminal = root.emplace<ScriptTerminal, CLIPlugin::Terminal>("script");
        terminal->lines = {"cd folder", "ls", "nope"};
        auto cli = root.emplace<CLIPlugin::CLI>("cli",
            root.exports(), root.tree(), root.id(), root.children<CLIPlugin::Terminal>());
        cli.globalAction("ls", &CLIPlugin::CLI::showList);
        cli.globalAction("cd", &CLIPlugin::CLI::changeDirectory);
        check(!terminal->path.empty(), "initial path is set");

        int idles = 0;
        cli->run([&] { ++idles; });
        check(idles > 0, "idle is called while serving");
        check(terminal->current() != rootId && terminal->path.find("folder") != std::string::npos, "cd moved terminal");
        check(terminal->output.find("Unknown command 'nope'") != std::string::npos, "unknown command reported");
        check(!terminal->attached(), "closed terminal detached");
    }
    runtime.nodes.ops.destroyBranch(rootId);
}
}

int main() {
    parsesArguments();
    resolvesTreePaths();
    servesScriptedTerminal();
    std::cout << "CLI: arguments, tree paths and scripted terminal passed\n";
}
