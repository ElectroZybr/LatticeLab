#include <CLI/include/CLI.hpp>

#include <chrono>
#include <thread>

#include <Lattice/Tools/TreeFormatter.hpp>
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

std::string_view valueType(const Lattice::Value& value) {
    if (value.is<std::string>()) return "string";
    if (value.is<bool>()) return "bool";
    if (value.is<int64_t>()) return "int";
    if (value.is<double>()) return "number";
    if (value.is<glm::vec2>()) return "vec2";
    if (value.is<glm::vec3>()) return "vec3";
    if (value.is<glm::vec4>()) return "vec4";
    if (value.is<Lattice::Array>()) return "array";
    return "table";
}

std::string formatExport(
    const ExportsView& exports,
    const Lattice::VisibleExport& entry,
    std::string_view name
) {
    if (entry.state == Lattice::ContextResolutionState::Ambiguous) {
        std::string candidates;
        for (Lattice::NodeId candidate : exports.candidates(entry)) {
            if (!candidates.empty()) candidates += ", ";
            candidates += std::format("#{}", candidate);
        }
        return std::format("<r>{}</> <gr>ambiguous: {}</>", name, candidates);
    }

    if (exports.kind(entry) == Lattice::ExportKind::Param) {
        const auto value = exports.value(entry);
        return std::format(
            "<g>@</> {} <gr>=</> <c>{}</> <gr>#{}</>",
            name,
            value ? value->toString() : "unreadable",
            entry.exportId
        );
    }

    std::string arguments;
    for (const Lattice::Value& argument : exports.argumentTypes(entry))
        arguments += std::format(" <gr><{}></>", valueType(argument));

    return std::format("<y>λ</> {}{} <gr>#{}</>", name, arguments, entry.exportId);
}

}

void CLI::logo() const {
    Logger::message(R"(<c>
    __    ___  ____________________________   ________    ____
   / /   /   |/_  __/_  __/  _/ ____/ ____/  / ____/ /   /  _/
  / /   / /| | / /   / /  / // /   / __/    / /   / /    / /
 / /___/ ___ |/ /   / / _/ // /___/ /___   / /___/ /____/ /
/_____/_/  |_/_/   /_/ /___/\____/_____/   \____/_____/___/
</>)");
}


CLI::CLI(NodeBuild branch) {
    tree_ = branch.tree();
    branch.globalAction("ls", [this](Lattice::ActionContext& context) { list(context); });
    branch.globalAction("help", [this](Lattice::ActionContext& context) { help(context); });
    branch.globalAction<std::string>(
        "cd",
        [this](Lattice::ActionContext& context, std::string path) {
            changeDirectory(context, std::move(path));
        }
    );
    branch.globalAction("tree", [this](Lattice::ActionContext& context) { showTree(context); });
    branch.globalAction("logo", [this] { logo(); });
    branch.add<CLIPlugin::LocalTerminal>("local");
}

void CLI::configure(NodeConfigure branch) {
    exports_ = branch.exports();
    commands_.setExports(exports_);
    commands_.setTree(tree_);
    terminals_ = branch.children<CLIPlugin::Terminal>();

    const Lattice::NodeId root = tree_.root(branch.id());
    for (CLIPlugin::Terminal* terminal : terminals_)
        terminal->setCurrent(root, tree_.path(root));
}

void CLI::list(Lattice::ActionContext& context) const {
    for (Lattice::NodeId child : tree_.children(context.node()))
        context.emit(Lattice::Value{TextFormatter::format("<b><c>{}<//>", tree_.label(child)).render()});
}

void CLI::help(Lattice::ActionContext& context) const {
    const auto entries = exports_.available(context.node());

    Lattice::TreeFormatter output(std::format("Context {}", tree_.path(context.node())));
    const auto appendSection = [&](bool global) {
        for (const auto entry : entries) {
            if (entry.global != global)
                continue;

            std::string names{exports_.name(entry)};
            if (entry.exportId != Lattice::InvalidExportId) {
                bool first = true;
                for (const auto candidate : entries) {
                    if (candidate.global != global || candidate.exportId != entry.exportId)
                        continue;
                    if (candidate.role == entry.role)
                        break;
                    first = false;
                    break;
                }
                if (!first)
                    continue;

                for (const auto alias : entries) {
                    if (alias.global != global ||
                        alias.exportId != entry.exportId ||
                        alias.role == entry.role)
                        continue;
                    names += " | ";
                    names += exports_.name(alias);
                }
            }

            output.node(formatExport(exports_, entry, names), 1);
        }
    };

    output.node("<b><c>Global<//>", 0);
    appendSection(true);

    bool hasLocal = false;
    for (const auto entry : entries)
        hasLocal = hasLocal || !entry.global;
    if (hasLocal) {
        output.node("<b><m>Local<//>", 0);
        appendSection(false);
    }

    context.emit(Lattice::Value{output.format().render()});
}

void CLI::showTree(Lattice::ActionContext& context) const {
    Lattice::TreeFormatter logTree(tree_.label(context.node()));
    bool root = true;
    for (const Lattice::TreeEntry entry : tree_.subtree(context.node())) {
        if (root) {
            root = false;
            continue;
        }
        logTree.node(formatTreeNode(tree_.info(entry.id)), entry.depth - 1);
    }

    context.emit(Lattice::Value{logTree.format().render()});
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

    for (CLIPlugin::Terminal* terminal : terminals_) {
        terminal->attach();

        const auto resolved = commands_.getExports().resolve(
            commands_.getExports().role("logo"),
            terminal->current()
        );

        if (resolved && resolved.invoke) {
            Lattice::ActionContext context{terminal->current()};
            resolved.invoke(resolved.object, context, {});
        }
    }

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

        if (input.line)
            commands_.execute(*terminal, *input.line);

        anyAttached = true;
    }

    return anyAttached;
}

}
