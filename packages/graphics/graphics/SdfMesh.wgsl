// Surface nets of a geometry program in bricks of 8³ cells; only bricks the surface may
// cross are sampled. Prepended: the geometry machine and `sdfDistance` (generated code).
const BRICK: u32 = 8u;
const BRICK_CELLS: u32 = 512u;
const BRICK_CORNERS: u32 = 729u;
const SLOT_WORDS: u32 = 1241u;           // samples (729 f32), then cell vertices (512 u32)
const NONE: u32 = 0xFFFFFFFFu;

struct Params {
    origin: vec4<f32>,      // w: cell size
    bricks: vec4<u32>,      // w: total
    capacity: vec4<u32>,    // slots, vertices, indices, edges
};
struct Counters {
    slots: atomic<u32>,
    vertices: atomic<u32>,
    indices: atomic<u32>,
    edges: atomic<u32>,
};
struct Vertex {
    position: vec4<f32>,
    normal: vec4<f32>,
};
struct Edge {
    a: u32,
    b: u32,
    pad0: u32,
    pad1: u32,
    first: vec4<f32>,
    second: vec4<f32>,
};

@group(0) @binding(0) var<storage, read> geometryCode: array<Instruction>;
@group(0) @binding(1) var<uniform> params: Params;
@group(0) @binding(2) var<storage, read_write> counters: Counters;
@group(0) @binding(3) var<storage, read_write> slotOf: array<u32>;
@group(0) @binding(4) var<storage, read_write> brickOf: array<u32>;
@group(0) @binding(5) var<storage, read_write> slotData: array<u32>;
@group(0) @binding(6) var<storage, read_write> vertices: array<Vertex>;
@group(0) @binding(7) var<storage, read_write> indices: array<u32>;
@group(0) @binding(8) var<storage, read_write> edges: array<Edge>;

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
    return bitcast<f32>(slotData[slot * SLOT_WORDS + (local.z * 9u + local.y) * 9u + local.x]);
}

fn cellVertexAt(slot: u32, cell: u32) -> u32 {
    return slotData[slot * SLOT_WORDS + BRICK_CORNERS + cell];
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
        slotData[slot * SLOT_WORDS + k] = bitcast<u32>(sdfDistance(cornerPosition(base + local)));
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
        slotData[slot * SLOT_WORDS + BRICK_CORNERS + c] = NONE;
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
        slotData[slot * SLOT_WORDS + BRICK_CORNERS + c] = index;
    }
}

fn vertexOfCell(cell: vec3<u32>) -> u32 {
    let b = cell / BRICK;
    let slot = slotOf[(b.z * params.bricks.y + b.y) * params.bricks.x + b.x];
    if (slot == NONE) { return NONE; }
    let local = cell % BRICK;
    return cellVertexAt(slot, (local.z * 8u + local.y) * 8u + local.x);
}

fn unit(axis: u32) -> vec3<u32> {
    var e = vec3<u32>(0u);
    e[axis] = 1u;
    return e;
}

// The quad of the grid edge from corner s along axis d (as connect makes it), or no
// normal (w 0) when connect makes none there.
fn quadNormal(s: vec3<u32>, d: u32) -> vec4<f32> {
    let cells = params.bricks.xyz * BRICK;
    let b = (d + 1u) % 3u;
    let c = (d + 2u) % 3u;
    if (s[d] >= cells[d] || s[b] == 0u || s[c] == 0u || s[b] >= cells[b] || s[c] >= cells[c]) {
        return vec4<f32>(0.0);
    }
    let brick = s / BRICK;
    let slot = slotOf[(brick.z * params.bricks.y + brick.y) * params.bricks.x + brick.x];
    if (slot == NONE) { return vec4<f32>(0.0); }
    let local = s % BRICK;
    let near = sampleAt(slot, local) > 0.0;
    let far = sampleAt(slot, local + unit(d)) > 0.0;
    if (near == far) { return vec4<f32>(0.0); }
    let q0 = vertexOfCell(s - unit(b) - unit(c));
    let q1 = vertexOfCell(s - unit(c));
    let q2 = vertexOfCell(s);
    let q3 = vertexOfCell(s - unit(b));
    if (q0 == NONE || q1 == NONE || q2 == NONE || q3 == NONE) { return vec4<f32>(0.0); }
    let n = cross(vertices[q2].position.xyz - vertices[q0].position.xyz,
                  vertices[q3].position.xyz - vertices[q1].position.xyz);
    let size = length(n);
    if (size == 0.0) { return vec4<f32>(0.0); }
    return vec4<f32>(select(-n, n, far) / size, 1.0);
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

// An edge of the mesh joins the vertices of two neighbouring cells; its faces are the quads
// of the crossed grid edges on the face between those cells.
@compute @workgroup_size(64)
fn outline(@builtin(workgroup_id) group: vec3<u32>, @builtin(num_workgroups) groups: vec3<u32>,
           @builtin(local_invocation_index) lane: u32) {
    let slot = group.x + group.y * groups.x;
    if (slot >= activeSlots()) { return; }
    let base = brickCoord(brickOf[slot]) * BRICK;
    let cells = params.bricks.xyz * BRICK;
    for (var k = lane; k < 3u * BRICK_CELLS; k += 64u) {
        let a = k / BRICK_CELLS;
        let index = k % BRICK_CELLS;
        let cell = base + vec3<u32>(index % 8u, (index / 8u) % 8u, index / 64u);
        if (cell[a] + 1u >= cells[a]) { continue; }
        let v0 = cellVertexAt(slot, index);
        let v1 = vertexOfCell(cell + unit(a));
        if (v0 == NONE || v1 == NONE) { continue; }

        let b = (a + 1u) % 3u;
        let c = (a + 2u) % 3u;
        let q = cell + unit(a);
        let candidates = array<vec4<f32>, 4>(
            quadNormal(q, b), quadNormal(q + unit(c), b), quadNormal(q, c), quadNormal(q + unit(b), c));
        var normals = array<vec4<f32>, 2>(vec4<f32>(0.0), vec4<f32>(0.0));
        var found = 0u;
        for (var i = 0u; i < 4u; i++) {
            if (candidates[i].w > 0.0 && found < 2u) {
                normals[found] = candidates[i];
                found += 1u;
            }
        }
        if (found == 0u) { continue; }
        let at = atomicAdd(&counters.edges, 1u);
        if (at < params.capacity.w) {
            edges[at] = Edge(v0, v1, 0u, 0u, normals[0], normals[1]);
        }
    }
}
