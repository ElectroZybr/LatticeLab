$input a_position

#include <bgfx_shader.sh>

void main() {
    gl_Position = mul(u_proj, mul(u_view, vec4(a_position, 1.0)));
}
