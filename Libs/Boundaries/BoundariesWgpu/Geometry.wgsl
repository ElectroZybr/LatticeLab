// Geometry machine: the same postfix program as the CPU runs (BoundariesCpu/Geometry.cpp).
// The including shader declares the program:
//     @group(G) @binding(B) var<storage, read> geometryCode: array<Instruction>;
// Constants mirror the C++ code; GpuWalls.cpp checks them at compile time.

const GEOMETRY_DEPTH: u32 = 16u;          // Boundaries::Wgpu::MaxDepth

// Boundaries::Op: primitives < binary < unary operations.
const OP_SPHERE: u32 = 0u;
const OP_BOX: u32 = 1u;
const OP_CAPSULE: u32 = 2u;
const OP_CYLINDER: u32 = 3u;
const OP_HALF_SPACE: u32 = 4u;
const OP_INFINITE_CYLINDER: u32 = 5u;
const OP_UNION: u32 = 6u;
const OP_INTERSECT: u32 = 7u;
const OP_SUBTRACT: u32 = 8u;
const OP_SMOOTH_UNION: u32 = 9u;
const OP_ROUND: u32 = 10u;
const OP_SHELL: u32 = 11u;

struct Instruction {
    op: u32,
    material: u32,
    motion: u32,
    scale: f32,
    local: array<vec4<f32>, 3>,
    params: array<vec4<f32>, 2>,
};

struct GeometrySample {
    distance: f32,      // > 0 in the region, < 0 in a wall
    owner: u32,         // instruction of the primitive owning the nearest surface
    normal: vec3<f32>,  // unit gradient of the distance, into the region (analytic)
};

// Primitives in local coordinates: vec4(gradient, signed distance), negative inside the
// region (the usual SDF convention). Gradients are exact where the distance is smooth.

fn unitOr(v: vec3<f32>, fallback: vec3<f32>) -> vec3<f32> {
    let length2 = dot(v, v);
    if (length2 > 0.0) { return v * inverseSqrt(length2); }
    return fallback;
}

fn sdSphere(p: vec3<f32>, q: array<vec4<f32>, 2>) -> vec4<f32> {
    return vec4<f32>(unitOr(p, vec3<f32>(0.0, 1.0, 0.0)), length(p) - q[0].x);
}

fn sdBox(p: vec3<f32>, q: array<vec4<f32>, 2>) -> vec4<f32> {
    let d = abs(p) - q[0].xyz;
    let s = select(vec3<f32>(1.0), vec3<f32>(-1.0), p < vec3<f32>(0.0));
    let outside = max(d, vec3<f32>(0.0));
    let inside = min(max(d.x, max(d.y, d.z)), 0.0);
    let length2 = dot(outside, outside);
    if (length2 > 0.0) {
        let length = sqrt(length2);
        return vec4<f32>(s * outside / length, length + inside);
    }
    var g = vec3<f32>(0.0);
    if (d.x >= d.y && d.x >= d.z) { g.x = s.x; } else if (d.y >= d.z) { g.y = s.y; } else { g.z = s.z; }
    return vec4<f32>(g, inside);
}

fn sdCapsule(p: vec3<f32>, q: array<vec4<f32>, 2>) -> vec4<f32> {
    let pa = p - q[0].xyz;
    let ba = q[1].xyz - q[0].xyz;
    let length2 = dot(ba, ba);
    var h = 0.0;
    if (length2 > 0.0) { h = clamp(dot(pa, ba) / length2, 0.0, 1.0); }
    let v = pa - ba * h;
    return vec4<f32>(unitOr(v, vec3<f32>(0.0, 1.0, 0.0)), length(v) - q[1].w);
}

fn sdCylinder(p: vec3<f32>, q: array<vec4<f32>, 2>) -> vec4<f32> {
    let radial = length(p.xz);
    var out = vec3<f32>(1.0, 0.0, 0.0);
    if (radial > 0.0) { out = vec3<f32>(p.x, 0.0, p.z) / radial; }
    let up = vec3<f32>(0.0, select(1.0, -1.0, p.y < 0.0), 0.0);
    let d = vec2<f32>(radial, abs(p.y)) - vec2<f32>(q[0].x, q[0].y);
    let outside = max(d, vec2<f32>(0.0));
    let length2 = dot(outside, outside);
    if (length2 > 0.0) {
        let length = sqrt(length2);
        return vec4<f32>((outside.x * out + outside.y * up) / length, length + min(max(d.x, d.y), 0.0));
    }
    if (d.x >= d.y) { return vec4<f32>(out, d.x); }
    return vec4<f32>(up, d.y);
}

