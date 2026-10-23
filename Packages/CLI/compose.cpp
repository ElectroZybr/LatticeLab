#include <format>
#include <Lattice/Composition.hpp>
#include <CLI/CLI.hpp>
#include <CLI/LocalTerminal.hpp>

extern "C" constinit const Lattice::Package lattice_package{
    .id = "CLI",
    .version = {1, 0, 0},
};

namespace {
void exportStyle(Lattice::ObjectBuild<CLIPlugin::LocalTerminal>& terminal) {
    auto& style = terminal->style();
    terminal.param("prompt", style.prompt);
    terminal.param("prompt_color", style.promptColor);
    terminal.param("path_color", style.pathColor);
}
}

// Interactive runtime shell. CLI and terminals are ordinary classes; commands,
// theme params and the service are declared here.
extern "C" void compose(NodeBuild root) {
    auto local = root.emplace<CLIPlugin::LocalTerminal, CLIPlugin::Terminal>("local");
    exportStyle(local);

    auto cli = root.emplace<CLIPlugin::CLI>("cli",
        root.exports(), root.tree(), root.id(), root.children<CLIPlugin::Terminal>());
    for (auto& entry : cli->theme().entries())
        cli.param(std::format("theme.{}", entry.name), entry.style);

    cli.globalAction("ls", &CLIPlugin::CLI::showList);
    cli.globalAction("help", &CLIPlugin::CLI::showHelp);
    cli.globalAction("cd", &CLIPlugin::CLI::changeDirectory);
    cli.globalAction("tree", &CLIPlugin::CLI::showTree);
    cli.globalAction("logo", &CLIPlugin::CLI::showLogo);
    cli.service(&CLIPlugin::CLI::run, &CLIPlugin::CLI::requestStop);
}
