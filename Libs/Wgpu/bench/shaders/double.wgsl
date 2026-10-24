@group(0) @binding(0) var<storage, read_write> data : array<f32>;
@compute @workgroup_size(256)
fn main(@builtin(global_invocation_id) id : vec3<u32>) {
    if (id.x < arrayLength(&data)) {
        data[id.x] = data[id.x] * 1.0001 + 1.0;
    }
}
