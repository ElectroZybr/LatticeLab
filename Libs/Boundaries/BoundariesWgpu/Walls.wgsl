// Wall laws per particle, after the geometry machine (Geometry.wgsl).
// Bindings: 0 geometry code, 1 materials, 2 particle table (f32 words), 3 parameters.

const WORKGROUP: u32 = 64u;               // GpuWalls::WorkgroupSize
const LAW_HARD: u32 = 1u;                 // Boundaries::Laws::Hard::id
const LAW_SOFT: u32 = 2u;                 // Boundaries::Laws::Soft::id

@group(0) @binding(0) var<storage, read> geometryCode: array<Instruction>;

// Index of an invocation in a 2D grid of workgroups (ComputePass::dispatchCovering).
fn gridIndex(id: vec3<u32>, groups: vec3<u32>) -> u32 { return id.x + id.y * groups.x * WORKGROUP; }
struct Material {
    law: u32,
    params: array<f32, 7>,
};
@group(0) @binding(1) var<storage, read> materials: array<Material>;
@group(0) @binding(2) var<storage, read_write> data: array<f32>;

// Word offsets of the columns; a, b, c — velocities (Constrain) or forces (Force).
struct Params {
    rows: u32, x: u32, y: u32, z: u32,
    a: u32, b: u32, c: u32, passes: u32,
    forceRange: f32, pad0: f32, pad1: f32, pad2: f32,
};
@group(0) @binding(3) var<uniform> params: Params;

fn position(i: u32) -> vec3<f32> { return vec3<f32>(data[params.x + i], data[params.y + i], data[params.z + i]); }
fn triple(i: u32) -> vec3<f32> { return vec3<f32>(data[params.a + i], data[params.b + i], data[params.c + i]); }

@compute @workgroup_size(WORKGROUP)
fn constrain(@builtin(global_invocation_id) id: vec3<u32>, @builtin(num_workgroups) groups: vec3<u32>) {
    let i = gridIndex(id, groups);
    if (i >= params.rows) { return; }
    var p = position(i);
    let sample = sampleGeometry(p);
    if (sample.distance >= 0.0) { return; }
    let material = materials[geometryCode[sample.owner].material];
    if (material.law != LAW_HARD) { return; }
    // Back onto the surface along the normal, repeated while still inside (blends).
    var current = sample;
    for (var iteration = 0u; iteration < params.passes && current.distance < 0.0; iteration++) {
        p = p - current.normal * current.distance;
        current = sampleGeometry(p);
    }
    let n = current.normal;
    var v = triple(i);
    let normalSpeed = dot(v, n);
    if (normalSpeed < 0.0) {
        v = v - (1.0 + material.params[0]) * normalSpeed * n;
    }
    data[params.x + i] = p.x; data[params.y + i] = p.y; data[params.z + i] = p.z;
    data[params.a + i] = v.x; data[params.b + i] = v.y; data[params.c + i] = v.z;
}

@compute @workgroup_size(WORKGROUP)
fn forces(@builtin(global_invocation_id) id: vec3<u32>, @builtin(num_workgroups) groups: vec3<u32>) {
    let i = gridIndex(id, groups);
    if (i >= params.rows) { return; }
    let p = position(i);
    let sample = sampleGeometry(p);
    if (sample.distance >= params.forceRange) { return; }
    let material = materials[geometryCode[sample.owner].material];
    if (material.law != LAW_SOFT) { return; }
    let depth = material.params[1] - sample.distance;
    if (depth <= 0.0) { return; }
    let f = material.params[0] * pow(depth, material.params[2] - 1.0) * sample.normal;
    data[params.a + i] += f.x; data[params.b + i] += f.y; data[params.c + i] += f.z;
}
