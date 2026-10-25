// Faces lit by the viewport's lights (or a flat fill), and the lines of the shape: creases,
// silhouettes and boundaries of the edge list, drawn as screen-space quads.
struct Camera {
    viewProjection: mat4x4<f32>,
    view: mat4x4<f32>,
    projection: mat4x4<f32>,
    position: vec4<f32>,
    viewport: vec4<f32>,
};
struct LightRecord {
    position: vec4<f32>,
    color: vec4<f32>,
};
struct Lights {
    ambient: vec4<f32>,
    count: vec4<u32>,
    items: array<LightRecord>,
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
// Mirrors `Uniform` in MeshObject.cpp.
struct Object {
    model: mat4x4<f32>,
    normalMatrix: mat4x4<f32>,
    color: vec4<f32>,       // rgb, opacity
    fill: vec4<f32>,        // rgb, w > 0: lit faces instead
    wire: vec4<f32>,        // triangles: rgb, width (0: off)
    edge: vec4<f32>,        // visible lines: rgb, width
    hidden: vec4<f32>,      // hidden lines: rgb, width
    material: vec4<f32>,    // specular, shininess, cos of the crease angle, dash length
};
@group(0) @binding(0) var<uniform> camera: Camera;
@group(0) @binding(1) var<storage, read> lights: Lights;
@group(1) @binding(0) var<storage, read> vertices: array<Vertex>;
@group(1) @binding(1) var<storage, read> indices: array<u32>;
@group(1) @binding(2) var<uniform> object: Object;
@group(1) @binding(3) var<storage, read> edges: array<Edge>;

struct FaceOut {
    @builtin(position) position: vec4<f32>,
    @location(0) world: vec3<f32>,
    @location(1) normal: vec3<f32>,
    @location(2) barycentric: vec3<f32>,
};

@vertex
fn vs(@builtin(vertex_index) corner: u32) -> FaceOut {
    let vertex = vertices[indices[corner]];
    let world = object.model * vec4<f32>(vertex.position.xyz, 1.0);
    var barycentric = vec3<f32>(0.0);
    barycentric[corner % 3u] = 1.0;

    var out: FaceOut;
    out.position = camera.viewProjection * world;
    out.world = world.xyz;
    out.normal = (object.normalMatrix * vec4<f32>(vertex.normal.xyz, 0.0)).xyz;
    out.barycentric = barycentric;
    return out;
}

fn shade(base: vec3<f32>, world: vec3<f32>, normal: vec3<f32>) -> vec3<f32> {
    let toCamera = normalize(camera.position.xyz - world);
    var color = lights.ambient.rgb * base;
    for (var i = 0u; i < lights.count.x; i++) {
        let light = lights.items[i];
        var toLight = -light.position.xyz;
        var radiance = light.color.rgb;
        if (light.position.w > 0.0) {
            let offset = light.position.xyz - world;
            let distance = length(offset);
            toLight = offset / max(distance, 1e-6);
            let fade = distance / light.color.w;
            radiance = radiance / (1.0 + fade * fade);
        }
        let diffuse = max(dot(normal, toLight), 0.0);
        let halfway = normalize(toLight + toCamera);
        let specular = select(0.0, pow(max(dot(normal, halfway), 0.0), object.material.y), diffuse > 0.0);
        color += radiance * (base * diffuse + object.material.x * specular);
    }
    return color;
}

@fragment
fn fs(in: FaceOut, @builtin(front_facing) front: bool) -> @location(0) vec4<f32> {
    let pixels = in.barycentric / max(fwidth(in.barycentric), vec3<f32>(1e-6));
    let edge = 1.0 - smoothstep(0.0, 1.0, min(pixels.x, min(pixels.y, pixels.z)) - 0.5 * object.wire.w + 0.5);
    let wire = select(0.0, edge, object.wire.w > 0.0);
    var color = object.fill.rgb;
    if (object.fill.w > 0.0) {
        var normal = normalize(in.normal);
        if (!front) { normal = -normal; }
        color = shade(object.color.rgb, in.world, normal);
    }
    return vec4<f32>(mix(color, object.wire.rgb, wire), object.color.a);
}

struct LineOut {
    @builtin(position) position: vec4<f32>,
    @location(0) along: f32,
    @location(1) across: f32,
};

fn worldNormal(n: vec4<f32>) -> vec3<f32> {
    return normalize((object.normalMatrix * vec4<f32>(n.xyz, 0.0)).xyz);
}

// A crease, a silhouette or a boundary as a quad `width` pixels wide; other edges
// collapse to nothing. Ends move slightly towards the camera to win against their faces.
fn line(index: u32, width: f32) -> LineOut {
    var out: LineOut;
    out.position = vec4<f32>(2.0, 2.0, 2.0, 1.0);
    let edge = edges[index / 6u];
    var a = (object.model * vec4<f32>(vertices[edge.a].position.xyz, 1.0)).xyz;
    var b = (object.model * vec4<f32>(vertices[edge.b].position.xyz, 1.0)).xyz;
    let n0 = worldNormal(edge.first);
    let toCamera = camera.position.xyz - 0.5 * (a + b);
    var feature = edge.second.w == 0.0;
    if (!feature) {
        let n1 = worldNormal(edge.second);
        feature = dot(n0, n1) < object.material.z || (dot(n0, toCamera) > 0.0) != (dot(n1, toCamera) > 0.0);
    }
    if (!feature) { return out; }

    a += (camera.position.xyz - a) * 1e-3;
    b += (camera.position.xyz - b) * 1e-3;
    let ca = camera.viewProjection * vec4<f32>(a, 1.0);
    let cb = camera.viewProjection * vec4<f32>(b, 1.0);
    if (ca.w <= 1e-4 || cb.w <= 1e-4) { return out; }
    let half = 0.5 * camera.viewport.xy;
    let sa = ca.xy / ca.w * half;
    let sb = cb.xy / cb.w * half;
    let span = length(sb - sa);
    let direction = select(vec2<f32>(1.0, 0.0), (sb - sa) / span, span > 1e-6);
    let side = vec2<f32>(-direction.y, direction.x);

    let corners = array<vec2<f32>, 6>(
        vec2<f32>(0.0, -1.0), vec2<f32>(1.0, -1.0), vec2<f32>(1.0, 1.0),
        vec2<f32>(0.0, -1.0), vec2<f32>(1.0, 1.0), vec2<f32>(0.0, 1.0));
    let corner = corners[index % 6u];
    let reach = 0.5 * width + 1.0;
    var position = mix(ca, cb, corner.x);
    position = vec4<f32>(position.xy + side * corner.y * reach / half * position.w, position.zw);
    out.position = position;
    out.along = corner.x * span;
    out.across = corner.y * reach;
    return out;
}

@vertex
fn vsEdge(@builtin(vertex_index) index: u32) -> LineOut { return line(index, object.edge.w); }

@vertex
fn vsHidden(@builtin(vertex_index) index: u32) -> LineOut { return line(index, object.hidden.w); }

fn coverage(across: f32, width: f32) -> f32 {
    return clamp(0.5 * width + 0.5 - abs(across), 0.0, 1.0);
}

@fragment
fn fsEdge(in: LineOut) -> @location(0) vec4<f32> {
    return vec4<f32>(object.edge.rgb, coverage(in.across, object.edge.w));
}

@fragment
fn fsHidden(in: LineOut) -> @location(0) vec4<f32> {
    if (object.material.w > 0.0 && fract(in.along / object.material.w) > 0.5) { discard; }
    return vec4<f32>(object.hidden.rgb, coverage(in.across, object.hidden.w));
}
