$input v_atomCount, v_maxCount

#include <bgfx_shader.sh>

void main() {
    float t = clamp(v_atomCount / max(v_maxCount, 1.0), 0.0, 1.0);
    vec3 color = mix(vec3(0.0, 1.0, 0.0), vec3(1.0, 0.0, 0.0), t);
    gl_FragColor = vec4(color, 0.3);
}