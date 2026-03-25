#version 330 core

layout(points) in;
layout(triangle_strip, max_vertices = 98) out;  // 14 сегментов: 30 боковина + 34 + 34 крышки

in vec3  gPos_A[];
in vec3  gPos_B[];
in float gRadius[];

uniform mat4 projection;
uniform mat4 view;

out vec3 fNormal;
out vec3 fFragPos;

const int SEGMENTS = 14;
const float PI = 3.14159265358979;

void buildBasis(vec3 axis, out vec3 u, out vec3 v) {
    vec3 up = abs(axis.y) < 0.99 ? vec3(0,1,0) : vec3(1,0,0);
    u = normalize(cross(axis, up));
    v = cross(axis, u);
}

void emitVertex(vec3 worldPos, vec3 normal) {
    fNormal   = normalize(mat3(transpose(inverse(view))) * normal);
    fFragPos  = vec3(view * vec4(worldPos, 1.0));
    gl_Position = projection * view * vec4(worldPos, 1.0);
    EmitVertex();
}

void main() {
    vec3 A = gPos_A[0];
    vec3 B = gPos_B[0];
    float r = gRadius[0];

    vec3 axis = B - A;
    float len = length(axis);
    if (len < 0.0001) return;
    vec3 dir = axis / len;

    vec3 u, v;
    buildBasis(dir, u, v);

    for (int i = 0; i <= SEGMENTS; ++i) {
        float angle = 2.0 * PI * float(i) / float(SEGMENTS);
        float c = cos(angle), s = sin(angle);
        vec3 normal = c * u + s * v;
        vec3 offset = r * normal;

        emitVertex(A + offset, normal);
        emitVertex(B + offset, normal);
    }
    EndPrimitive();

    emitVertex(A, -dir);
    for (int i = 0; i <= SEGMENTS; ++i) {
        float angle = 2.0 * PI * float(i) / float(SEGMENTS);
        vec3 offset = r * (cos(angle) * u + sin(angle) * v);
        emitVertex(A + offset, -dir);
        emitVertex(A, -dir);
    }
    EndPrimitive();

    emitVertex(B, dir);
    for (int i = 0; i <= SEGMENTS; ++i) {
        float angle = 2.0 * PI * float(i) / float(SEGMENTS);
        vec3 offset = r * (cos(angle) * u + sin(angle) * v);
        emitVertex(B + offset, dir);
        emitVertex(B, dir);
    }
    EndPrimitive();
}
