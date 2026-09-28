#include "WGPUTexture.hpp"

#include "WGPUDevice.hpp"
#include <Lattice/Tools/Exception.hpp>
#include <Lattice/Kernel/NodeViews.hpp>

namespace WGPU {

namespace {

constexpr WGPUTextureFormat nativeFormat(GPU::TextureFormat format) {
    switch (format) {
        case GPU::TextureFormat::R8Unorm:             return WGPUTextureFormat_R8Unorm;
        case GPU::TextureFormat::RG8Unorm:            return WGPUTextureFormat_RG8Unorm;
        case GPU::TextureFormat::RGBA8Unorm:          return WGPUTextureFormat_RGBA8Unorm;
        case GPU::TextureFormat::BGRA8Unorm:          return WGPUTextureFormat_BGRA8Unorm;
        case GPU::TextureFormat::R16Float:            return WGPUTextureFormat_R16Float;
        case GPU::TextureFormat::RG16Float:           return WGPUTextureFormat_RG16Float;
        case GPU::TextureFormat::RGBA16Float:         return WGPUTextureFormat_RGBA16Float;
        case GPU::TextureFormat::R32Float:            return WGPUTextureFormat_R32Float;
        case GPU::TextureFormat::RG32Float:           return WGPUTextureFormat_RG32Float;
        case GPU::TextureFormat::RGBA32Float:         return WGPUTextureFormat_RGBA32Float;
        case GPU::TextureFormat::Depth16Unorm:        return WGPUTextureFormat_Depth16Unorm;
        case GPU::TextureFormat::Depth24Plus:         return WGPUTextureFormat_Depth24Plus;
        case GPU::TextureFormat::Depth24PlusStencil8: return WGPUTextureFormat_Depth24PlusStencil8;
        case GPU::TextureFormat::Depth32Float:        return WGPUTextureFormat_Depth32Float;
        default:                                      return WGPUTextureFormat_Undefined;
    }
}

}

Texture::Texture(NodeBuild node, const Desc& desc) {
    const auto device = node.ancestor<Device>();

    WGPUTextureDescriptor nativeDesc{};
    nativeDesc.size = {desc.width, desc.height, desc.depth};
    nativeDesc.mipLevelCount = desc.mipLevels;
    nativeDesc.sampleCount = 1;
    nativeDesc.dimension = WGPUTextureDimension_2D;
    nativeDesc.format = nativeFormat(desc.format);
    nativeDesc.usage = static_cast<WGPUTextureUsage>(desc.usage);

    texture_ = wgpuDeviceCreateTexture(device->native(), &nativeDesc);
    if (!texture_)
        throw Lattice::Exception("WGPU::Texture", "failed to create texture");
}

Texture::~Texture() {
    if (texture_)
        wgpuTextureRelease(texture_);
}

WGPUTexture Texture::native() const noexcept {
    return texture_;
}

}
