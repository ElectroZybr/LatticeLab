#version 330 core
layout(location = 0) in vec2 quadPos;
layout(location = 1) in float posX;
layout(location = 2) in float posY;
layout(location = 3) in float posZ;
layout(location = 4) in float atomRadius;
layout(location = 5) in vec3 atomColor;
layout(location = 6) in int isSelected;

uniform mat4 projection;
uniform mat4 view;
uniform vec3 boxStart;

out vec3 fragAtomPos;
out float fragRadius;
out vec3 fragColor;
out vec2 fragQuadPos;
flat out int vIsSelected;

void main() {
    vec3 atomPos = vec3(posX, posY, posZ) + boxStart;

    fragAtomPos = atomPos;
    fragRadius  = atomRadius;
    fragColor   = atomColor;
    fragQuadPos = quadPos;
    vIsSelected = isSelected;

    vec4 center = view * vec4(atomPos, 1.0);
    center.xy  += quadPos * atomRadius;
    center.z   -= atomRadius;
    gl_Position = projection * center;
}