$input a_position
$output v_fragColor, v_uv, v_isSelected

#include <bgfx_shader.sh>
#include "gradient.sh"

SAMPLER2D(s_posX,   0);
SAMPLER2D(s_posY,   1);
SAMPLER2D(s_posZ,   2);
SAMPLER2D(s_velX,   3);
SAMPLER2D(s_velY,   4);
SAMPLER2D(s_velZ,   5);
SAMPLER2D(s_type,   6);
SAMPLER2D(s_radius, 7);
SAMPLER2D(s_sel,    8);

uniform vec4 u_maxSpeedSqr;
uniform vec4 u_colorMode;
uniform vec4 u_typeColors[119];

void main() {
    ivec2 coord = ivec2(gl_InstanceID % 4096, gl_InstanceID / 4096);

    float posX   = texelFetch(s_posX,   coord, 0).r;
    float posY   = texelFetch(s_posY,   coord, 0).r;
    float velX   = texelFetch(s_velX,   coord, 0).r;
    float velY   = texelFetch(s_velY,   coord, 0).r;
    float velZ   = texelFetch(s_velZ,   coord, 0).r;
    float radius = texelFetch(s_radius, coord, 0).r;
    int   aType  = int(texelFetch(s_type, coord, 0).r);
    float sel    = texelFetch(s_sel,    coord, 0).r;

    int mode = int(u_colorMode.x);
    vec3 color;
    if (mode == 0) {
        color = u_typeColors[aType].rgb;
    } else {
        float vSqr = velX*velX + velY*velY + velZ*velZ;
        float t    = clamp(sqrt(vSqr / u_maxSpeedSqr.x), 0.0, 1.0);
        color = (mode == 1) ? vec3(t, 0.0, 1.0 - t) : turboColor(t);
    }

    v_fragColor  = color;
    v_uv         = a_position.xy;
    v_isSelected = sel;

    vec2 offset = a_position.xy * radius;
    gl_Position = mul(u_proj, mul(u_view, vec4(posX + offset.x, posY + offset.y, 0.0, 1.0)));
}