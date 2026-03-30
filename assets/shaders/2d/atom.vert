#version 330 core

layout(location = 0) in vec2 quadPos;
layout(location = 1) in float posX;
layout(location = 2) in float posY;
layout(location = 3) in float posZ;
layout(location = 4) in float radius;
layout(location = 5) in float velX;
layout(location = 6) in float velY;
layout(location = 7) in float velZ;
layout(location = 8) in uint  atomType;
layout(location = 9) in int   isSelected;

uniform vec3  boxStart;
uniform mat4  projection;
uniform mat4  view;
uniform float maxSpeedSqr;
uniform vec3  typeColors[119];

flat out int vIsSelected;
out vec3 fragColor;
out vec2 uv;

#ifndef COLOR_MODE
#define COLOR_MODE 0
#endif

vec3 turboColor(float t) {
    t = clamp(t, 0.0, 1.0);
    float r = 34.61 + t * (1172.33 + t * (-10793.56 + t * (33300.12 + t * (-38394.49 + t * 14825.05))));
    float g = 23.31 + t * (557.33  + t * (1225.33   + t * (-3574.96  + t * (1073.77   + t * 707.56))));
    float b = 27.20 + t * (3211.10 + t * (-15327.97 + t * (27814.00  + t * (-22569.18 + t * 6838.66))));
    return clamp(vec3(r, g, b) / 255.0, 0.0, 1.0);
}

void main() {
    vec3 color;
#if COLOR_MODE == 0
    color = typeColors[atomType];
#elif COLOR_MODE == 1
    float vSqr = velX*velX + velY*velY + velZ*velZ;
    float t    = clamp(sqrt(vSqr / maxSpeedSqr), 0.0, 1.0);
    color = vec3(t, 0.0, 1.0 - t);
#else
    float vSqr = velX*velX + velY*velY + velZ*velZ;
    float t    = clamp(sqrt(vSqr / maxSpeedSqr), 0.0, 1.0);
    color = turboColor(t);
#endif

    fragColor = color;
    uv = quadPos;
    vIsSelected = isSelected;
    vec2 screenOffset = quadPos * radius;
    gl_Position = projection * view * vec4(vec2(posX, posY) + boxStart.xy + screenOffset, 0.0, 1.0);
}
