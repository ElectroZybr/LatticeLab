// Surface of a geometry program (Boundaries), sphere traced inside the box of its space.
// Prepended: the geometry machine (Boundaries::Wgpu::geometrySource()) and `traceDistance`
// — the interpreted or the generated distance of the program (Codegen.hpp).
// A full-screen triangle per viewport: each pixel casts a camera ray, clips it to the
// box and marches the program to d = 0; the hit is lit and writes its true depth.
struct Camera {
    viewProjection: mat4x4<f32>,
    view: mat4x4<f32>,
    projection: mat4x4<f32>,
    position: vec4<f32>,
};
// Mirrors `Params` in GeometrySurface.cpp.
struct Params {
    boxMin: vec4<f32>,
    boxMax: vec4<f32>,
    regionColor: vec4<f32>,   // the surface seen from the region (inside the container)
    wallColor: vec4<f32>,     // the surface seen from the wall
    cullWallSide: u32,        // 1: rays pass the surface seen from the wall (a cutaway)
    maxSteps: u32,
    pad0: u32,
    pad1: u32,
};
@group(0) @binding(0) var<uniform> camera: Camera;
@group(1) @binding(0) var<storage, read> geometryCode: array<Instruction>;
@group(1) @binding(1) var<uniform> params: Params;

struct VSOut {
    @builtin(position) position: vec4<f32>,
    @location(0) ndc: vec2<f32>,
};

@vertex
fn vs(@builtin(vertex_index) vertex: u32) -> VSOut {
    var corners = array<vec2<f32>, 3>(vec2<f32>(-1.0, -1.0), vec2<f32>(3.0, -1.0), vec2<f32>(-1.0, 3.0));
    var out: VSOut;
    out.position = vec4<f32>(corners[vertex], 0.0, 1.0);
    out.ndc = corners[vertex];
    return out;
}

struct FSOut {
    @builtin(frag_depth) depth: f32,
    @location(0) color: vec4<f32>,
};

// Ray parameters [near, far] inside the box; near > far when the ray misses it.
fn boxInterval(origin: vec3<f32>, direction: vec3<f32>) -> vec2<f32> {
    let safe = select(direction, vec3<f32>(1e-12), abs(direction) < vec3<f32>(1e-12));
    let a = (params.boxMin.xyz - origin) / safe;
    let b = (params.boxMax.xyz - origin) / safe;
    let lo = min(a, b);
    let hi = max(a, b);
    return vec2<f32>(max(lo.x, max(lo.y, lo.z)), min(hi.x, min(hi.y, hi.z)));
}

@fragment
fn fs(in: VSOut) -> FSOut {
    // Camera ray in world space: view-space direction through the pixel, rotated by the
    // camera (rows of the view matrix are its axes).
    let right = vec3<f32>(camera.view[0][0], camera.view[1][0], camera.view[2][0]);
    let up = vec3<f32>(camera.view[0][1], camera.view[1][1], camera.view[2][1]);
    let back = vec3<f32>(camera.view[0][2], camera.view[1][2], camera.view[2][2]);
    let local = vec3<f32>(in.ndc.x / camera.projection[0][0], in.ndc.y / camera.projection[1][1], -1.0);
    let direction = normalize(local.x * right + local.y * up + local.z * back);
    let origin = camera.position.xyz;
    // Angular size of a pixel: the surface is reached within a pixel's footprint.
    let pixel = length(fwidth(direction));

    let span = boxInterval(origin, direction);
    var t = max(span.x, 0.0);
    if (t > span.y) {
        discard;
    }

    // Sphere tracing on distances only; the full sample (normal) only near the surface.
    // Over-relaxed steps (ω > 1) cross open space faster; a step that overshoots (the
    // spheres of two points do not overlap) is taken back and retried plainly.
    var hit = false;
    var sample: GeometrySample;
    var omega = 1.5;
    var previous = 0.0;
    var advance = 0.0;
    for (var step = 0u; step < params.maxSteps && t <= span.y; step++) {
        let distance = abs(traceDistance(origin + direction * t));
        if (omega > 1.0 && distance + previous < advance) {
            t -= advance - previous;
            advance = previous;
            omega = 1.0;
            continue;
        }
        let epsilon = max(pixel * t, 1e-5);
        if (distance < epsilon) {
            sample = sampleGeometry(origin + direction * t);
            // The normal points into the region: a ray against it comes from the region.
            let facing = dot(sample.normal, direction);
            if (params.cullWallSide == 0u || facing < 0.0) {
                hit = true;
                break;
            }
            // Cut away: leave the band |d| < ε at once, its length along the ray is ε / cos.
            t += 2.0 * epsilon / max(facing, 0.05);
            previous = 0.0;
            advance = 0.0;
            omega = 1.0;
            continue;
        }
        previous = distance;
        advance = omega * distance;
        t += advance;
    }
    if (!hit) {
        discard;
    }

    let point = origin + direction * t;
    let fromRegion = dot(sample.normal, direction) < 0.0;
    let normal = select(-sample.normal, sample.normal, fromRegion);
    let light = normalize(vec3<f32>(0.3, 0.7, 0.4) - direction);
    let shade = 0.25 + 0.75 * max(dot(normal, light), 0.0);
    let base = select(params.wallColor, params.regionColor, fromRegion);
    let clip = camera.viewProjection * vec4<f32>(point, 1.0);

    var out: FSOut;
    out.depth = clip.z / clip.w;
    out.color = vec4<f32>(base.rgb * shade, 1.0);
    return out;
}
