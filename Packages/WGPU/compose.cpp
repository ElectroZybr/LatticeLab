#include <Lattice/Composition.hpp>
#include <WGPU/Device.hpp>

extern "C" constinit const Lattice::Package lattice_package{
    .id = "WGPU",
    .version = {1, 0, 0},
};

// One GPU device for the application. Consumers mount it and own their resources
// as ordinary RAII objects; the device keeps a registry of what is alive.
extern "C" void compose(NodeBuild root) {
    auto instance = root.emplace<WGPU::Instance>("instance");
    auto device = root.emplaceShared<WGPU::Device>("device", instance);
    device.action("resources", [](WGPU::Device& self, Lattice::ActionContext& context) {
        context.emit(Lattice::Value{self.resources().report()});
    });
}
