#pragma once

#include <cstdint>
#include <memory>
#include <vector>

#include <glm/vec4.hpp>
#include <Boundaries/Space.hpp>
#include <BoundariesWgpu/Codegen.hpp>
#include <BoundariesWgpu/GpuGeometry.hpp>
#include <Wgpu/Resources.hpp>

#include "RenderLayout.hpp"
#include "RenderPipeline.hpp"
#include "SceneObject.hpp"

/// Surface of a geometry program (Boundaries) drawn by sphere tracing inside the box of
/// its space: rays march the same WGSL machine the walls run, hits are lit and write
/// their true depth, so particles and walls overlap correctly. The renderer knows the
/// program and the box, not what the region holds. Marching uses `distance`: code
/// generated for the program's structure (the shader is rebuilt when it changes) or the
/// interpreting machine.
class GeometrySurface final : public SceneObject {
public:
    GeometrySurface(RenderLayout& layout, const Boundaries::Wgpu::GpuGeometry& geometry,
                    const Boundaries::Space& space,
                    Boundaries::Wgpu::DistanceSource distance = Boundaries::Wgpu::generateDistance);
    ~GeometrySurface() override;

    void render(RenderPass& pass, const ViewState& view) override;

    glm::vec4 regionColor{0.55f, 0.65f, 0.75f, 1.0f};   ///< the surface seen from the region
    glm::vec4 wallColor{0.35f, 0.33f, 0.3f, 1.0f};      ///< the surface seen from the wall
    /// Rays pass the surface seen from the wall: from outside, the container is cut away
    /// and its inner side stays visible.
    bool cullWallSide = true;
    uint32_t maxSteps = 128;

private:
    struct Group;
    void build();

    const Boundaries::Wgpu::GpuGeometry& geometry_;
    const Boundaries::Space& space_;
    Boundaries::Wgpu::DistanceSource distance_;
    RenderLayout& renderLayout_;
    Wgpu::Device& device_;
    Wgpu::BindingLayout layout_;
    Wgpu::Buffer params_;
    std::vector<Boundaries::Op> structure_;   // the program the pipeline was built for
    std::unique_ptr<Wgpu::Shader> shader_;
    std::unique_ptr<RenderPipeline> pipeline_;
    std::unique_ptr<Group> group_;
    uint64_t generation_ = 0;
};
