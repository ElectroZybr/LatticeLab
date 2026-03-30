#version 330 core
layout(location = 0) in vec2 quadPos;
layout(location = 1) in float posX;
layout(location = 2) in float posY;
layout(location = 3) in float posZ;
layout(location = 4) in float atomRadius;
layout(location = 5) in float velX;
layout(location = 6) in float velY;
layout(location = 7) in float velZ;
layout(location = 8) in uint  atomType;
layout(location = 9) in int isSelected;

uniform mat4 projection;
uniform mat4 view;
uniform vec3 boxStart;

// режим окраски
uniform int   colorMode; // 0 = AtomColor, 1 = GradientClassic, 2 = GradientTurbo
uniform float maxSpeedSqr;
uniform vec3  typeColors[119];  // цвета по типу атомов

out vec3 fragAtomPos;
out float fragRadius;
out vec3 fragColor;
out vec2 fragQuadPos;
flat out int vIsSelected;

vec3 turboColor(float t) {
    t = clamp(t, 0.0, 1.0);
    float r = 34.61 + t * (1172.33 + t * (-10793.56 + t * (33300.12 + t * (-38394.49 + t * 14825.05))));
    float g = 23.31 + t * (557.33  + t * (1225.33   + t * (-3574.96  + t * (1073.77   + t * 707.56))));
    float b = 27.20 + t * (3211.10 + t * (-15327.97 + t * (27814.00  + t * (-22569.18 + t * 6838.66))));
    return clamp(vec3(r, g, b) / 255.0, 0.0, 1.0);
}

void main() {
    vec3 atomPos = vec3(posX, posY, posZ) + boxStart;

    vec3 color;
    if (colorMode == 0) {
        color = typeColors[atomType];
    }
    else {
        float vSqr = velX*velX + velY*velY + velZ*velZ;
        float t    = clamp(sqrt(vSqr / maxSpeedSqr), 0.0, 1.0);
        if (colorMode == 1)
            color = vec3(t, 0.0, 1.0 - t);
        else
            color = turboColor(t);
    }

    fragAtomPos = atomPos;
    fragRadius  = atomRadius;
    fragColor   = color;
    fragQuadPos = quadPos;
    vIsSelected = isSelected;

    vec4 center = view * vec4(atomPos, 1.0);
    center.xy  += quadPos * atomRadius;
    center.z   -= atomRadius;
    gl_Position = projection * center;
}