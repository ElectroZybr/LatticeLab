// Kernel dependences
#include "Lattice/Kernel/Node.hpp"
#include "Lattice/Kernel/SubsystemAPI.hpp"

// Sources
#include "GPUAPI.hpp"
#include "Device.hpp"
#include "Buffer.hpp"
#include "Queue.hpp"
#include "Shader.hpp"
#include "Texture.hpp"
#include "CommandList.hpp"

namespace GPU {

extern "C" bool plugin_register(Lattice::Node& blueprints) {
    Lattice::Node& GPUFolder = blueprints.addFolder("GPU");
    GPUFolder.blueprint<GPUAPI, SubsystemAPI>();
    GPUFolder.blueprint<Device>();
    GPUFolder.blueprint<Buffer>();
    GPUFolder.blueprint<Queue>();
    GPUFolder.blueprint<Shader>();
    GPUFolder.blueprint<Texture>();
    GPUFolder.blueprint<CommandList>();
    return true;
}

extern "C" void plugin_shutdown() {}

} // GPU