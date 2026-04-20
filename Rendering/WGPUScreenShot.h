#pragma once

#include <cstdint>
#include <functional>
#include <string>
#include <string_view>
#include <unordered_map>

#include <webgpu/webgpu.hpp>

class WGPUScreenShot {
public:
    using ScreenShotFn = std::function<void(uint32_t width, uint32_t height, const void* data, uint32_t size)>;

    static WGPUScreenShot& instance() {
        static WGPUScreenShot inst;
        return inst;
    }

    void addScreenShotCallback(std::string_view filePath, ScreenShotFn fn) { callbacks_[std::string(filePath)] = std::move(fn); }

    void removeScreenShotCallback(std::string_view filePath) { callbacks_.erase(std::string(filePath)); }

    void capture(wgpu::Device device, wgpu::Texture sourceTexture, uint32_t width, uint32_t height, const std::string& filePath) {
        auto it = callbacks_.find(filePath);
        if (it == callbacks_.end() || !it->second) {
            return;
        }

        const uint32_t bytesPerRow = alignUp(width * 4, 256);
        const uint64_t bufferSize = bytesPerRow * height;

        wgpu::BufferDescriptor bufDesc{};
        bufDesc.size = bufferSize;
        bufDesc.usage = wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::MapRead;
        auto readbackBuf = device.createBuffer(bufDesc);

        wgpu::CommandEncoderDescriptor encDesc{};
        auto encoder = device.createCommandEncoder(encDesc);

        wgpu::TexelCopyTextureInfo src{};
        src.texture = sourceTexture;
        src.mipLevel = 0;
        src.aspect = wgpu::TextureAspect::All;

        wgpu::TexelCopyBufferInfo dst{};
        dst.buffer = readbackBuf;
        dst.layout.bytesPerRow = bytesPerRow;
        dst.layout.rowsPerImage = height;

        wgpu::Extent3D extent{width, height, 1};
        encoder.copyTextureToBuffer(src, dst, extent);

        auto cmd = encoder.finish();
        device.getQueue().submit(1, &cmd);

        struct CbData {
            bool done = false;
        };
        auto cbData = std::make_shared<CbData>();

        wgpu::BufferMapCallbackInfo cbInfo{};
        cbInfo.mode = wgpu::CallbackMode::AllowProcessEvents;
        cbInfo.callback = [](WGPUMapAsyncStatus, WGPUStringView, void* ud, void*) { static_cast<CbData*>(ud)->done = true; };
        cbInfo.userdata1 = cbData.get();

        readbackBuf.mapAsync(wgpu::MapMode::Read, 0, bufferSize, cbInfo);

        while (!cbData->done) {
            device.poll(false, nullptr);
        }

        const void* data = readbackBuf.getConstMappedRange(0, bufferSize);
        it->second(width, height, data, static_cast<uint32_t>(bufferSize));
        readbackBuf.unmap();
    }

private:
    WGPUScreenShot() = default;

    std::unordered_map<std::string, ScreenShotFn> callbacks_;

    static uint32_t alignUp(uint32_t value, uint32_t alignment) { return (value + alignment - 1) & ~(alignment - 1); }
};