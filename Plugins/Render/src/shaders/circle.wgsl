struct VSOut {
    @builtin(position) position: vec4<f32>,
    @location(0) uv: vec2<f32>,
};

@vertex
fn vs(@builtin(vertex_index) vertex: u32) -> VSOut {
    var positions = array<vec2<f32>, 6>(
        vec2<f32>(-1.0, -1.0),
        vec2<f32>( 1.0, -1.0),
        vec2<f32>( 1.0,  1.0),

        vec2<f32>(-1.0, -1.0),
        vec2<f32>( 1.0,  1.0),
        vec2<f32>(-1.0,  1.0)
    );

    let p = positions[vertex];

    var out: VSOut;
    out.position = vec4<f32>(p * 0.5, 0.0, 1.0);
    out.uv = p;
    return out;
}

@fragment
fn fs(in: VSOut) -> @location(0) vec4<f32> {
    if (length(in.uv) > 1.0) {
        discard;
    }

    return vec4<f32>(1.0, 0.2, 0.1, 1.0);
}