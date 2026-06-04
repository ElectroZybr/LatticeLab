#include "Render.h"

#include <algorithm>
#include <ranges>

#include "Lattice/Engine/physics/AtomData.h"
#include "Rendering/backend/WGPUContext.h"

void RendererWGPU::ensureStorageBuffers(size_t count) {
    if (count <= sbCapacity_) {
        return;
    }

    const uint64_t vec4Bytes = count * sizeof(AtomVec4);
    const uint64_t f32Bytes = count * sizeof(float);
    const wgpu::BufferUsage usage = wgpu::BufferUsage::Storage | wgpu::BufferUsage::CopyDst;

    sbPos = WGPUContext::instance().createBuffer(vec4Bytes, usage, "Atoms_Pos");
    sbVel = WGPUContext::instance().createBuffer(vec4Bytes, usage, "Atoms_Vel");
    sbType = WGPUContext::instance().createBuffer(f32Bytes, usage, "Atoms_Type");
    sbRadius = WGPUContext::instance().createBuffer(f32Bytes, usage, "Atoms_Radius");
    sbSel = WGPUContext::instance().createBuffer(f32Bytes, usage, "Atoms_Selection");
    sbCapacity_ = count;

    std::array<wgpu::BindGroupEntry, 6> entries{};
    entries[0].binding = 0;
    entries[0].buffer = *uniformBuffer;
    entries[0].size = sizeof(SceneUniforms);
    entries[1].binding = 1;
    entries[1].buffer = *sbPos;
    entries[1].size = vec4Bytes;
    entries[2].binding = 2;
    entries[2].buffer = *sbVel;
    entries[2].size = vec4Bytes;
    entries[3].binding = 3;
    entries[3].buffer = *sbType;
    entries[3].size = f32Bytes;
    entries[4].binding = 4;
    entries[4].buffer = *sbRadius;
    entries[4].size = f32Bytes;
    entries[5].binding = 5;
    entries[5].buffer = *sbSel;
    entries[5].size = f32Bytes;

    atomBindGroup = WGPUContext::instance().createBindGroup(*atomBindGroupLayout, entries, "AtomBindGroup");
}

template <typename T> void RendererWGPU::uploadStorageBuffer(wgpu::Buffer& buf, const T* data, size_t count) {
    WGPUContext::instance().queue()->writeBuffer(buf, 0, data, count * sizeof(T));
}

void RendererWGPU::drawAtomsImpl(const RenderAtomsView& atoms, const RenderData& renderData, bool applySelection) {
    const size_t count = atoms.count;
    if (count == 0 || !atoms.hasPositions() || !atoms.hasTypes()) {
        return;
    }

    ensureStorageBuffers(count);

    posData_.resize(count);
    velData_.resize(count);
    radii.resize(count);
    typeData.resize(count);
    selectedData.assign(count, 0.0f);

    for (size_t i = 0; i < count; ++i) {
        posData_[i] = {atoms.x[i], atoms.y[i], atoms.z[i]};
        velData_[i] = atoms.hasVelocities() ? AtomVec4{atoms.vx[i], atoms.vy[i], atoms.vz[i]} : AtomVec4{};
        const AtomData::Type atomType = static_cast<AtomData::Type>(atoms.type[i]);
        radii[i] = atoms.hasRadii() ? atoms.radius[i] : AtomData::getProps(atomType).radius;
        typeData[i] = static_cast<float>(atomType);
    }
    if (applySelection) {
        for (const size_t idx : renderData.selectedAtomIndices) {
            if (idx < count) {
                selectedData[idx] = 1.0f;
            }
        }
    }

    uploadStorageBuffer(*sbPos, posData_.data(), count);
    uploadStorageBuffer(*sbVel, velData_.data(), count);
    uploadStorageBuffer(*sbRadius, radii.data(), count);
    uploadStorageBuffer(*sbType, typeData.data(), count);
    uploadStorageBuffer(*sbSel, selectedData.data(), count);

    float maxSpeedSqr = 1.f;
    if (renderData.speedColorMode != RenderData::SpeedColorMode::AtomColor) {
        if (renderData.speedGradientMax > 0.f) {
            maxSpeedSqr = renderData.speedGradientMax * renderData.speedGradientMax;
        }
        else if (atoms.hasVelocities()) {
            const auto it = std::ranges::max_element(std::views::iota(size_t{0}, count), {}, [&](size_t i) {
                return atoms.vx[i] * atoms.vx[i] + atoms.vy[i] * atoms.vy[i] + atoms.vz[i] * atoms.vz[i];
            });
            const float speedSqr = atoms.vx[*it] * atoms.vx[*it] + atoms.vy[*it] * atoms.vy[*it] + atoms.vz[*it] * atoms.vz[*it];
            maxSpeedSqr = std::max(1e-6f, speedSqr);
        }
    }
    WGPUContext::instance().queue()->writeBuffer(*uniformBuffer, offsetof(SceneUniforms, maxSpeedSqr), &maxSpeedSqr, sizeof(float));

    currentPass->setPipeline(*atomPipeline);
    currentPass->setBindGroup(0, *atomBindGroup, 0, nullptr);
    currentPass->setVertexBuffer(0, *atomQuadVb, 0, atomQuadVb->getSize());
    currentPass->draw(6, count, 0, 0);
}
