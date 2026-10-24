struct Camera { viewProjection : mat4x4<f32> }
@group(0) @binding(0) var<uniform> camera : Camera;
@group(1) @binding(0) var<storage, read> data : array<f32>;
@vertex fn vs(@builtin(vertex_index) i : u32, @builtin(instance_index) n : u32) -> @builtin(position) vec4<f32> {
    return camera.viewProjection * vec4<f32>(data[n], f32(i), 0.0, 1.0);
}
@fragment fn fs() -> @location(0) vec4<f32> { return vec4<f32>(1.0); }
