#version 330 core

layout(location = 0) in vec2 quadPos;
layout(location = 1) in vec3 pos;
layout(location = 2) in float radius;
layout(location = 3) in vec3 color;
layout(location = 4) in float isSelected;  

uniform mat4 projection;
uniform mat4 view;

out float vIsSelected;
out vec3 fragColor;
out vec2 uv;

void main() {
    fragColor = color;
    uv = quadPos;
    vIsSelected = isSelected;
    vec2 screenOffset = quadPos * radius;
    gl_Position = projection * view * vec4(pos.xy + screenOffset, 0.0, 1.0);
}