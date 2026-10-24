#include <charconv>
#include <exception>
#include <format>
#include <optional>
#include <string>
#include <vector>

#include "Command.hpp"
#include "ValueParser.hpp"
#include <Logger/Logger.hpp>

namespace Console {
namespace {

std::optional<Lattice::ExportId> parseExportId(std::string_view command) {
    if (command.size() < 2 || command.front() != '#')
        return std::nullopt;

    Lattice::ExportId id = Lattice::InvalidExportId;
    const char* begin = command.data() + 1;
    const char* end = command.data() + command.size();
    const auto [position, error] = std::from_chars(begin, end, id);
    if (error != std::errc{} || position != end)
        return std::nullopt;
    return id;
}

}

void CommandDispatcher::reply(Terminal& terminal, Level level, std::string_view text) const {
    TextFormatter message;
    message.append(text);
    terminal.write(TextFormatter::format(*theme_, "{} <mut><b>[<light>CLI</>]<//> {}</>",
        LogStyle::get(level).style, message.markup()).render() + '\n');
}

void CommandDispatcher::reply(Terminal& terminal, std::string_view text) const {
    reply(terminal, Level::Info, text);
}

void CommandDispatcher::execute(
    Terminal& terminal,
    std::string_view command
) const {
    command = trim(command);
    if (command.empty())
        return;

    const size_t separator = command.find_first_of(" \t");
    std::string_view selector = command.substr(0, separator);
    std::string_view argument = separator == std::string_view::npos
        ? std::string_view{}
        : trim(command.substr(separator + 1));

    const bool reset = selector == "reset";
    if (reset) {
        selector = argument;
        argument = {};
    }

    try {
        Lattice::ResolvedExport resolved;
        std::string_view name = selector;
        if (selector.starts_with('#')) {
            const auto id = parseExportId(selector);
            if (!id) {
                reply(terminal, Level::Warning, std::format("Invalid export id '{}'", selector));
                return;
            }
            resolved = exports_.resolveExport(*id);
            if (resolved)
                name = exports_.name(*id);
        } else {
            resolved = exports_.resolve(exports_.role(selector), terminal.current());
        }

        if (!resolved) {
            reply(terminal, Level::Warning, std::format("Unknown command '{}'", selector));
            return;
        }

        if (reset) {
            if (!resolved.set) {
                reply(terminal, Level::Warning, "reset expects a parameter");
                return;
            }
            exports_.reset(resolved);
            reply(terminal, std::format("Override '{}' removed; current value is unchanged", name));
            return;
        }

        if (resolved.invoke) {
            const auto sourceArguments = parseArguments(argument);
            if (!sourceArguments) {
                reply(terminal, Level::Warning, "Invalid quoted argument");
                return;
            }

            if (
                sourceArguments->size() < resolved.requiredArguments ||
                sourceArguments->size() > resolved.argumentTypes.size()
            ) {
                const std::string expected =
                    resolved.requiredArguments == resolved.argumentTypes.size()
                        ? std::to_string(resolved.requiredArguments)
                        : std::format(
                            "{}-{}",
                            resolved.requiredArguments,
                            resolved.argumentTypes.size()
                        );
                reply(
                    terminal, Level::Warning,
                    std::format(
                        "Action '{}' expects {} argument(s), received {}",
                        name,
                        expected,
                        sourceArguments->size()
                    )
                );
                return;
            }

            std::vector<Lattice::Value> arguments;
            arguments.reserve(sourceArguments->size());

            for (size_t index = 0; index < sourceArguments->size(); ++index) {
                auto value = parseValue((*sourceArguments)[index], resolved.argumentTypes[index]);
                if (!value) {
                    reply(
                        terminal, Level::Warning,
                        std::format("Invalid argument {} for action '{}'", index + 1, name)
                    );
                    return;
                }
                arguments.push_back(std::move(*value));
            }

            const Lattice::NodeId previous = terminal.current();
            Lattice::ActionContext context{previous};
            resolved.invoke(resolved.object, context, arguments);

            if (!tree_.contains(context.node()))
                throw Lattice::Exception<CommandDispatcher>(
                    "Action '{}' selected missing node #{}",
                    name,
                    context.node()
                );

            if (context.node() != previous)
                terminal.setCurrent(context.node(), tree_.path(context.node()));

            std::string output;
            for (const Lattice::ActionOutput& item : context.output()) {
                if (const auto* value = std::get_if<Lattice::Value>(&item)) {
                    output += value->toString();
                    output += '\n';
                    continue;
                }

                const auto& view = std::get<Lattice::ActionView>(item);
                output += std::format("Unsupported action view '{}'\n", view.type());
            }
            if (!output.empty())
                terminal.write(output);
            return;
        }

        if (!resolved.get) {
            reply(terminal, Level::Warning, std::format("Export '{}' is not readable", name));
            return;
        }

        const Lattice::Value current = resolved.get(resolved.object);
        if (argument.empty()) {
            reply(terminal, std::format("{} = {}", name, current.toString()));
            return;
        }

        const auto value = parseValue(argument, current);
        if (!value || !resolved.set) {
            reply(terminal, Level::Warning, std::format("Cannot assign '{}' to '{}'", argument, name));
            return;
        }

        exports_.set(resolved, *value);
        reply(terminal, std::format("{} = {}", name, resolved.get(resolved.object).toString()));
    } catch (const std::exception& error) {
        reply(terminal, Level::Error, std::format("Command failed: {}", error.what()));
    }
}

}
