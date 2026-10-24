// Surface nets of a geometry program in bricks of 8³ cells; only bricks the surface may
// cross are sampled. Prepended: the geometry machine and `sdfDistance` (generated code).
const BRICK: u32 = 8u;
const BRICK_CELLS: u32 = 512u;
const BRICK_CORNERS: u32 = 729u;
const NONE: u32 = 0xFFFFFFFFu;

struct Params {
    origin: vec4<f32>,      // w: cell size
    bricks: vec4<u32>,      // w: total
    capacity: vec4<u32>,    // slots, vertices, indices
};
struct Counters {
    slots: atomic<u32>,
    vertices: atomic<u32>,
    indices: atomic<u32>,
    pad: u32,
};
struct Vertex {
    position: vec4<f32>,
    normal: vec4<f32>,
};

@group(0) @binding(0) var<storage, read> geometryCode: array<Instruction>;
@group(0) @binding(1) var<uniform> params: Params;
@group(0) @binding(2) var<storage, read_write> counters: Counters;
@group(0) @binding(3) var<storage, read_write> slotOf: array<u32>;
@group(0) @binding(4) var<storage, read_write> brickOf: array<u32>;
@group(0) @binding(5) var<storage, read_write> samples: array<f32>;
@group(0) @binding(6) var<storage, read_write> cellVertex: array<u32>;
@group(0) @binding(7) var<storage, read_write> vertices: array<Vertex>;
@group(0) @binding(8) var<storage, read_write> indices: array<u32>;

fn gridIndex(id: vec3<u32>, groups: vec3<u32>) -> u32 { return id.x + id.y * groups.x * 64u; }

fn brickCoord(index: u32) -> vec3<u32> {
    let b = params.bricks;
    return vec3<u32>(index % b.x, (index / b.x) % b.y, index / (b.x * b.y));
}

fn cornerPosition(corner: vec3<u32>) -> vec3<f32> {
    return params.origin.xyz + vec3<f32>(corner) * params.origin.w;
}

fn activeSlots() -> u32 {
    return min(atomicLoad(&counters.slots), params.capacity.x);
}

fn sampleAt(slot: u32, local: vec3<u32>) -> f32 {
    return samples[slot * BRICK_CORNERS + (local.z * 9u + local.y) * 9u + local.x];
}

@compute @workgroup_size(64)
fn clear(@builtin(global_invocation_id) id: vec3<u32>, @builtin(num_workgroups) groups: vec3<u32>) {
    let i = gridIndex(id, groups);
    if (i < params.bricks.w) { slotOf[i] = NONE; }
}

// A brick is kept when the surface may cross it: |d| at its center within the half
// diagonal (the program's distance never overestimates).
@compute @workgroup_size(64)
fn classify(@builtin(global_invocation_id) id: vec3<u32>, @builtin(num_workgroups) groups: vec3<u32>) {
    let i = gridIndex(id, groups);
    if (i >= params.bricks.w) { return; }
    let cell = params.origin.w;
    let center = params.origin.xyz + (vec3<f32>(brickCoord(i) * BRICK) + 4.0) * cell;
    if (abs(sdfDistance(center)) > 4.0 * cell * 1.7320508 + cell * 1e-3) { return; }
    let slot = atomicAdd(&counters.slots, 1u);
    if (slot < params.capacity.x) {
        slotOf[i] = slot;
        brickOf[slot] = i;
    }
}

@compute @workgroup_size(64)
fn sample(@builtin(workgroup_id) group: vec3<u32>, @builtin(num_workgroups) groups: vec3<u32>,
          @builtin(local_invocation_index) lane: u32) {
    let slot = group.x + group.y * groups.x;
    if (slot >= activeSlots()) { return; }
    let base = brickCoord(brickOf[slot]) * BRICK;
    for (var k = lane; k < BRICK_CORNERS; k += 64u) {
        let local = vec3<u32>(k % 9u, (k / 9u) % 9u, k / 81u);
        samples[slot * BRICK_CORNERS + k] = sdfDistance(cornerPosition(base + local));
    }
}

const CORNERS = array<vec3<u32>, 8>(
    vec3<u32>(0u, 0u, 0u), vec3<u32>(1u, 0u, 0u), vec3<u32>(0u, 1u, 0u), vec3<u32>(1u, 1u, 0u),
    vec3<u32>(0u, 0u, 1u), vec3<u32>(1u, 0u, 1u), vec3<u32>(0u, 1u, 1u), vec3<u32>(1u, 1u, 1u));
const EDGES = array<vec2<u32>, 12>(
    vec2<u32>(0u, 1u), vec2<u32>(2u, 3u), vec2<u32>(4u, 5u), vec2<u32>(6u, 7u),
    vec2<u32>(0u, 2u), vec2<u32>(1u, 3u), vec2<u32>(4u, 6u), vec2<u32>(5u, 7u),
    vec2<u32>(0u, 4u), vec2<u32>(1u, 5u), vec2<u32>(2u, 6u), vec2<u32>(3u, 7u));

