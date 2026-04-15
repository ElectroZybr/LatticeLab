$input v_fragColor, v_uv, v_isSelected

#include <bgfx_shader.sh>

void main() {
    float d2 = dot(v_uv, v_uv);
    if (d2 > 1.0) discard;

    float d = sqrt(d2);
    float outline = step(0.9, d);

    vec3 outlineColor = vec3(0.05, 0.05, 0.05);
    if (v_isSelected > 0.5) {
        outlineColor = vec3(0.95, 0.72, 0.28);
    }

    vec3 color = mix(v_fragColor, outlineColor, outline);
    gl_FragColor = vec4(color, 1.0);
}
