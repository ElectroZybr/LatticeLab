$input a_position, i_data0, i_data1
$output v_atomCount, v_maxCount

#include <bgfx_shader.sh>

uniform vec4 u_maxCount;

void main() {
    vec3 origin = i_data0.xyz;
    float cell  = i_data0.w;
    float count = i_data1.x;

    vec3 worldPos = origin + a_position.xyz * cell;
    gl_Position = mul(u_proj, mul(u_view, vec4(worldPos, 1.0)));

    v_atomCount = count;
    v_maxCount  = u_maxCount.x;
}