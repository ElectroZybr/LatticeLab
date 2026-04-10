$input a_position, a_texcoord0, a_texcoord1, a_texcoord2
$output v_atomCount, v_maxCount

#include <bgfx_shader.sh>

uniform vec4 u_maxCount;

void main() {
    vec3 origin   = a_texcoord0.xyz;
    float cell    = a_texcoord1.x;
    float count   = a_texcoord2.x;

    vec3 worldPos = origin + a_position * cell;
    gl_Position = mul(u_proj, mul(u_view, vec4(worldPos, 1.0)));

    v_atomCount = count;
    v_maxCount  = u_maxCount.x;
}