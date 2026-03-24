#version 330 core

layout(location = 0) in vec3 aPos_A;
layout(location = 1) in vec3 aPos_B;
layout(location = 2) in float aRadius;

out vec3 gPos_A;
out vec3 gPos_B;
out float gRadius;

void main() {
    gPos_A  = aPos_A;
    gPos_B  = aPos_B;
    gRadius = aRadius;
    gl_Position = vec4(0.0);
}