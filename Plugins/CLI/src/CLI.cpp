#include <CLI/include/CLI.hpp>

#include <chrono>
#include <thread>

#include <Lattice/Tools/LogTree.hpp>
#include <Lattice/Tools/Logger.hpp>

#include <CLI/include/LocalTerminal.hpp>
#include <CLI/include/ValueParser.hpp>


namespace CLIPlugin {

namespace {

std::string formatTreeNode(const Lattice::TreeNodeInfo& node) {
    std::string line;

    switch (node.kind) {
        case Lattice::NodeKind::Folder:
            line = std::format("{} <m>F</>", node.name);
            break;

        case Lattice::NodeKind::Component:
            line = node.name.empty()
                ? std::format("{} <g>C</>", node.type)
                : std::format("{}<gr>::{}</> <g>C</>", node.type, node.name);
            break;

        case Lattice::NodeKind::Slot:
            line = node.name.empty()
                ? node.type
                : std::format("{}<gr>::{}</>", node.type, node.name);
            line += node.hasObject
                ? std::format("<gr>::<c>{}<//> <c>S</>", node.implementation)
                : "<gr>::<c>empty<//> <c>S</>";
            break;

        case Lattice::NodeKind::Binding:
            line = std::format("{} <y>λ</>", node.name);
            break;

        case Lattice::NodeKind::Mount:
            line = node.name.empty()
                ? std::format("<m>[&{}]</> <bl>&</>", node.type)
                : std::format("<m>[&{}]</> <bl>&</>", node.name);
            break;

        case Lattice::NodeKind::SharedMount:
            line = node.name.empty()
                ? std::format("<m>[&&{}]</> <bl>&&</>", node.type)
                : std::format("<m>[&&{}]</> <bl>&&</>", node.name);
            break;
    }

    return line + std::format(" <gr>#{}</>", node.id);
}

}

CLI::CLI(NodeBuild branch) {
    tree_ = branch.tree();
    branch.action("ls", [this](Lattice::ActionContext& context) { list(context); });
    branch.action<std::string>(
        "cd",
        [this](Lattice::ActionContext& context, std::string path) {
            changeDirectory(context, std::move(path));
        }
    );
    branch.action("tree", [this](Lattice::ActionContext& context) { showTree(context); });
    branch.add<CLIPlugin::LocalTerminal>("local");
}

void CLI::configure(NodeConfigure branch) {
    commands_.setExports(branch.exports());
    commands_.setTree(tree_);
    terminals_ = branch.children<CLIPlugin::Terminal>();

    const Lattice::NodeId root = tree_.root(branch.id());
    for (CLIPlugin::Terminal* terminal : terminals_)
        terminal->setCurrent(root, tree_.path(root));
}

void CLI::list(Lattice::ActionContext& context) const {
    for (Lattice::NodeId child : tree_.children(context.node()))
        context.emit(Lattice::Value{Text::format("<b><c>{}<//>", tree_.label(child)).render()});
}

void CLI::showTree(Lattice::ActionContext& context) const {
    Logger::Tree logTree(tree_.label(context.node()));
    bool root = true;
    for (const Lattice::TreeEntry entry : tree_.subtree(context.node())) {
        if (root) {
            root = false;
            continue;
        }
        logTree.node(formatTreeNode(tree_.info(entry.id)), entry.depth - 1);
    }

    context.emit(Lattice::Value{logTree.text().render()});
}

void CLI::changeDirectory(Lattice::ActionContext& context, std::string path) const {
    const Lattice::NodeId target = CLIPlugin::resolveTreePath(tree_, context.node(), path);
    if (target == Lattice::InvalidNodeId)
        throw Lattice::Exception("CLI", "Path '{}' not found", path);

    context.setNode(target);
}

void CLI::run() {
    const auto writer = LogSystem::setConsoleWriter(
        [this](std::string_view text) { broadcast(text); }
    );

    for (CLIPlugin::Terminal* terminal : terminals_)
        terminal->attach();

    while (!stopRequested()) {
        if (!pollTerminals()) {
            requestStop();
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(16));
    }

    for (CLIPlugin::Terminal* terminal : terminals_)
        terminal->detach();

    LogSystem::resetConsoleWriter(writer);
}

void CLI::broadcast(std::string_view text) {
    for (CLIPlugin::Terminal* terminal : terminals_)
        terminal->write(text);
}

bool CLI::pollTerminals() {
    bool anyAttached = false;

    for (CLIPlugin::Terminal* terminal : terminals_) {
        if (!terminal->attached())
            continue;

        auto input = terminal->poll();
        if (input.closed) {
            terminal->detach();
            continue;
        }

        if (input.line &&
            commands_.execute(*terminal, *input.line) == CLIPlugin::CommandResult::Detach) {
            terminal->detach();
            continue;
        }

        anyAttached = true;
    }

    return anyAttached;
}

}