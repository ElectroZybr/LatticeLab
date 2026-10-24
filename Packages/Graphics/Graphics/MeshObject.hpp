#pragma once

#include <array>
#include <cstdint>
#include <memory>

#include <glm/vec3.hpp>
#include <Wgpu/Resources.hpp>

#include "GpuMesh.hpp"
#include "RenderLayout.hpp"
#include "RenderPipeline.hpp"
#include "SceneObject.hpp"

/// A mesh drawn in a scene: its transform, a material lit by the viewport's lights and
/// display modes — solid, wireframe over it or alone, transparency. The mesh is shared
/// data (GpuMesh); several objects may draw one mesh with different transforms.
class MeshObject final : public SceneObject {
public:
    MeshObject(RenderLayout& layout, GpuMesh& mesh);
    ~MeshObject() override;

    void render(RenderPass& pass, const ViewState& view) override;
    bool transparent() const override { return opacity < 1.0f; }

    glm::vec3 color{0.7f, 0.72f, 0.75f};
    float opacity = 1.0f;
    float specular = 0.3f;
    float shininess = 32.0f;
    /// Faces drawn lit; off with the wireframe on — the wireframe alone.
    bool solid = true;
    bool wireframe = false;
    glm::vec3 wireColor{0.08f, 0.09f, 0.1f};
    float wireWidth = 1.0f;   ///< pixels
    /// Back faces discarded (closed meshes); off — both sides drawn and lit.
    bool cullBack = true;

private:
    struct Group;
    RenderPipeline& pipeline();

    RenderLayout& layout_;
    GpuMesh& mesh_;
    Wgpu::Device& device_;
    Wgpu::BindingLayout groupLayout_;
    Wgpu::Buffer uniform_;
    Wgpu::Shader shader_;
    /// By (transparent, culled): built on first use.
    std::array<std::unique_ptr<RenderPipeline>, 4> pipelines_;
    std::unique_ptr<Group> group_;
    uint64_t generation_ = 0;
};
