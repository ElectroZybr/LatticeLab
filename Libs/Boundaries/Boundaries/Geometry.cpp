#include "Geometry.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <vector>

#include <Lattice/Tools/Exception.hpp>

namespace Boundaries {

using Error = Lattice::Exception<Geometry>;

namespace {

constexpr size_t Block = 64;

// Signed distances of the region's surface: negative inside the region (the usual
// SDF convention); the public distance is the negation (positive where particles may be).

float sphere(glm::vec3 p, const glm::vec4* q) { return glm::length(p) - q[0].x; }

float box(glm::vec3 p, const glm::vec4* q) {
    const glm::vec3 d = glm::abs(p) - glm::vec3(q[0]);
    return glm::length(glm::max(d, glm::vec3(0.0f))) + std::min(std::max(d.x, std::max(d.y, d.z)), 0.0f);
}

float capsule(glm::vec3 p, const glm::vec4* q) {
    const glm::vec3 a(q[0]), b(q[1]);
    const glm::vec3 pa = p - a, ba = b - a;
    const float length = glm::dot(ba, ba);
    const float h = length > 0.0f ? std::clamp(glm::dot(pa, ba) / length, 0.0f, 1.0f) : 0.0f;
    return glm::length(pa - ba * h) - q[1].w;
}

float cylinder(glm::vec3 p, const glm::vec4* q) {
    const glm::vec2 d = glm::abs(glm::vec2(glm::length(glm::vec2(p.x, p.z)), p.y)) - glm::vec2(q[0].x, q[0].y);
    return std::min(std::max(d.x, d.y), 0.0f) + glm::length(glm::max(d, glm::vec2(0.0f)));
}

float halfSpace(glm::vec3 p, const glm::vec4* q) { return glm::dot(glm::vec3(q[0]), p) - q[0].w; }

float infiniteCylinder(glm::vec3 p, const glm::vec4* q) { return glm::length(glm::vec2(p.x, p.z)) - q[0].x; }

template<class Distance>
void evaluatePrimitive(const Instruction& instruction, Distance distance, size_t count,
                       const float* x, const float* y, const float* z, float* out) {
    const glm::vec4 r0 = instruction.local[0], r1 = instruction.local[1], r2 = instruction.local[2];
    for (size_t i = 0; i < count; ++i) {
        const glm::vec4 p(x[i], y[i], z[i], 1.0f);
        const glm::vec3 local(glm::dot(r0, p), glm::dot(r1, p), glm::dot(r2, p));
        out[i] = instruction.scale * distance(local, instruction.params);
    }
}

}

Geometry::Geometry(Program program) : program_(std::move(program)) {
    if (program_.code.empty() || program_.depth == 0)
        throw Error("geometry program is empty");
}

void Geometry::evaluate(std::span<const float> x, std::span<const float> y, std::span<const float> z,
                        std::span<float> distance, std::span<uint32_t> material) const {
    if (!material.empty() && material.size() != distance.size())
        throw Error("evaluate: material column differs in length");
    run(x, y, z, distance, material);
    for (uint32_t& value : material)
        value = program_.code[value].material;
}

void Geometry::run(std::span<const float> x, std::span<const float> y, std::span<const float> z,
                   std::span<float> distance, std::span<uint32_t> primitive) const {
    const size_t count = x.size();
    if (y.size() != count || z.size() != count || distance.size() != count)
        throw Error("evaluate: columns differ in length");

    // Stack of the block: depth × Block distances and owning primitives. Typical
    // programs fit the fixed buffer; deeper ones take the heap once per call.
    constexpr size_t FixedDepth = 16;
    std::array<float, FixedDepth * Block> fixedStack;
    std::array<uint32_t, FixedDepth * Block> fixedOwners;
    std::vector<float> heapStack;
    std::vector<uint32_t> heapOwners;
    std::span<float> stack(fixedStack);
    std::span<uint32_t> owners(fixedOwners);
    if (program_.depth > FixedDepth) {
        heapStack.resize(program_.depth * Block);
        heapOwners.resize(program_.depth * Block);
        stack = heapStack;
        owners = heapOwners;
    }

    for (size_t begin = 0; begin < count; begin += Block) {
        const size_t n = std::min(Block, count - begin);
        const float *px = x.data() + begin, *py = y.data() + begin, *pz = z.data() + begin;
        size_t top = 0;
        for (uint32_t index = 0; index < program_.code.size(); ++index) {
            const auto& instruction = program_.code[index];
            float* slot = stack.data() + top * Block;
            uint32_t* owner = owners.data() + top * Block;
            switch (instruction.op) {
                case Op::Sphere:           evaluatePrimitive(instruction, sphere, n, px, py, pz, slot); break;
                case Op::Box:              evaluatePrimitive(instruction, box, n, px, py, pz, slot); break;
                case Op::Capsule:          evaluatePrimitive(instruction, capsule, n, px, py, pz, slot); break;
                case Op::Cylinder:         evaluatePrimitive(instruction, cylinder, n, px, py, pz, slot); break;
                case Op::HalfSpace:        evaluatePrimitive(instruction, halfSpace, n, px, py, pz, slot); break;
                case Op::InfiniteCylinder: evaluatePrimitive(instruction, infiniteCylinder, n, px, py, pz, slot); break;
                default: break;
            }
            if (isPrimitive(instruction.op)) {
                std::fill_n(owner, n, index);
                ++top;
                continue;
            }
            if (isBinary(instruction.op)) {
                float* a = stack.data() + (top - 2) * Block;
                const float* b = stack.data() + (top - 1) * Block;
                uint32_t* ma = owners.data() + (top - 2) * Block;
                const uint32_t* mb = owners.data() + (top - 1) * Block;
                const float k = instruction.params[0].x;
                for (size_t i = 0; i < n; ++i) {
                    float value = a[i];
                    bool second = false;
                    switch (instruction.op) {
                        case Op::Union:     second = b[i] < a[i]; value = second ? b[i] : a[i]; break;
                        case Op::Intersect: second = b[i] > a[i]; value = second ? b[i] : a[i]; break;
                        case Op::Subtract:  second = -b[i] > a[i]; value = second ? -b[i] : a[i]; break;
                        case Op::SmoothUnion: {
                            const float h = std::max(k - std::abs(a[i] - b[i]), 0.0f) / k;
                            second = b[i] < a[i];
                            value = std::min(a[i], b[i]) - h * h * k * 0.25f;
                            break;
                        }
                        default: break;
                    }
                    a[i] = value;
                    if (second)
                        ma[i] = mb[i];
                }
                --top;
                continue;
            }
            float* a = stack.data() + (top - 1) * Block;
            const float parameter = instruction.params[0].x;
            if (instruction.op == Op::Round)
                for (size_t i = 0; i < n; ++i) a[i] -= parameter;
            else if (instruction.op == Op::Shell)
                for (size_t i = 0; i < n; ++i) a[i] = std::abs(a[i]) - parameter;
        }
        for (size_t i = 0; i < n; ++i)
            distance[begin + i] = -stack[i];
        if (!primitive.empty())
            for (size_t i = 0; i < n; ++i)
                primitive[begin + i] = owners[i];
    }
}

Sample Geometry::sample(glm::vec3 point) const {
    float distance = 0.0f;
    uint32_t owner = 0;
    run({&point.x, 1}, {&point.y, 1}, {&point.z, 1}, {&distance, 1}, {&owner, 1});
    const auto& primitive = program_.code[owner];
    return {distance, primitive.material, primitive.motion};
}

void Geometry::normals(std::span<const float> x, std::span<const float> y, std::span<const float> z,
                       std::span<float> nx, std::span<float> ny, std::span<float> nz, float step) const {
    const size_t count = x.size();
    if (y.size() != count || z.size() != count || nx.size() != count || ny.size() != count || nz.size() != count)
        throw Error("normals: columns differ in length");
    static constexpr glm::vec3 k[4] = {{1, -1, -1}, {-1, -1, 1}, {-1, 1, -1}, {1, 1, 1}};
    // The four tetrahedral offsets of every point, evaluated together.
    std::vector<float> ox(4 * count), oy(4 * count), oz(4 * count), d(4 * count);
    for (size_t corner = 0; corner < 4; ++corner)
        for (size_t i = 0; i < count; ++i) {
            ox[corner * count + i] = x[i] + k[corner].x * step;
            oy[corner * count + i] = y[i] + k[corner].y * step;
            oz[corner * count + i] = z[i] + k[corner].z * step;
        }
    run(ox, oy, oz, d, {});
    for (size_t i = 0; i < count; ++i) {
        glm::vec3 gradient(0.0f);
        for (size_t corner = 0; corner < 4; ++corner)
            gradient += k[corner] * d[corner * count + i];
        const float length = glm::length(gradient);
        const glm::vec3 n = length > 0.0f ? gradient / length : glm::vec3(0.0f);
        nx[i] = n.x, ny[i] = n.y, nz[i] = n.z;
    }
}

glm::vec3 Geometry::normal(glm::vec3 point, float step) const {
    // Tetrahedral differences: four evaluations in one block.
    const glm::vec3 k[4] = {{1, -1, -1}, {-1, -1, 1}, {-1, 1, -1}, {1, 1, 1}};
    float x[4], y[4], z[4], d[4];
    for (int i = 0; i < 4; ++i) {
        const glm::vec3 p = point + k[i] * step;
        x[i] = p.x, y[i] = p.y, z[i] = p.z;
    }
    run(x, y, z, d, {});
    glm::vec3 gradient(0.0f);
    for (int i = 0; i < 4; ++i)
        gradient += k[i] * d[i];
    const float length = glm::length(gradient);
    return length > 0.0f ? gradient / length : glm::vec3(0.0f);
}

}
