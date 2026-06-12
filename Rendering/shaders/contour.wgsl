struct SceneUniforms {
    view       : mat4x4<f32>,
    projection : mat4x4<f32>,
    lightDir   : vec4<f32>,
    colorMode  : vec4<f32>,
    maxSpeedSqr: vec4<f32>,
    maxCount   : vec4<f32>,
    renderOffset: vec4<f32>,
    lineColor  : vec4<f32>,
    typeColors : array<vec4<f32>, 119>,
}
@group(0) @binding(0) var<uniform> uScene: SceneUniforms;

struct VertOut {
    @builtin(position) pos   : vec4<f32>,
    @location(0)       localPos : vec2<f32>,
    @location(1)       potentials : vec4<f32>,
}

@vertex
fn vs_main(
    @location(0) localPos  : vec2<f32>,
    @location(1) origin    : vec4<f32>,
    @location(2) cellSize  : vec2<f32>,
    @location(3) potentials : vec4<f32>,
) -> VertOut {
    let worldPos = origin.xyz + vec3<f32>(localPos * cellSize, 0.0) + uScene.renderOffset.xyz;
    var out: VertOut;
    out.pos = uScene.projection * uScene.view * vec4<f32>(worldPos, 1.0);
    out.localPos = localPos;
    out.potentials = potentials;
    return out;
}

@fragment
fn fs_main(in: VertOut) -> @location(0) vec4<f32> {
    let contourScale = max(uScene.maxCount.x, 0.0001);
    let contourStep = max(uScene.maxCount.z, 0.01);

    let pBottom = mix(in.potentials.x, in.potentials.y, in.localPos.x);
    let pTop = mix(in.potentials.z, in.potentials.w, in.localPos.x);
    let scaledPotential = mix(pBottom, pTop, in.localPos.y) / contourScale;

    let contourCoord = scaledPotential / contourStep;
    let contourPhase = fract(contourCoord);
    let contourDistance = min(contourPhase, 1.0 - contourPhase);
    let aa = max(fwidth(contourCoord), 0.0001);
    let contourMask = 1.0 - smoothstep(0.0, 1.0 * aa, contourDistance);
    let zeroMask = 1.0 - step(contourStep * 0.5, abs(scaledPotential));
    let visibleMask = contourMask * (1.0 - zeroMask);
    return vec4<f32>(uScene.lineColor.rgb, uScene.lineColor.a * visibleMask);
}
