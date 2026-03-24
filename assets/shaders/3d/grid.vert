#version 330 core

// 24 вершины куба (12 рёбер * 2) — статический буфер
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aCellOrigin;  // левый нижний угол ячейки
layout(location = 2) in float aCellSize;
layout(location = 3) in float aAtomCount;

uniform mat4 projection;
uniform mat4 view;

out float vAtomCount;

void main() {
    vAtomCount = aAtomCount;
    vec3 worldPos = aCellOrigin + aPos * aCellSize;
    gl_Position = projection * view * vec4(worldPos, 1.0);
}