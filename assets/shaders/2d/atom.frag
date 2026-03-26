#version 330 core

in vec3 fragColor;
in vec2 uv;
flat in int vIsSelected;

out vec4 outColor;

void main() {
    float d2 = dot(uv, uv);
    if (d2 > 1.0) discard;

    float d = sqrt(d2);

    float outline = step(0.9, d);

    vec3 outlineColor = vec3(0.05, 0.05, 0.05);
    if (vIsSelected != 0) {
        outlineColor = vec3(0.95, 0.72, 0.28);
    }

    vec3 color = mix(fragColor, outlineColor, outline);

    outColor = vec4(color, 1.0);
}
