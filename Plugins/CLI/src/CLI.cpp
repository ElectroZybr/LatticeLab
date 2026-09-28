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
            line = std::format("{} <a>F</>", node.name);
            break;

        case Lattice::NodeKind::Component:
            line = node.name.empty()
                ? std::format("{} <param>C</>", node.type)
                : std::format("{}<mut2>::{}</> <param>C</>", node.type, node.name);
            break;

        case Lattice::NodeKind::Slot:
            line = node.name.empty()
                ? node.type
                : std::format("{}<mut2>::{}</>", node.type, node.name);
            line += node.hasObject
                ? std::format("<mut2>::</><a2>{}</> <a2>S</>", node.implementation)
                : "<mut2>::</><a2>empty</> <a2>S</>";
            break;

        case Lattice::NodeKind::Binding:
            line = std::format("{} <action>λ</>", node.name);
            break;

        case Lattice::NodeKind::Mount:
            line = node.name.empty()
                ? std::format("<a>[&{}]</> <h2>&</>", node.type)
                : std::format("<a>[&{}]</> <h2>&</>", node.name);
            break;

        case Lattice::NodeKind::SharedMount:
            line = node.name.empty()
                ? std::format("<a>[&&{}]</> <h2>&&</>", node.type)
                : std::format("<a>[&&{}]</> <h2>&&</>", node.name);
            break;
    }

    return line + std::format(" <mut2>#{}</>", node.id);
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
        return std::format("<err>{}</> <mut>ambiguous: {}</>", name, candidates);
    }

    if (exports.kind(entry) == Lattice::ExportKind::Param) {
        const auto value = exports.value(entry);
        std::string formattedValue = value ? value->toString() : "unreadable";
        if (name.starts_with("theme.") && value && value->is<std::string>()) {
            TextFormatter preview;
            preview.append("●", Lattice::parseTextStyle(value->get<std::string>()));
            formattedValue = std::format("{} {}", preview.markup(), formattedValue);
        }
        return std::format(
            "<param>@</> {} <mut>= {}</> <mut2>#{}</>",
            name,
            formattedValue,
            entry.exportId
        );
    }

    std::string arguments;
    const auto argumentTypes = exports.argumentTypes(entry);
    const size_t requiredArguments = exports.requiredArguments(entry);
    for (size_t index = 0; index < argumentTypes.size(); ++index) {
        arguments += std::format(
            " <mut><{}{}></>",
            index < requiredArguments ? "" : "?",
            valueType(argumentTypes[index])
        );
    }

    return std::format("<action>λ</> {}{} <mut2>#{}</>", name, arguments, entry.exportId);
}

}

CLI::CLI(NodeBuild branch) {
    tree_ = branch.tree();

    // cli-styles param
    theme_.add("param", TextStyle::rgb(0x28D08A));
    theme_.add("action", TextStyle::rgb(0xffff55));
    for (auto& entry : theme_.entries())
        branch.param(std::format("theme.{}", entry.name), entry.style);

    // adding global actions
    branch.globalAction("ls", [this](Lattice::ActionContext& context) { showList(context); });
    branch.globalAction("help", [this](Lattice::ActionContext& context) { showHelp(context); });
    branch.globalAction<std::string>("cd", [this](Lattice::ActionContext& context, std::string path) {
        changeDirectory(context, std::move(path));
    });
    branch.globalAction("tree", [this](Lattice::ActionContext& context) { showTree(context); });
    branch.globalAction("logo", [this] { showLogo(); });

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

void CLI::showLogo() const {
    Logger::message(R"(<a2>
    __    ___  ____________________________   ________    ____
   / /   /   |/_  __/_  __/  _/ ____/ ____/  / ____/ /   /  _/
  / /   / /| | / /   / /  / // /   / __/    / /   / /    / /
 / /___/ ___ |/ /   / / _/ // /___/ /___   / /___/ /____/ /
/_____/_/  |_/_/   /_/ /___/\____/_____/   \____/_____/___/
</>)");
}

void CLI::showList(Lattice::ActionContext& context) const {
    for (Lattice::NodeId child : tree_.children(context.node()))
        context.emit(Lattice::Value{TextFormatter::format(theme_, "<h>{}</>", tree_.label(child)).render()});
}

void CLI::showHelp(Lattice::ActionContext& context) const {
    const auto entries = exports_.available(context.node());

    Lattice::TreeFormatter output(std::format("Context {}", tree_.path(context.node())), theme_);
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

    output.node("<a><b>Global<//>", 0);
    appendSection(true);

    bool hasLocal = false;
    for (const auto entry : entries)
        hasLocal = hasLocal || !entry.global;
    if (hasLocal) {
        output.node("<a2><b>Local<//>", 0);
        appendSection(false);
    }

    context.emit(Lattice::Value{output.format().render()});
}

void CLI::showTree(Lattice::ActionContext& context) const {
    Lattice::TreeFormatter logTree(tree_.label(context.node()), theme_);
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
        throw Lattice::Exception<CLI>("Path '{}' not found", path);

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
