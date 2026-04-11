$input a_position, i_data0, i_data1, i_data2
$output v_fragColor, v_uv, v_isSelected, v_data0

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
    float posX   = i_data0.x;
    float posY   = i_data0.y;
    float posZ   = i_data0.z;
    float radius = i_data0.w;

    float velX   = i_data1.x;
    float velY   = i_data1.y;
    float velZ   = i_data1.z;
    int   aType  = int(i_data1.w);

    float sel    = i_data2.x;

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

    v_data0 = i_data0;
    v_fragColor  = color;
    v_uv         = a_position.xy;
    v_isSelected = sel;

    vec4 center = mul(u_view, vec4(posX, posY, posZ, 1.0));
    center.xy  += a_position.xy * radius;
    center.z   -= radius;
    gl_Position = mul(u_proj, center);
}
