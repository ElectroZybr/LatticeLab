// A lit triangle mesh: vertices pulled from storage buffers by index, Blinn–Phong under
// the lights of the viewport, an optional wireframe from barycentric coordinates.
struct Camera {
    viewProjection: mat4x4<f32>,
    view: mat4x4<f32>,
    projection: mat4x4<f32>,
    position: vec4<f32>,
};
// Mirror LightsHeader and LightRecord (Viewport.hpp).
struct LightRecord {
    position: vec4<f32>,    // point: xyz, w 1; directional: direction of the rays, w 0
    color: vec4<f32>,       // rgb × intensity, w range
};
struct Lights {
    ambient: vec4<f32>,
    count: vec4<u32>,
    items: array<LightRecord>,
};
// Mirrors MeshVertex (MeshData.hpp).
struct Vertex {
    position: vec4<f32>,
    normal: vec4<f32>,
};
// Mirrors `Uniform` in MeshObject.cpp.
struct Object {
    model: mat4x4<f32>,
    normalMatrix: mat4x4<f32>,
    color: vec4<f32>,       // rgb, opacity
    wire: vec4<f32>,        // rgb, width in pixels
    material: vec4<f32>,    // specular, shininess
    flags: vec4<u32>,       // solid, wireframe
};
@group(0) @binding(0) var<uniform> camera: Camera;
@group(0) @binding(1) var<storage, read> lights: Lights;
@group(1) @binding(0) var<storage, read> vertices: array<Vertex>;
@group(1) @binding(1) var<storage, read> indices: array<u32>;
@group(1) @binding(2) var<uniform> object: Object;

struct VSOut {
    @builtin(position) position: vec4<f32>,
    @location(0) world: vec3<f32>,
    @location(1) normal: vec3<f32>,
    @location(2) barycentric: vec3<f32>,
};

@vertex
fn vs(@builtin(vertex_index) corner: u32) -> VSOut {
    let vertex = vertices[indices[corner]];
    let world = object.model * vec4<f32>(vertex.position.xyz, 1.0);
    var barycentric = vec3<f32>(0.0);
    barycentric[corner % 3u] = 1.0;

    var out: VSOut;
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
fn fs(in: VSOut, @builtin(front_facing) front: bool) -> @location(0) vec4<f32> {
    // Distance to the nearest edge in pixels, before any branch (derivatives).
    let pixels = in.barycentric / max(fwidth(in.barycentric), vec3<f32>(1e-6));
    let edge = 1.0 - smoothstep(0.0, 1.0, min(pixels.x, min(pixels.y, pixels.z)) - 0.5 * object.wire.w + 0.5);
    let wire = select(0.0, edge, object.flags.y != 0u);

    if (object.flags.x == 0u) {
        // Wireframe only: the faces are empty.
        if (wire <= 0.0) {
            discard;
        }
        return vec4<f32>(object.wire.rgb, wire * object.color.a);
    }
    // Both sides are lit: seen from behind, the face turns its normal to the camera.
    var normal = normalize(in.normal);
    if (!front) {
        normal = -normal;
    }
    let lit = shade(object.color.rgb, in.world, normal);
    return vec4<f32>(mix(lit, object.wire.rgb, wire), object.color.a);
}