fn sdHalfSpace(p: vec3<f32>, q: array<vec4<f32>, 2>) -> vec4<f32> {
    return vec4<f32>(q[0].xyz, dot(q[0].xyz, p) - q[0].w);
}

fn sdInfiniteCylinder(p: vec3<f32>, q: array<vec4<f32>, 2>) -> vec4<f32> {
    let radial = length(p.xz);
    var out = vec3<f32>(1.0, 0.0, 0.0);
    if (radial > 0.0) { out = vec3<f32>(p.x, 0.0, p.z) / radial; }
    return vec4<f32>(out, radial - q[0].x);
}

fn sampleGeometry(p: vec3<f32>) -> GeometrySample {
    var stack: array<vec4<f32>, GEOMETRY_DEPTH>;   // xyz gradient, w distance
    var owner: array<u32, GEOMETRY_DEPTH>;
    var top = 0u;
    let count = arrayLength(&geometryCode);
    let h = vec4<f32>(p, 1.0);
    for (var i = 0u; i < count; i++) {
        let instruction = geometryCode[i];
        let op = instruction.op;
        if (op <= OP_INFINITE_CYLINDER) {
            let r0 = instruction.local[0];
            let r1 = instruction.local[1];
            let r2 = instruction.local[2];
            let l = vec3<f32>(dot(r0, h), dot(r1, h), dot(r2, h));
            var f = vec4<f32>(0.0);
            switch op {
                case OP_SPHERE: { f = sdSphere(l, instruction.params); }
                case OP_BOX: { f = sdBox(l, instruction.params); }
                case OP_CAPSULE: { f = sdCapsule(l, instruction.params); }
                case OP_CYLINDER: { f = sdCylinder(l, instruction.params); }
                case OP_HALF_SPACE: { f = sdHalfSpace(l, instruction.params); }
                default: { f = sdInfiniteCylinder(l, instruction.params); }
            }
            // World gradient = scale · Lᵀ · local gradient.
            let g = r0.xyz * f.x + r1.xyz * f.y + r2.xyz * f.z;
            stack[top] = instruction.scale * vec4<f32>(g, f.w);
            owner[top] = i;
            top += 1u;
        } else if (op <= OP_SMOOTH_UNION) {
            let a = stack[top - 2u];
            let b = stack[top - 1u];
            var value = a;
            var second = false;
            switch op {
                case OP_UNION: { second = b.w < a.w; value = select(a, b, second); }
                case OP_INTERSECT: { second = b.w > a.w; value = select(a, b, second); }
                case OP_SUBTRACT: { second = -b.w > a.w; value = select(a, -b, second); }
                default: {
                    let k = instruction.params[0].x;
                    let blend = max(k - abs(a.w - b.w), 0.0) / k;
                    second = b.w < a.w;
                    // d value / d near = 1 − blend/2, d value / d far = blend/2.
                    let near = select(a, b, second);
                    let far = select(b, a, second);
                    value = vec4<f32>((1.0 - 0.5 * blend) * near.xyz + 0.5 * blend * far.xyz,
                                      min(a.w, b.w) - blend * blend * k * 0.25);
                }
            }
            stack[top - 2u] = value;
            if (second) {
                owner[top - 2u] = owner[top - 1u];
            }
            top -= 1u;
        } else if (op == OP_ROUND) {
            stack[top - 1u].w = stack[top - 1u].w - instruction.params[0].x;
        } else {
            let v = stack[top - 1u];
            let sign = select(1.0, -1.0, v.w < 0.0);
            stack[top - 1u] = vec4<f32>(sign * v.xyz, abs(v.w) - instruction.params[0].x);
        }
    }
    let result = stack[0];
    return GeometrySample(-result.w, owner[0], -unitOr(result.xyz, vec3<f32>(0.0)));
}

fn geometryDistance(p: vec3<f32>) -> f32 { return sampleGeometry(p).distance; }

fn geometryNormal(p: vec3<f32>) -> vec3<f32> { return sampleGeometry(p).normal; }
