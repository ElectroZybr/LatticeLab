#pragma once

#include <array>
#include <vector>

#include <glm/glm.hpp>

#include "Rendering/BaseRenderer.h"

class RendererWGPU : public IRenderer {
public:
    RendererWGPU(SimBox& simbox, wgpu::Device device, wgpu::TextureFormat surfaceFormat);
    ~RendererWGPU() override = default;

    void drawShot(wgpu::TextureView targetView, wgpu::TextureView depthView, const AtomStorage& atoms, const Bond::List& bonds,
                  const SimBox& box) override;
    void endFrame() override;

    wgpu::RenderPassEncoder getCurrentPass() { return currentPass; }

protected:
    virtual void updateMatrices() = 0;
    virtual glm::vec3 getLightDir() = 0;
    virtual bool useLighting() = 0;

    glm::mat4 projection{1.f};
    glm::mat4 view{1.f};

    wgpu::Device device = nullptr;

    void initAtomPipeline(const char* atomWGSL);
    void initGridPipeline(const char* gridWGSL);
    void initBoxPipeline(const char* boxWGSL);
    void initBondPipeline(const char* bondWGSL);

private:
    struct SceneUniforms {
        glm::mat4 view;
        glm::mat4 projection;
        glm::vec4 lightDir;
        glm::vec4 colorMode;   // x = SpeedColorMode
        glm::vec4 maxSpeedSqr; // x = value
        glm::vec4 maxCount;    // x = value
        glm::vec4 typeColors[119];
    };
    wgpu::Buffer uniformBuffer = nullptr;

    // Pipelines (аналог bgfx::ProgramHandle)
    wgpu::RenderPipeline atomPipeline = nullptr;
    wgpu::RenderPipeline bondPipeline = nullptr;
    wgpu::RenderPipeline boxPipeline = nullptr;
    wgpu::RenderPipeline gridPipeline = nullptr;

    // Bind group layouts
    wgpu::BindGroupLayout atomBindGroupLayout = nullptr;
    wgpu::BindGroupLayout lineBindGroupLayout = nullptr;
    wgpu::BindGroupLayout gridBindGroupLayout = nullptr;
    wgpu::BindGroup lineBindGroup = nullptr;
    wgpu::BindGroup gridBindGroup = nullptr;

    // Vertex buffers
    wgpu::Buffer atomQuadVb = nullptr;
    wgpu::Buffer bondVb = nullptr;
    wgpu::Buffer boxVb = nullptr;
    wgpu::Buffer gridLineVb = nullptr;
    wgpu::Buffer gridInstVb = nullptr;

    // Storage buffers
    wgpu::Buffer sbPos = nullptr;    // array<vec4<f32>> — x,y,z,pad
    wgpu::Buffer sbVel = nullptr;    // array<vec4<f32>> — vx,vy,vz,pad
    wgpu::Buffer sbType = nullptr;   // array<f32>
    wgpu::Buffer sbRadius = nullptr; // array<f32>
    wgpu::Buffer sbSel = nullptr;    // array<f32>

    size_t sbCapacity_ = 0;
    size_t bondVbCapacity_ = 0;
    size_t gridInstVbCapacity_ = 0;

    wgpu::BindGroup atomBindGroup = nullptr;

    // Текущий render pass (живёт в течение drawShot)
    wgpu::RenderPassEncoder currentPass = nullptr;
    wgpu::TextureView currentTargetView = nullptr;
    wgpu::TextureView currentDepthView = nullptr;

    wgpu::TextureFormat surfaceFormat;

    void initAtomColors();
    void initAtomQuadBuffer();
    void initBoxBuffer();
    void initBondBuffer();
    void initGridLineBuffer();
    void initLinePipeline(wgpu::RenderPipeline& outPipeline, const char* wgsl);

    // Helpers
    wgpu::Buffer createBuffer(uint64_t size, wgpu::BufferUsage usage, wgpu::StringView label = wgpu::StringView());
    void ensureStorageBuffers(size_t count);
    template <typename T> void uploadStorageBuffer(wgpu::Buffer& buf, const T* data, size_t count);

    // Draw
    void drawAtomsImpl(const AtomStorage& atoms);
    void drawBondsImpl(const AtomStorage& atoms, const Bond::List& bonds);
    void drawBoxImpl(const SimBox& box);
    void drawGridImpl(const SpatialGrid& grid);

    // Data
    struct GridInstance {
        glm::vec4 origin;
        float cellSize;
        float atomCount;
        float pad[2] = {};
    };

    struct AtomVec4 {
        float x, y, z, pad = 0.f;
    };
    std::vector<AtomVec4> posData_;
    std::vector<AtomVec4> velData_;

    std::vector<GridInstance> gridData;
    std::vector<glm::vec4> typeColorsData;
    std::vector<float> selectedData;
    std::vector<float> radii;
    std::vector<float> typeData;
    std::array<float, 24 * 3> boxVertices_{};
    Vec3f cachedBoxSize_{-1.0, -1.0, -1.0};

    wgpu::CommandEncoder currentEncoder = nullptr;
};
