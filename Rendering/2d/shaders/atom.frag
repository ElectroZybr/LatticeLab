#version 330 core

in vec3 fragColor;
in vec2 uv;
in float vIsSelected;

out vec4 outColor;

void main() {
    float d2 = dot(uv, uv);
    if (d2 > 1.0) discard;

    float d = sqrt(d2);

    float outline = smoothstep(0.7, 1.0, d);

    vec3 outlineColor = vec3(0.0, 0.0, 0.0);
    if (vIsSelected > 0.5) {
        outlineColor = vec3(1.0, 1.0, 1.0);
    }

    vec3 color = mix(fragColor, outlineColor, outline);

    outColor = vec4(color, 1.0);
}