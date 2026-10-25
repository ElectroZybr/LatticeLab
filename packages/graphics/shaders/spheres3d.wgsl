// Spheres from a GPU table: one row — one sphere. Centers are read from the table's
// columns by word offset; the kind column (optional) picks color and radius.
struct Camera {
    viewProjection: mat4x4<f32>,
    view: mat4x4<f32>,
    projection: mat4x4<f32>,
    position: vec4<f32>,
};
struct Params {
    x: u32, y: u32, z: u32, kind: u32,
    paletteSize: u32, pad0: u32, pad1: u32, pad2: u32,
};
@group(0) @binding(0) var<uniform> camera: Camera;
@group(1) @binding(0) var<storage, read> data: array<u32>;
@group(1) @binding(1) var<uniform> params: Params;
@group(1) @binding(2) var<storage, read> palette: array<vec4<f32>>;   // rgb, radius

struct VSOut {
    @builtin(position) position: vec4<f32>,
    @location(0) uv: vec2<f32>,
    @location(1) color: vec3<f32>,
    @location(2) centerView: vec3<f32>,
    @location(3) radius: f32,
};

@vertex
fn vs(@builtin(vertex_index) vertex: u32, @builtin(instance_index) row: u32) -> VSOut {
    var corners = array<vec2<f32>, 6>(
        vec2<f32>(-1.0, -1.0), vec2<f32>(1.0, -1.0), vec2<f32>(1.0, 1.0),
        vec2<f32>(-1.0, -1.0), vec2<f32>(1.0, 1.0), vec2<f32>(-1.0, 1.0)
    );
    let corner = corners[vertex];
    let center = vec3<f32>(
        bitcast<f32>(data[params.x + row]),
        bitcast<f32>(data[params.y + row]),
        bitcast<f32>(data[params.z + row]));
    var kind = 0u;
    if (params.kind != 0xFFFFFFFFu) {
        kind = data[params.kind + row] % params.paletteSize;
    }
    let style = palette[kind];
    let radius = style.w;

    let right = vec3<f32>(camera.view[0][0], camera.view[1][0], camera.view[2][0]);
    let up = vec3<f32>(camera.view[0][1], camera.view[1][1], camera.view[2][1]);
    let world = center + (right * corner.x + up * corner.y) * radius;

    var out: VSOut;
    out.position = camera.viewProjection * vec4<f32>(world, 1.0);
    out.uv = corner;
    out.color = style.rgb;
    out.centerView = (camera.view * vec4<f32>(center, 1.0)).xyz;
    out.radius = radius;
    return out;
}

struct FSOut {
    @builtin(frag_depth) depth: f32,
    @location(0) color: vec4<f32>,
};

@fragment
fn fs(in: VSOut) -> FSOut {
    let d2 = dot(in.uv, in.uv);
    if (d2 > 1.0) {
        discard;
    }
    // Sphere surface under this pixel, in view space: lit and with its real depth.
    let z = sqrt(1.0 - d2);
    let normal = vec3<f32>(in.uv, z);
    let light = normalize(vec3<f32>(0.4, 0.6, 1.0));
    let shade = 0.3 + 0.7 * max(dot(normal, light), 0.0);
    let surface = in.centerView + vec3<f32>(in.uv * in.radius, z * in.radius);
    let clip = camera.projection * vec4<f32>(surface, 1.0);

    var out: FSOut;
    out.depth = clip.z / clip.w;
    out.color = vec4<f32>(in.color * shade, 1.0);
    return out;
}
