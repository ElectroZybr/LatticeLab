#include <format>
#include <Lattice/Composition.hpp>
#include <CLI/CLI.hpp>
#include <CLI/LocalTerminal.hpp>

extern "C" constinit const Lattice::Package lattice_package{
    .id = "CLI",
    .version = {1, 0, 0},
};

namespace {
void exportStyle(Lattice::ObjectBuild<Console::LocalTerminal>& terminal) {
    auto& style = terminal->style();
    terminal.param("prompt", style.prompt);
    terminal.param("prompt_color", style.promptColor);
    terminal.param("path_color", style.pathColor);
}
}

extern "C" void compose(NodeBuild root) {
    auto local = root.emplace<Console::LocalTerminal, Console::Terminal>("local");
    exportStyle(local);

    auto cli = root.construct<Console::CLI>(root.exports(), root.tree(), root.id());
    for (auto& entry : cli->theme().entries())
        cli.param(std::format("theme.{}", entry.name), entry.style);

    cli.globalAction("ls", &Console::CLI::showList);
    cli.globalAction("help", &Console::CLI::showHelp);
    cli.globalAction("cd", &Console::CLI::changeDirectory);
    cli.globalAction("tree", &Console::CLI::showTree);
    cli.globalAction("exports", &Console::CLI::showExports);
    cli.globalAction("logo", &Console::CLI::showLogo);
    cli.onMaintain(&Console::CLI::processCommands);
    cli.service(&Console::CLI::run, &Console::CLI::requestStop);
}

// CLI serves the terminals currently below its branch.
extern "C" void configure(NodeConfigure root) {
    root.self<Console::CLI>()->setTerminals(root.children<Console::Terminal>());
}
