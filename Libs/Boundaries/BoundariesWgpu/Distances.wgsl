// Distances of arbitrary points, after the geometry machine (Geometry.wgsl).

@group(0) @binding(0) var<storage, read> geometryCode: array<Instruction>;
@group(0) @binding(1) var<storage, read> data: array<f32>;
@group(0) @binding(2) var<storage, read_write> distances: array<f32>;
struct Params { rows: u32, x: u32, y: u32, z: u32 };
@group(0) @binding(3) var<uniform> params: Params;

// Index of an invocation in a 2D grid of workgroups (ComputePass::dispatchCovering).
fn gridIndex(id: vec3<u32>, groups: vec3<u32>) -> u32 { return id.x + id.y * groups.x * 64u; }

@compute @workgroup_size(64)
fn main(@builtin(global_invocation_id) id: vec3<u32>, @builtin(num_workgroups) groups: vec3<u32>) {
    let i = gridIndex(id, groups);
    if (i >= params.rows) { return; }
    distances[i] = geometryDistance(vec3<f32>(data[params.x + i], data[params.y + i], data[params.z + i]));
}
