#version 330 core

in vec3 fNormal;
in vec3 fFragPos;

uniform vec3 lightDir;
uniform vec3 bondColor;

out vec4 FragColor;

void main() {
    vec3 norm = normalize(fNormal);
    vec3 ld   = normalize(lightDir);

    float ambient  = 0.25;
    float diffuse  = max(dot(norm, ld), 0.0) * 0.75;

    vec3  viewDir  = normalize(-fFragPos);
    vec3  halfDir  = normalize(ld + viewDir);
    float specular = pow(max(dot(norm, halfDir), 0.0), 32.0) * 0.4;

    vec3 result = (ambient + diffuse) * bondColor + specular * vec3(1.0);
    FragColor = vec4(result, 1.0);
}