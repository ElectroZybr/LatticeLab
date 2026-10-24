#pragma once

#include <array>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

#include <glm/vec4.hpp>
#include <WGPU/GpuSoA.hpp>
#include <WGPU/Resources.hpp>

#include "RenderLayout.hpp"
#include "SceneObject.hpp"

/// Columns of a GPU table that make spheres: centers (f32) and an optional kind (u32)
/// indexing the palette. Names are the table's column names.
struct SphereColumns {
    std::string x, y, z;
    std::string kind;   // empty: every sphere uses palette[0]
};

/// Spheres read straight from a GPU table (one row — one sphere), drawn as lit
/// camera-facing impostors with true depth. The renderer knows columns, not models.
class Spheres final : public SceneObject {
public:
    Spheres(RenderLayout& layout, WGPU::GpuSoA& table, SphereColumns columns,
            const std::filesystem::path& shader);
    ~Spheres() override;

    void render(WGPU::RenderPass& pass, const ViewState& view) override;

    /// Per kind: rgb color and radius (w).
    void setPalette(std::vector<glm::vec4> palette);
    /// Spheres drawn by the last render.
    size_t count() const noexcept { return count_; }

private:
    struct Group;
    void rebuild(const WGPU::GpuSoA::View& view);

    WGPU::Device& device_;
    WGPU::GpuSoA& table_;
    SphereColumns columns_;
    WGPU::BindingLayout layout_;
    WGPU::Buffer params_;
    std::unique_ptr<WGPU::Buffer> palette_;
    size_t paletteSize_ = 0;
    WGPU::Shader shader_;
    WGPU::Pipeline pipeline_;
    std::unique_ptr<Group> group_;
    uint64_t generation_ = 0;
    bool paletteChanged_ = true;
    size_t count_ = 0;
};
