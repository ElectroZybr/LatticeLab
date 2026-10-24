#include "Geometry.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <vector>

#include <Lattice/Tools/Exception.hpp>

namespace Boundaries::Cpu {

using Error = Lattice::Exception<Geometry>;

namespace {

constexpr size_t Block = 64;

// Signed distances of the region's surface and their gradients, in local coordinates:
// negative inside the region (the usual SDF convention); the public distance is the
// negation (positive where particles may be). Gradients are exact where the distance
// is differentiable; on medial surfaces one side is taken.

struct Field {
    float d;
    glm::vec3 g;
};

glm::vec3 direction(glm::vec3 v, glm::vec3 fallback) {
    const float length = glm::length(v);
    return length > 0.0f ? v / length : fallback;
}

Field sphere(glm::vec3 p, const glm::vec4* q) {
    const float length = glm::length(p);
    return {length - q[0].x, length > 0.0f ? p / length : glm::vec3(0, 1, 0)};
}

Field box(glm::vec3 p, const glm::vec4* q) {
    const glm::vec3 d = glm::abs(p) - glm::vec3(q[0]);
    const glm::vec3 s(p.x < 0 ? -1.0f : 1.0f, p.y < 0 ? -1.0f : 1.0f, p.z < 0 ? -1.0f : 1.0f);
    const glm::vec3 outside = glm::max(d, glm::vec3(0.0f));
    const float inside = std::min(std::max(d.x, std::max(d.y, d.z)), 0.0f);
    const float length = glm::length(outside);
    if (length > 0.0f)
        return {length + inside, s * outside / length};
    // Inside: the nearest face is the largest component.
    const int axis = d.x >= d.y && d.x >= d.z ? 0 : (d.y >= d.z ? 1 : 2);
    glm::vec3 g(0.0f);
    g[axis] = s[axis];
    return {inside, g};
}

Field capsule(glm::vec3 p, const glm::vec4* q) {
    const glm::vec3 a(q[0]), b(q[1]);
    const glm::vec3 pa = p - a, ba = b - a;
    const float length = glm::dot(ba, ba);
    const float h = length > 0.0f ? std::clamp(glm::dot(pa, ba) / length, 0.0f, 1.0f) : 0.0f;
    const glm::vec3 v = pa - ba * h;
    return {glm::length(v) - q[1].w, direction(v, glm::vec3(0, 1, 0))};
}

Field cylinder(glm::vec3 p, const glm::vec4* q) {
    const float radial = glm::length(glm::vec2(p.x, p.z));
    const glm::vec3 out = radial > 0.0f ? glm::vec3(p.x, 0, p.z) / radial : glm::vec3(1, 0, 0);
    const glm::vec3 up(0, p.y < 0 ? -1.0f : 1.0f, 0);
    const glm::vec2 d = glm::vec2(radial, std::abs(p.y)) - glm::vec2(q[0].x, q[0].y);
    const glm::vec2 outside = glm::max(d, glm::vec2(0.0f));
    const float length = glm::length(outside);
    if (length > 0.0f)
        return {length + std::min(std::max(d.x, d.y), 0.0f), (outside.x * out + outside.y * up) / length};
    return d.x >= d.y ? Field{d.x, out} : Field{d.y, up};
}

Field halfSpace(glm::vec3 p, const glm::vec4* q) { return {glm::dot(glm::vec3(q[0]), p) - q[0].w, glm::vec3(q[0])}; }

Field infiniteCylinder(glm::vec3 p, const glm::vec4* q) {
    const float radial = glm::length(glm::vec2(p.x, p.z));
    return {radial - q[0].x, radial > 0.0f ? glm::vec3(p.x, 0, p.z) / radial : glm::vec3(1, 0, 0)};
}

// Distance only: the path of evaluate(), without the cost of gradients.
float sphereDistance(glm::vec3 p, const glm::vec4* q) { return glm::length(p) - q[0].x; }

float boxDistance(glm::vec3 p, const glm::vec4* q) {
    const glm::vec3 d = glm::abs(p) - glm::vec3(q[0]);
    return glm::length(glm::max(d, glm::vec3(0.0f))) + std::min(std::max(d.x, std::max(d.y, d.z)), 0.0f);
}

float capsuleDistance(glm::vec3 p, const glm::vec4* q) {
    const glm::vec3 a(q[0]), b(q[1]);
    const glm::vec3 pa = p - a, ba = b - a;
    const float length = glm::dot(ba, ba);
    const float h = length > 0.0f ? std::clamp(glm::dot(pa, ba) / length, 0.0f, 1.0f) : 0.0f;
    return glm::length(pa - ba * h) - q[1].w;
}

float cylinderDistance(glm::vec3 p, const glm::vec4* q) {
    const glm::vec2 d = glm::abs(glm::vec2(glm::length(glm::vec2(p.x, p.z)), p.y)) - glm::vec2(q[0].x, q[0].y);
    return std::min(std::max(d.x, d.y), 0.0f) + glm::length(glm::max(d, glm::vec2(0.0f)));
}

float halfSpaceDistance(glm::vec3 p, const glm::vec4* q) { return glm::dot(glm::vec3(q[0]), p) - q[0].w; }

float infiniteCylinderDistance(glm::vec3 p, const glm::vec4* q) { return glm::length(glm::vec2(p.x, p.z)) - q[0].x; }

/// One level of the stack for a block: distances, gradients (when asked) and owners.
struct Level {
    float* d;
    float* gx;
    float* gy;
    float* gz;
    uint32_t* owner;
};

template<class Distance>
void evaluateDistance(const Instruction& instruction, Distance distance, size_t count,
                      const float* x, const float* y, const float* z, float* out) {
    const glm::vec4 r0 = instruction.local[0], r1 = instruction.local[1], r2 = instruction.local[2];
    for (size_t i = 0; i < count; ++i) {
        const glm::vec4 p(x[i], y[i], z[i], 1.0f);
        out[i] = instruction.scale * distance({glm::dot(r0, p), glm::dot(r1, p), glm::dot(r2, p)}, instruction.params);
    }
}

template<bool Gradient, class Primitive>
void evaluatePrimitive(const Instruction& instruction, Primitive primitive, size_t count,
                       const float* x, const float* y, const float* z, const Level& out) {
    const glm::vec4 r0 = instruction.local[0], r1 = instruction.local[1], r2 = instruction.local[2];
    // World gradient = scale · Lᵀ · local gradient (L — the 3×3 part of world → local).
    const glm::vec3 c0(r0.x, r1.x, r2.x), c1(r0.y, r1.y, r2.y), c2(r0.z, r1.z, r2.z);
    for (size_t i = 0; i < count; ++i) {
        const glm::vec4 p(x[i], y[i], z[i], 1.0f);
        const glm::vec3 local(glm::dot(r0, p), glm::dot(r1, p), glm::dot(r2, p));
        const Field field = primitive(local, instruction.params);
        out.d[i] = instruction.scale * field.d;
        if constexpr (Gradient) {
            const glm::vec3 g = field.g;
            out.gx[i] = instruction.scale * glm::dot(c0, g);
            out.gy[i] = instruction.scale * glm::dot(c1, g);
            out.gz[i] = instruction.scale * glm::dot(c2, g);
        }
    }
}

}

Geometry::Geometry(Program program) : program_(std::move(program)) {
    if (program_.code.empty() || program_.depth == 0)
        throw Error("geometry program is empty");
}

template<bool Gradient>
void Geometry::run(std::span<const float> x, std::span<const float> y, std::span<const float> z,
                   std::span<float> distance, std::span<uint32_t> primitive,
                   std::span<float> nx, std::span<float> ny, std::span<float> nz) const {
    const size_t count = x.size();
    if (y.size() != count || z.size() != count || distance.size() != count)
        throw Error("evaluate: columns differ in length");

    // Stack of the block: depth × Block values per field. Typical programs fit the fixed
    // buffer; deeper ones take the heap once per call.
    constexpr size_t FixedDepth = 16;
    constexpr size_t Fields = Gradient ? 4 : 1;
    std::array<float, FixedDepth * Block * Fields> fixedValues;
    std::array<uint32_t, FixedDepth * Block> fixedOwners;
    std::vector<float> heapValues;
    std::vector<uint32_t> heapOwners;
    std::span<float> values(fixedValues);
    std::span<uint32_t> owners(fixedOwners);
    if (program_.depth > FixedDepth) {
        heapValues.resize(program_.depth * Block * Fields);
        heapOwners.resize(program_.depth * Block);
        values = heapValues;
        owners = heapOwners;
    }
    const auto level = [&](size_t index) {
        float* base = values.data() + index * Block * Fields;
        return Level{base, base + (Gradient ? Block : 0), base + (Gradient ? 2 * Block : 0),
            base + (Gradient ? 3 * Block : 0), owners.data() + index * Block};
    };

    for (size_t begin = 0; begin < count; begin += Block) {
        const size_t n = std::min(Block, count - begin);
        const float *px = x.data() + begin, *py = y.data() + begin, *pz = z.data() + begin;
        size_t top = 0;
        for (uint32_t index = 0; index < program_.code.size(); ++index) {
            const auto& instruction = program_.code[index];
            if (isPrimitive(instruction.op)) {
                const Level out = level(top);
                if constexpr (Gradient) {
                    switch (instruction.op) {
                        case Op::Sphere:    evaluatePrimitive<true>(instruction, sphere, n, px, py, pz, out); break;
                        case Op::Box:       evaluatePrimitive<true>(instruction, box, n, px, py, pz, out); break;
                        case Op::Capsule:   evaluatePrimitive<true>(instruction, capsule, n, px, py, pz, out); break;
                        case Op::Cylinder:  evaluatePrimitive<true>(instruction, cylinder, n, px, py, pz, out); break;
                        case Op::HalfSpace: evaluatePrimitive<true>(instruction, halfSpace, n, px, py, pz, out); break;
                        default:   evaluatePrimitive<true>(instruction, infiniteCylinder, n, px, py, pz, out); break;
                    }
                } else {
                    // One call site per formula: the formula inlines into its loop.
                    switch (instruction.op) {
                        case Op::Sphere:    evaluateDistance(instruction, sphereDistance, n, px, py, pz, out.d); break;
                        case Op::Box:       evaluateDistance(instruction, boxDistance, n, px, py, pz, out.d); break;
                        case Op::Capsule:   evaluateDistance(instruction, capsuleDistance, n, px, py, pz, out.d); break;
                        case Op::Cylinder:  evaluateDistance(instruction, cylinderDistance, n, px, py, pz, out.d); break;
                        case Op::HalfSpace: evaluateDistance(instruction, halfSpaceDistance, n, px, py, pz, out.d); break;
                        default:   evaluateDistance(instruction, infiniteCylinderDistance, n, px, py, pz, out.d); break;
                    }
                }
                std::fill_n(out.owner, n, index);
                ++top;
                continue;
            }
            if (isBinary(instruction.op) && !Gradient) {
                const Level a = level(top - 2), b = level(top - 1);
                const float k = instruction.params[0].x;
                switch (instruction.op) {
                    case Op::Union:
                        for (size_t i = 0; i < n; ++i)
                            if (b.d[i] < a.d[i]) a.d[i] = b.d[i], a.owner[i] = b.owner[i];
                        break;
                    case Op::Intersect:
                        for (size_t i = 0; i < n; ++i)
                            if (b.d[i] > a.d[i]) a.d[i] = b.d[i], a.owner[i] = b.owner[i];
                        break;
                    case Op::Subtract:
                        for (size_t i = 0; i < n; ++i)
                            if (-b.d[i] > a.d[i]) a.d[i] = -b.d[i], a.owner[i] = b.owner[i];
                        break;
                    default:
                        for (size_t i = 0; i < n; ++i) {
                            const float h = std::max(k - std::abs(a.d[i] - b.d[i]), 0.0f) / k;
                            if (b.d[i] < a.d[i]) a.owner[i] = b.owner[i];
                            a.d[i] = std::min(a.d[i], b.d[i]) - h * h * k * 0.25f;
                        }
                        break;
                }
                --top;
                continue;
            }
            if (isBinary(instruction.op)) {
                const Level a = level(top - 2), b = level(top - 1);
                const float k = instruction.params[0].x;
                for (size_t i = 0; i < n; ++i) {
                    // value = wa·a + wb·b (for the gradient); selection for the owner.
                    float value = a.d[i], wa = 1.0f, wb = 0.0f;
                    bool second = false;
                    switch (instruction.op) {
                        case Op::Union:     second = b.d[i] < a.d[i]; break;
                        case Op::Intersect: second = b.d[i] > a.d[i]; break;
                        case Op::Subtract:  second = -b.d[i] > a.d[i]; break;
                        default: break;
                    }
                    if (instruction.op == Op::SmoothUnion) {
                        const float h = std::max(k - std::abs(a.d[i] - b.d[i]), 0.0f) / k;
                        second = b.d[i] < a.d[i];
                        value = std::min(a.d[i], b.d[i]) - h * h * k * 0.25f;
                        // d value / d near = 1 − h/2, d value / d far = h/2.
                        wa = second ? 0.5f * h : 1.0f - 0.5f * h;
                        wb = second ? 1.0f - 0.5f * h : 0.5f * h;
                    } else if (second) {
                        const float sign = instruction.op == Op::Subtract ? -1.0f : 1.0f;
                        value = sign * b.d[i];
                        wa = 0.0f, wb = sign;
                    }
                    a.d[i] = value;
                    if constexpr (Gradient) {
                        a.gx[i] = wa * a.gx[i] + wb * b.gx[i];
                        a.gy[i] = wa * a.gy[i] + wb * b.gy[i];
                        a.gz[i] = wa * a.gz[i] + wb * b.gz[i];
                    }
                    if (second)
                        a.owner[i] = b.owner[i];
                }
                --top;
                continue;
            }
            const Level a = level(top - 1);
            const float parameter = instruction.params[0].x;
            if (instruction.op == Op::Round) {
                for (size_t i = 0; i < n; ++i) a.d[i] -= parameter;
            } else {
                for (size_t i = 0; i < n; ++i) {
                    const float sign = a.d[i] < 0.0f ? -1.0f : 1.0f;
                    a.d[i] = std::abs(a.d[i]) - parameter;
                    if constexpr (Gradient)
                        a.gx[i] *= sign, a.gy[i] *= sign, a.gz[i] *= sign;
                }
            }
        }
        const Level result = level(0);
        for (size_t i = 0; i < n; ++i)
            distance[begin + i] = -result.d[i];
        if (!primitive.empty())
            for (size_t i = 0; i < n; ++i)
                primitive[begin + i] = result.owner[i];
        if constexpr (Gradient)
            for (size_t i = 0; i < n; ++i) {
                // The distance into the region is the negation: so is its gradient.
                const glm::vec3 g = -direction({result.gx[i], result.gy[i], result.gz[i]}, glm::vec3(0.0f));
                nx[begin + i] = g.x, ny[begin + i] = g.y, nz[begin + i] = g.z;
            }
    }
}

void Geometry::evaluate(std::span<const float> x, std::span<const float> y, std::span<const float> z,
                        std::span<float> distance, std::span<uint32_t> material) const {
    if (!material.empty() && material.size() != distance.size())
        throw Error("evaluate: material column differs in length");
    run<false>(x, y, z, distance, material, {}, {}, {});
    for (uint32_t& value : material)
        value = program_.code[value].material;
}

Sample Geometry::sample(glm::vec3 point) const {
    float distance = 0.0f;
    uint32_t owner = 0;
    run<false>({&point.x, 1}, {&point.y, 1}, {&point.z, 1}, {&distance, 1}, {&owner, 1}, {}, {}, {});
    const auto& primitive = program_.code[owner];
    return {distance, primitive.material, primitive.motion};
}

void Geometry::normals(std::span<const float> x, std::span<const float> y, std::span<const float> z,
                       std::span<float> nx, std::span<float> ny, std::span<float> nz) const {
    const size_t count = x.size();
    if (nx.size() != count || ny.size() != count || nz.size() != count)
        throw Error("normals: columns differ in length");
    // Walls call this per chunk: no allocation for chunk-sized batches.
    std::array<float, 256> fixed;
    std::vector<float> heap;
    std::span<float> distance;
    if (count <= fixed.size()) {
        distance = std::span(fixed).first(count);
    } else {
        heap.resize(count);
        distance = heap;
    }
    run<true>(x, y, z, distance, {}, nx, ny, nz);
}

glm::vec3 Geometry::normal(glm::vec3 point) const {
    float distance = 0.0f;
    glm::vec3 n;
    run<true>({&point.x, 1}, {&point.y, 1}, {&point.z, 1}, {&distance, 1}, {}, {&n.x, 1}, {&n.y, 1}, {&n.z, 1});
    return n;
}

}
