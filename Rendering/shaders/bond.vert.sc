$input a_position

#include <bgfx_shader.sh>

void main() {
    gl_Position = mul(u_proj, mul(u_view, a_position));
}
