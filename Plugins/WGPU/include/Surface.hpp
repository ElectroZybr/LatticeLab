#pragma once
#include <memory>
#include <Lattice/Kernel/Node.hpp>
#include <Graphics/include/Surface.hpp>
#include <Graphics/include/RenderPipeline.hpp>
#include <webgpu/webgpu.h>

namespace WGPU {
class Surface final : public Graphics::Surface {
public:
    Surface(Lattice::Node&, const Desc&);
    ~Surface() override;
    GPU::TextureFormat format() const override;
    void resize(uint32_t, uint32_t) override;
    bool acquire() override;
    void present() override;
    void releaseFrame() override;
    WGPUTextureView view() const;
    WGPUDevice device() const;
private:
    struct State;
    std::unique_ptr<State> state_;
};

class RenderPipeline final : public Graphics::RenderPipeline {
public:
    RenderPipeline(Lattice::Node&, const Desc&);
    ~RenderPipeline() override;
    WGPURenderPipeline native() const { return pipeline_; }
    WGPUDevice device() const { return device_; }
private:
    WGPURenderPipeline pipeline_ = nullptr;
    WGPUDevice device_ = nullptr;
};
}
