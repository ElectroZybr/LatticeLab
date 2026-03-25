#version 330 core

in float vAtomCount;

uniform float uMaxCount;

out vec4 outColor;

void main() {
    float t = clamp(vAtomCount / uMaxCount, 0.0, 1.0);
    vec3 color = mix(vec3(0.0, 1.0, 0.0), vec3(1.0, 0.0, 0.0), t);
    outColor = vec4(color, 0.3 + 0.4 * t);
}