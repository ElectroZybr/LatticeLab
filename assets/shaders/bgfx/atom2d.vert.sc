$input a_position, a_texcoord0, a_texcoord1, a_texcoord2
$output v_fragColor, v_uv, v_isSelected

#include <bgfx_shader.sh>

uniform vec4 u_maxSpeedSqr;
uniform vec4 u_colorMode;
uniform vec4 u_typeColors[119];

vec3 turboColor(float t) {
    t = clamp(t, 0.0, 1.0);
    float r = 34.61 + t * (1172.33 + t * (-10793.56 + t * (33300.12 + t * (-38394.49 + t * 14825.05))));
    float g = 23.31 + t * (557.33   + t * (1225.33   + t * (-3574.96  + t * (1073.77  + t * 707.56))));
    float b = 27.20 + t * (3211.10  + t * (-15327.97 + t * (27814.00  + t * (-22569.18 + t * 6838.66))));
    return clamp(vec3(r, g, b) / 255.0, 0.0, 1.0);
}

void main() {
    float posX   = a_texcoord0.x;
    float posY   = a_texcoord0.y;
    float radius = a_texcoord0.w;

    float velX   = a_texcoord1.x;
    float velY   = a_texcoord1.y;
    float velZ   = a_texcoord1.z;
    int   aType  = int(a_texcoord1.w);

    float sel    = a_texcoord2.x;

    int mode = int(u_colorMode.x);
    vec3 color;
    if (mode == 0) {
        color = u_typeColors[aType].rgb;
    } else {
        float vSqr = velX*velX + velY*velY + velZ*velZ;
        float t    = clamp(sqrt(vSqr / u_maxSpeedSqr.x), 0.0, 1.0);
        if (mode == 1) {
            color = vec3(t, 0.0, 1.0 - t);
        } else {
            color = turboColor(t);
        }
    }

    v_fragColor  = color;
    v_uv         = a_position.xy;
    v_isSelected = sel;

    vec2 screenOffset = a_position.xy * radius;
    gl_Position = mul(u_proj, mul(u_view, vec4(posX + screenOffset.x, posY + screenOffset.y, 0.0, 1.0)));
}
