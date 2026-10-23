#include <optional>
#include <string>

#include <Lattice/Composition.hpp>
#include <Serialization/Serializer.hpp>
#include <StdIo/IOSubsystem.hpp>
#include <StdIo/YamlParser.hpp>
#include <StdIo/TomlParser.hpp>
#include <StdIo/TsvParser.hpp>

extern "C" constinit const Lattice::Package lattice_package{
    .id = "StdIo",
    .name = "Standard input/output",
    .version = {1, 0, 0},
};

// Parsers, the shared IO and the runtime configuration serializer built on it.
extern "C" void compose(NodeBuild root) {
    root.emplace<YamlParser, ParserAPI>("yaml");
    root.emplace<TomlParser, ParserAPI>("toml");
    root.emplace<TsvParser, ParserAPI>("tsv");
    auto io = root.emplaceShared<IOSubsystem>("io", root.children<ParserAPI>());

    auto& nodes = *io.ref().runtime();
    auto serializer = root.emplace<Serialization::Serializer>("config", nodes,
        nodes.ops.root(root.id()), io);

    // Startup calls load on the composition root. Forward through Exports so a
    // retired serializer rejects the call instead of using a stale pointer.
    const auto forward = [&](std::string_view name, Lattice::ExportId target) {
        root.action<std::optional<std::string>>(name,
            [exports = &nodes.exports, target](Lattice::ActionContext& context, std::optional<std::string> path) {
                if (path) {
                    const Lattice::Value argument{*path};
                    exports->invoke(target, context, {&argument, 1});
                } else exports->invoke(target, context);
            });
    };
    forward("load", serializer.action("load", &Serialization::Serializer::load));
    forward("save", serializer.action("save", &Serialization::Serializer::save));
    const auto status = serializer.action("status", [](Serialization::Serializer& self, Lattice::ActionContext& context) {
        context.emit(self.status());
    });
    root.globalAction("config", [exports = &nodes.exports, status](Lattice::ActionContext& context) {
        exports->invoke(status, context);
    });
}
