#pragma once
#include <memory>
#include <Lattice/Kernel/Node.hpp>
#include <GPU/include/Surface.hpp>
#include <GPU/include/Pipeline.hpp>
#include <webgpu/webgpu.h>

namespace WGPU {
class Surface final : public GPU::Surface {
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

class Pipeline final : public GPU::Pipeline {
public:
    Pipeline(Lattice::Node&, const Desc&);
    ~Pipeline() override;
    WGPURenderPipeline native() const { return pipeline_; }
    WGPUDevice device() const { return device_; }
private:
    WGPURenderPipeline pipeline_ = nullptr;
    WGPUDevice device_ = nullptr;
};
}
