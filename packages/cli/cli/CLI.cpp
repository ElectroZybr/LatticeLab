#include <algorithm>
#include "CLI.hpp"

#include <chrono>
#include <memory>
#include <thread>

#include <Lattice/tools/TreeFormatter.hpp>
#include <lattice-logger/Logger.hpp>

#include "ValueParser.hpp"


namespace Console {

namespace {

std::string formatTreeNode(const Lattice::TreeNodeInfo& node) {
    std::string line;

    switch (node.kind) {
        case Lattice::NodeKind::Folder:
            line = std::format("{} <a>F</>", node.name);
            break;

        case Lattice::NodeKind::Object:
            line = node.name.empty()
                ? std::format("{} <param>C</>", node.type)
                : std::format("{}<mut2>::{}</> <param>C</>", node.type, node.name);
            break;

        case Lattice::NodeKind::Slot:
            line = node.name.empty()
                ? node.type
                : std::format("{}<mut2>::{}</>", node.type, node.name);
            // Composition slots own their implementation in a child node: no object here.
            line += !node.implementation.empty()
                ? std::format("<mut2>::</><a2>{}</> <a2>S</>", node.implementation)
                : "<mut2>::</><a2>empty</> <a2>S</>";
            break;

        case Lattice::NodeKind::Binding:
            line = std::format("{} <action>λ</>", node.name);
            break;
    }

    if (node.service) line += " <a2>[service]</>";
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

CLI::CLI(ExportsView exports, Lattice::TreeView tree, Lattice::NodeId context)
    : exports_(exports), tree_(tree) {
    if (theme_.find("param") == Lattice::TextTheme::InvalidTextStyleId)
        theme_.add("param", TextStyle::rgb(0x28D08A));
    if (theme_.find("action") == Lattice::TextTheme::InvalidTextStyleId)
        theme_.add("action", TextStyle::rgb(0xffff55));

    commands_.setExports(exports_);
    commands_.setTree(tree_);
    commands_.setTheme(theme_);

    root_ = tree_.root(context);
    rootPath_ = tree_.path(root_);
}

void CLI::setTerminals(std::vector<Terminal*> terminals) {
    std::lock_guard lock(mutex_);
    std::erase_if(sessions_, [&](Session& session) {
        if (std::ranges::find(terminals, session.terminal) != terminals.end()) return false;
        session.terminal->detach();
        std::erase_if(requests_, [&](const Request& request) { return request.session == session.id; });
        return true;
    });
    for (Terminal* terminal : terminals) {
        if (std::ranges::find(sessions_, terminal, &Session::terminal) != sessions_.end()) continue;
        terminal->setCurrent(root_, rootPath_);
        sessions_.push_back(Session{.id = nextSession_++, .terminal = terminal, .current = root_});
    }
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

void CLI::showExports(Lattice::ActionContext& context) const {
    Lattice::TreeFormatter output(tree_.label(context.node()), theme_);

    const auto formatOwned = [&](Lattice::ExportId id) {
        const std::string_view name = exports_.name(id);
        if (exports_.kind(id) == Lattice::ExportKind::Param) {
            const auto value = exports_.value(id);
            std::string text = value ? value->toString() : "unreadable";
            // Color params get a preview dot, as in help.
            if (value && value->is<std::string>() && (name.starts_with("theme.") || name.ends_with("color"))) {
                TextFormatter preview;
                preview.append("●", Lattice::parseTextStyle(value->get<std::string>()));
                text = std::format("{} {}", preview.markup(), text);
            }
            return std::format("<param>@</> {} <mut>= {}</> <mut2>#{}</>", name, text, id);
        }
        std::string arguments;
        const auto types = exports_.argumentTypes(id);
        const size_t required = exports_.requiredArguments(id);
        for (size_t index = 0; index < types.size(); ++index)
            arguments += std::format(" <mut><{}{}></>", index < required ? "" : "?", valueType(types[index]));
        return std::format("<action>λ</> {}{} <mut2>#{}</>", name, arguments, id);
    };
    // Global exports are listed once, at the root; owners show only their local ones.
    const auto local = [&](Lattice::NodeId node) {
        std::vector<Lattice::ExportId> result;
        for (Lattice::ExportId id : exports_.owned(node))
            if (!exports_.global(id)) result.push_back(id);
        return result;
    };
    if (tree_.parent(context.node()) == Lattice::InvalidNodeId)
        for (const Lattice::TreeEntry entry : tree_.subtree(context.node()))
            for (Lattice::ExportId id : exports_.owned(entry.id))
                if (exports_.global(id)) output.node(formatOwned(id), 0);

    // A node is shown only when it or a descendant owns exports.
    const auto owns = [&](auto&& self, Lattice::NodeId node) -> bool {
        if (!local(node).empty())
            return true;
        for (Lattice::NodeId child : tree_.children(node))
            if (self(self, child)) return true;
        return false;
    };
    const auto append = [&](auto&& self, Lattice::NodeId node, size_t depth) -> void {
        if (depth > 0)
            output.node(std::format("<h>{}</> <mut2>#{}</>", tree_.label(node), node), depth - 1);
        for (Lattice::ExportId id : local(node))
            output.node(formatOwned(id), depth);
        for (Lattice::NodeId child : tree_.children(node))
            if (owns(owns, child)) self(self, child, depth + 1);
    };
    append(append, context.node(), 0);

    context.emit(Lattice::Value{output.format().render()});
}

void CLI::changeDirectory(Lattice::ActionContext& context, std::string path) const {
    const Lattice::NodeId target = Console::resolveTreePath(tree_, context.node(), path);
    if (target == Lattice::InvalidNodeId)
        throw Lattice::Exception<CLI>("Path '{}' not found", path);

    context.setNode(target);
}

namespace {
// A host-local command session. It captures output and navigation, never terminal I/O.
class CommandTerminal final : public Terminal {
public:
    std::string output;
    std::string path;
protected:
    void onAttach() override {}
    void onDetach() override {}
    Input onPoll() override { return {}; }
    void onWrite(std::string_view text) override { output += text; }
    void onPathChanged(std::string value) override { path = std::move(value); }
};
}

void CLI::processCommands() {
    if (processing_) return;
    processing_ = true;
    struct Guard { bool& active; ~Guard() { active = false; } } guard{processing_};
    // A bounded batch keeps one busy console from monopolizing the host checkpoint.
    for (size_t count = 0; count < 32 && !stop_; ++count) {
        Request request;
        Lattice::NodeId current;
        {
            std::lock_guard lock(mutex_);
            if (requests_.empty()) break;
            request = std::move(requests_.front());
            requests_.pop_front();
            const auto session = std::ranges::find(sessions_, request.session, &Session::id);
            if (session == sessions_.end()) continue;
            current = session->current;
        }
        if (!tree_.contains(current)) current = root_;
        CommandTerminal terminal;
        terminal.setCurrent(current, tree_.path(current));
        terminal.attach();
        if (request.startup) {
            for (std::string_view name : {"logo", "config"})
                if (exports_.resolve(exports_.role(name), current)) commands_.execute(terminal, name);
        } else commands_.execute(terminal, request.line);
        {
            std::lock_guard lock(mutex_);
            const auto session = std::ranges::find(sessions_, request.session, &Session::id);
            if (session == sessions_.end() || stop_) continue; // Removed or stopped during the command.
            session->current = terminal.current();
            --session->pending;
            session->replies.push_back({std::move(terminal.output), terminal.current(), std::move(terminal.path)});
        }
    }
}

void CLI::run(const std::function<void()>& idle) {
    struct Output {
        std::mutex mutex;
        std::deque<std::string> lines;
    };
    // A logger callback already copied by another thread may outlive its registration.
    // It owns only this mailbox, never the CLI or a physical terminal.
    auto output = std::make_shared<Output>();
    const auto writer = LogSystem::setConsoleWriter(
        [output](std::string_view text) {
            std::lock_guard lock(output->mutex);
            output->lines.emplace_back(text);
        });
    struct WriterGuard {
        decltype(writer) token;
        ~WriterGuard() { LogSystem::resetConsoleWriter(token); }
    } writerGuard{writer};
    const auto flushLogs = [&] {
        std::deque<std::string> lines;
        {
            std::lock_guard lock(output->mutex);
            lines.swap(output->lines);
        }
        std::lock_guard lock(mutex_);
        for (auto& session : sessions_)
            for (const auto& line : lines) session.terminal->write(line);
    };
    while (!stop_) {
        // All runtime access is in processCommands(), called by the host's maintain.
        // When CLI itself is the host service, idle() drives that checkpoint too.
        idle();
        flushLogs();
        {
            std::lock_guard lock(mutex_);
            if (!pollTerminals()) break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(16));
    }
    flushLogs();
    std::lock_guard lock(mutex_);
    requests_.clear();
    for (auto& session : sessions_) {
        for (const auto& reply : session.replies) {
            if (!reply.path.empty()) session.terminal->setCurrent(reply.current, reply.path);
            session.terminal->write(reply.text);
        }
        session.replies.clear();
        session.pending = 0;
        session.terminal->detach();
    }
}

bool CLI::pollTerminals() {
    bool active = false;
    for (auto& session : sessions_) {
        auto& terminal = *session.terminal;
        if (!session.started) {
            terminal.attach();
            session.started = true;
            requests_.push_back({session.id, {}, true});
            ++session.pending;
        }
        for (auto& reply : session.replies) {
            if (!reply.path.empty()) terminal.setCurrent(reply.current, std::move(reply.path));
            terminal.write(reply.text);
        }
        session.replies.clear();
        if (!session.closed) {
            auto input = terminal.poll();
            if (input.line) {
                requests_.push_back({session.id, std::move(*input.line)});
                ++session.pending;
            }
            session.closed = input.closed;
        }
        if (session.closed && !session.pending) terminal.detach();
        else active = true; // Drain accepted commands and replies before handling EOF.
    }
    return active;
}

}
