#include <Lattice/Composition.hpp>
#include <Serialization/Serializer.hpp>

extern "C" constinit const Lattice::Package lattice_package{
    .id = "Serialization",
    .name = "Runtime configuration serialization",
    .version = {1, 0, 0},
    .dependencies = {{.id = "StdIo", .min = {1, 0, 0}}},
};

extern "C" void compose(NodeBuild root) {
    auto mountedIO = root.mount<IOSubsystem>();
    auto io = mountedIO.ref();
    auto& nodes = *io.runtime();
    auto serializer = root.emplace<Serialization::Serializer>("config", nodes,
        nodes.ops.root(root.id()), io);

    // Startup calls load on the composition root. Forward through Exports so a
    // retired/deleted service rejects the call instead of using a stale pointer.
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
}
