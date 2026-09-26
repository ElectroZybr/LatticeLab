#include <exception>
#include <format>
#include <string>
#include <vector>

#include <CLI/include/Command.hpp>
#include <CLI/include/ValueParser.hpp>
#include <Lattice/Tools/Logger.hpp>

namespace CLIPlugin {
namespace {

void reply(Terminal& terminal, Level level, std::string_view text) {
    Text message;
    message.append(text);
    terminal.write(Logger::line(level, "CLI", message).render() + '\n');
}

void reply(Terminal& terminal, std::string_view text) {
    reply(terminal, Level::Info, text);
}

}

CommandResult CommandDispatcher::execute(
    Terminal& terminal,
    std::string_view command
) const {
    command = trim(command);
    if (command.empty())
        return CommandResult::Continue;

    const size_t separator = command.find_first_of(" \t");
    const std::string_view name = command.substr(0, separator);
    const std::string_view argument = separator == std::string_view::npos
        ? std::string_view{}
        : trim(command.substr(separator + 1));

    if (name == "quit" || name == "exit")
        return CommandResult::Detach;

    if (name == "help") {
        reply(terminal, "Commands: help, quit, <action>, <parameter>, <parameter> <value>");
        return CommandResult::Continue;
    }

    try {
        const Lattice::ResolvedExport resolved = exports_.resolve(
            exports_.role(name),
            terminal.current()
        );
        if (!resolved) {
            reply(terminal, Level::Warning, std::format("Unknown command '{}'", name));
            return CommandResult::Continue;
        }

        if (resolved.invoke) {
            const auto sourceArguments = parseArguments(argument);
            if (!sourceArguments) {
                reply(terminal, Level::Warning, "Invalid quoted argument");
                return CommandResult::Continue;
            }

            if (sourceArguments->size() != resolved.argumentTypes.size()) {
                reply(
                    terminal, Level::Warning,
                    std::format(
                        "Action '{}' expects {} argument(s), received {}",
                        name,
                        resolved.argumentTypes.size(),
                        sourceArguments->size()
                    )
                );
                return CommandResult::Continue;
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
                    return CommandResult::Continue;
                }
                arguments.push_back(std::move(*value));
            }

            const Lattice::NodeId previous = terminal.current();
            Lattice::ActionContext context{previous};
            resolved.invoke(resolved.object, context, arguments);

            if (!tree_.contains(context.node()))
                throw Lattice::Exception(
                    "CLI",
                    "Action '{}' selected missing node #{}",
                    name,
                    context.node()
                );

            if (context.node() != previous)
                terminal.setCurrent(context.node(), tree_.path(context.node()));

            std::string output;
            for (const Lattice::Value& value : context.output()) {
                output += value.toString();
                output += '\n';
            }
            if (!output.empty())
                terminal.write(output);
            return CommandResult::Continue;
        }

        if (!resolved.get) {
            reply(terminal, Level::Warning, std::format("Export '{}' is not readable", name));
            return CommandResult::Continue;
        }

        const Lattice::Value current = resolved.get(resolved.object);
        if (argument.empty()) {
            reply(terminal, std::format("{} = {}", name, current.toString()));
            return CommandResult::Continue;
        }

        const auto value = parseValue(argument, current);
        if (!value || !resolved.set) {
            reply(terminal, Level::Warning, std::format("Cannot assign '{}' to '{}'", argument, name));
            return CommandResult::Continue;
        }

        resolved.set(resolved.object, *value);
        reply(terminal, std::format("{} = {}", name, resolved.get(resolved.object).toString()));
    } catch (const std::exception& error) {
        reply(terminal, Level::Error, std::format("Command failed: {}", error.what()));
    }

    return CommandResult::Continue;
}

}
