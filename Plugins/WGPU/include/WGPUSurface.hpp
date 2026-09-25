#pragma once

#include <memory>
#include <webgpu/webgpu.h>

#include "Surface.hpp"
#include "Pipeline.hpp"

class NodeBuild;

namespace WGPU {

class Surface final : public GPU::Surface {
public:
    Surface(NodeBuild node, const Desc& desc);
    ~Surface() override;

    GPU::TextureFormat format() const override;
    void resize(uint32_t width, uint32_t height) override;
    bool acquire() override;
    void present() override;
    void releaseFrame() override;

    WGPUTextureView view() const noexcept;
    WGPUDevice device() const noexcept;

private:
    struct State;
    std::unique_ptr<State> state_;
};

class Pipeline final : public GPU::Pipeline {
public:
    Pipeline(NodeBuild node, const Desc& desc);
    ~Pipeline() override;

    WGPURenderPipeline native() const noexcept;
    WGPUDevice device() const noexcept;

private:
    WGPURenderPipeline pipeline_ = nullptr;
    WGPUDevice device_ = nullptr;
};

}
