#version 330 core
layout(location = 0) in vec2 quadPos;
layout(location = 1) in vec3 atomPos;
layout(location = 2) in float atomRadius;
layout(location = 3) in vec3 atomColor;
layout(location = 4) in float isSelected;

uniform mat4 projection;
uniform mat4 view;

out vec3 fragAtomPos;
out float fragRadius;
out vec3 fragColor;
out vec2 fragQuadPos;
out float vIsSelected;

void main() {
    fragAtomPos = atomPos;
    fragRadius  = atomRadius;
    fragColor   = atomColor;
    fragQuadPos = quadPos;
    vIsSelected  = isSelected;

    vec4 center = view * vec4(atomPos, 1.0);
    center.xy  += quadPos * atomRadius;
    center.z   -= atomRadius;
    gl_Position = projection * center;
}