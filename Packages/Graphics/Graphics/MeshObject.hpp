#pragma once

#include <array>
#include <cstdint>
#include <memory>
#include <string>

#include <glm/vec3.hpp>
#include <Wgpu/Resources.hpp>

#include "GpuMesh.hpp"
#include "RenderLayout.hpp"
#include "RenderPipeline.hpp"
#include "SceneObject.hpp"

/// A mesh drawn in a scene. `display` is one of the CAD modes: "shaded", "shaded-edges",
/// "hidden-removed", "hidden-visible", "wireframe". The lines are the shape's edges —
/// creases sharper than `creaseAngle`, silhouettes and boundaries — not its triangles
/// (those are `triangles`).
class MeshObject final : public SceneObject {
public:
    MeshObject(RenderLayout& layout, GpuMesh& mesh);
    ~MeshObject() override;

    void render(RenderPass& pass, const ViewState& view) override;
    bool transparent() const override;

    std::string display = "shaded";
    glm::vec3 color{0.7f, 0.72f, 0.75f};
    float opacity = 1.0f;
    float specular = 0.3f;
    float shininess = 32.0f;
    bool cullBack = true;

    glm::vec3 fillColor{0.95f, 0.95f, 0.96f};
    glm::vec3 edgeColor{0.05f, 0.06f, 0.07f};
    float edgeWidth = 1.5f;
    glm::vec3 hiddenColor{0.45f, 0.47f, 0.5f};
    float hiddenWidth = 1.0f;
    float dashLength = 8.0f;
    float creaseAngle = 30.0f;

    bool triangles = false;
    glm::vec3 triangleColor{0.08f, 0.09f, 0.1f};
    float triangleWidth = 1.0f;

private:
    enum class Mode { Shaded, ShadedEdges, HiddenRemoved, HiddenVisible, Wireframe };
    enum class Lines { Visible, Hidden, Always };
    struct Group;
    Mode mode() const;
    RenderPipeline& faces(bool blend);
    RenderPipeline& lines(Lines kind);

    RenderLayout& layout_;
    GpuMesh& mesh_;
    Wgpu::Device& device_;
    Wgpu::BindingLayout groupLayout_;
    Wgpu::Buffer uniform_;
    Wgpu::Buffer noEdges_;
    Wgpu::Shader shader_;
    std::array<std::unique_ptr<RenderPipeline>, 4> faces_;
    std::array<std::unique_ptr<RenderPipeline>, 3> lines_;
    std::unique_ptr<Group> group_;
    uint64_t generation_ = 0;
};
