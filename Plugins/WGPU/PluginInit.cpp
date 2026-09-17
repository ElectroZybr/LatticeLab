// Kernel dependences
#include <Lattice/Kernel/Node.hpp>

#include "Device.hpp"
#include "WGPU.hpp"
#include "WGPUDevice.hpp"


extern "C" bool plugin_register(Lattice::Node& blueprints) {
    blueprints.blueprint<WGPU::WGPU, GPU::GPUAPI>();
    blueprints.blueprint<WGPU::WDevice, GPU::Device>();
    return true;
}

extern "C" void plugin_shutdown() {}
