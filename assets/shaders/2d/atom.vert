#version 330 core

layout(location = 0) in vec2 quadPos;
layout(location = 1) in float posX;
layout(location = 2) in float posY;
layout(location = 3) in float posZ;
layout(location = 4) in float radius;
layout(location = 5) in vec3 color;
layout(location = 6) in int isSelected;

uniform vec3 boxStart;
uniform mat4 projection;
uniform mat4 view;

flat out int vIsSelected;
out vec3 fragColor;
out vec2 uv;

void main() {
    fragColor = color;
    uv = quadPos;
    vIsSelected = isSelected;
    vec2 screenOffset = quadPos * radius;
    gl_Position = projection * view * vec4(vec2(posX, posY) + boxStart.xy + screenOffset, 0.0, 1.0);
}