// Mirrors surfaceNets (SurfaceNets.cpp): the vertex at the mean of the edge crossings,
// the normal from the trilinear gradient.
@compute @workgroup_size(64)
fn place(@builtin(workgroup_id) group: vec3<u32>, @builtin(num_workgroups) groups: vec3<u32>,
         @builtin(local_invocation_index) lane: u32) {
    let slot = group.x + group.y * groups.x;
    if (slot >= activeSlots()) { return; }
    let base = brickCoord(brickOf[slot]) * BRICK;
    for (var c = lane; c < BRICK_CELLS; c += 64u) {
        let local = vec3<u32>(c % 8u, (c / 8u) % 8u, c / 64u);
        var d: array<f32, 8>;
        var inside = false;
        var outside = false;
        for (var k = 0u; k < 8u; k++) {
            d[k] = sampleAt(slot, local + CORNERS[k]);
            if (d[k] > 0.0) { inside = true; } else { outside = true; }
        }
        cellVertex[slot * BRICK_CELLS + c] = NONE;
        if (!inside || !outside) { continue; }

        var sum = vec3<f32>(0.0);
        var crossings = 0u;
        for (var e = 0u; e < 12u; e++) {
            let a = EDGES[e].x;
            let b = EDGES[e].y;
            if ((d[a] > 0.0) == (d[b] > 0.0)) { continue; }
            let t = d[a] / (d[a] - d[b]);
            sum += mix(vec3<f32>(CORNERS[a]), vec3<f32>(CORNERS[b]), t);
            crossings += 1u;
        }
        let u = sum / f32(crossings);
        let v = 1.0 - u;
        let gradient = vec3<f32>(
            v.y * v.z * (d[1] - d[0]) + u.y * v.z * (d[3] - d[2]) + v.y * u.z * (d[5] - d[4]) + u.y * u.z * (d[7] - d[6]),
            v.x * v.z * (d[2] - d[0]) + u.x * v.z * (d[3] - d[1]) + v.x * u.z * (d[6] - d[4]) + u.x * u.z * (d[7] - d[5]),
            v.x * v.y * (d[4] - d[0]) + u.x * v.y * (d[5] - d[1]) + v.x * u.y * (d[6] - d[2]) + u.x * u.y * (d[7] - d[3]));
        let magnitude = length(gradient);
        let normal = select(vec3<f32>(0.0, 1.0, 0.0), gradient / magnitude, magnitude > 0.0);

        let index = atomicAdd(&counters.vertices, 1u);
        if (index >= params.capacity.y) { continue; }
        let cell = params.origin.w;
        vertices[index] = Vertex(vec4<f32>(params.origin.xyz + (vec3<f32>(base + local) + u) * cell, 0.0),
                                 vec4<f32>(normal, 0.0));
        cellVertex[slot * BRICK_CELLS + c] = index;
    }
}

fn vertexOfCell(cell: vec3<u32>) -> u32 {
    let b = cell / BRICK;
    let slot = slotOf[(b.z * params.bricks.y + b.y) * params.bricks.x + b.x];
    if (slot == NONE) { return NONE; }
    let local = cell % BRICK;
    return cellVertex[slot * BRICK_CELLS + (local.z * 8u + local.y) * 8u + local.x];
}

// An edge belongs to the brick of its start corner; its quad joins the four cells around
// it and faces the side where the region lies (as in surfaceNets).
@compute @workgroup_size(64)
fn connect(@builtin(workgroup_id) group: vec3<u32>, @builtin(num_workgroups) groups: vec3<u32>,
           @builtin(local_invocation_index) lane: u32) {
    let slot = group.x + group.y * groups.x;
    if (slot >= activeSlots()) { return; }
    let base = brickCoord(brickOf[slot]) * BRICK;
    let cells = params.bricks.xyz * BRICK;
    for (var k = lane; k < 3u * BRICK_CELLS; k += 64u) {
        let a = k / BRICK_CELLS;
        let b = (a + 1u) % 3u;
        let c = (a + 2u) % 3u;
        let corner = k % BRICK_CELLS;
        let local = vec3<u32>(corner % 8u, (corner / 8u) % 8u, corner / 64u);
        let p = base + local;
        if (p[a] >= cells[a] || p[b] == 0u || p[c] == 0u) { continue; }
        var e = vec3<u32>(0u);
        e[a] = 1u;
        let near = sampleAt(slot, local) > 0.0;
        let far = sampleAt(slot, local + e) > 0.0;
        if (near == far) { continue; }

        var eb = vec3<u32>(0u);
        eb[b] = 1u;
        var ec = vec3<u32>(0u);
        ec[c] = 1u;
        let q0 = vertexOfCell(p - eb - ec);
        let q1 = vertexOfCell(p - ec);
        let q2 = vertexOfCell(p);
        let q3 = vertexOfCell(p - eb);
        if (q0 == NONE || q1 == NONE || q2 == NONE || q3 == NONE) { continue; }
        let at = atomicAdd(&counters.indices, 6u);
        if (at + 6u > params.capacity.z) { continue; }
        if (far) {
            indices[at] = q0; indices[at + 1u] = q1; indices[at + 2u] = q2;
            indices[at + 3u] = q0; indices[at + 4u] = q2; indices[at + 5u] = q3;
        } else {
            indices[at] = q0; indices[at + 1u] = q2; indices[at + 2u] = q1;
            indices[at + 3u] = q0; indices[at + 4u] = q3; indices[at + 5u] = q2;
        }
    }
}
