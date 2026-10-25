#include "Spheres.hpp"

#include <fstream>
#include <iterator>
#include <span>

#include <Lattice/tools/Exception.hpp>

namespace {

using Error = Lattice::Exception<Spheres>;

constexpr uint32_t NoColumn = 0xFFFFFFFFu;

// Mirrors `Params` in spheres3d.wgsl: word offsets of the columns in the table.
struct Params {
    uint32_t x, y, z, kind;
    uint32_t paletteSize;
    uint32_t pad[3];
};

constexpr std::array<Wgpu::LayoutEntry, 3> groupEntries{{
    {.binding = 0, .type = Wgpu::BindingType::ReadOnlyStorage, .visibility = Wgpu::ShaderStage::Vertex},
    {.binding = 1, .type = Wgpu::BindingType::Uniform, .visibility = Wgpu::ShaderStage::Vertex},
    {.binding = 2, .type = Wgpu::BindingType::ReadOnlyStorage, .visibility = Wgpu::ShaderStage::Vertex},
}};

std::string readShader(const std::filesystem::path& path) {
    std::ifstream file(path);
    if (!file)
        throw Error("cannot read shader '{}'", path.string());
    return {std::istreambuf_iterator<char>(file), {}};
}

/// Word offset of a 32-bit column; NoColumn for an empty name.
uint32_t wordOffset(const Wgpu::GpuSoA::View& view, const std::string& name, bool integer) {
    if (name.empty())
        return NoColumn;
    std::string names;
    for (const auto& column : view.columns) {
        if (column.name == name) {
            const bool matches = integer ? column.type.is<uint32_t>() : column.type.is<float>();
            if (!matches || column.offset % 4)
                throw Error("column '{}' is not a word-aligned {}", name, integer ? "u32" : "f32");
            return uint32_t(column.offset / 4);
        }
        names += (names.empty() ? "" : ", ") + column.name;
    }
    throw Error("table has no column '{}'; columns: [{}]", name, names);
}

}

/// Group 1 over the current table buffer: recreated when the table's buffer changes.
struct Spheres::Group {
    Group(Wgpu::Device& device, Wgpu::BindingLayout& layout, std::span<const Wgpu::Binding> bindings)
        : set(device, layout, bindings) {}
    Wgpu::BindingSet set;
};

Spheres::Spheres(RenderLayout& layout, Wgpu::GpuSoA& table, SphereColumns columns,
                 const std::filesystem::path& shader)
    : device_(layout.device()),
      table_(table),
      columns_(std::move(columns)),
      layout_(device_, groupEntries, "spheres"),
      params_(device_, {.size = sizeof(Params),
          .usage = Wgpu::BufferUsage::Uniform | Wgpu::BufferUsage::CopyDestination, .label = "spheres/params"}),
      shader_(device_, {.source = readShader(shader), .label = shader.filename().string()}),
      pipeline_(device_, shader_, {.colorFormat = layout.colorFormat(), .depthFormat = layout.depthFormat(),
          .layouts = {&layout.camera(), &layout_}, .label = "spheres"}) {
    setPalette({{0.85f, 0.85f, 0.9f, 0.5f}});
}

Spheres::~Spheres() = default;

void Spheres::setPalette(std::vector<glm::vec4> palette) {
    if (palette.empty())
        throw Error("palette needs at least one kind");
    if (!palette_ || palette_->size() < palette.size() * sizeof(glm::vec4)) {
        palette_ = std::make_unique<Wgpu::Buffer>(device_, Wgpu::BufferDesc{
            .size = palette.size() * sizeof(glm::vec4),
            .usage = Wgpu::BufferUsage::Storage | Wgpu::BufferUsage::CopyDestination, .label = "spheres/palette"});
        group_.reset();
    }
    device_.writeBuffer(*palette_, 0, std::as_bytes(std::span(palette)));
    paletteSize_ = palette.size();
    paletteChanged_ = true;
}

void Spheres::rebuild(const Wgpu::GpuSoA::View& view) {
    const Params params{
        .x = wordOffset(view, columns_.x, false),
        .y = wordOffset(view, columns_.y, false),
        .z = wordOffset(view, columns_.z, false),
        .kind = wordOffset(view, columns_.kind, true),
        .paletteSize = uint32_t(paletteSize_),
        .pad = {},
    };
    device_.writeBuffer(params_, 0, std::as_bytes(std::span(&params, 1)));
    const std::array bindings{
        Wgpu::Binding{.binding = 0, .buffer = &view.buffer},
        Wgpu::Binding{.binding = 1, .buffer = &params_},
        Wgpu::Binding{.binding = 2, .buffer = palette_.get()},
    };
    group_.reset();
    group_ = std::make_unique<Group>(device_, layout_, bindings);
    generation_ = view.generation;
    paletteChanged_ = false;
}

void Spheres::render(RenderPass& pass, const ViewState& view) {
    // The buffer and layout of the table are taken under its lock, as one snapshot.
    table_.read([&](const Wgpu::GpuSoA::View& table) {
        if (!group_ || table.generation != generation_ || paletteChanged_)
            rebuild(table);
        count_ = table.rows;
    });
    if (!count_)
        return;
    pass.setPipeline(pipeline_);
    pass.setBindings(0, view.cameraGroup);
    pass.setBindings(1, group_->set);
    pass.draw(6, uint32_t(count_));
}
