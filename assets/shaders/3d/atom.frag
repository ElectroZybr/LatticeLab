#version 330 core
in vec3 fragAtomPos;
in float fragRadius;
in vec3 fragColor;
in vec2 fragQuadPos;
in float vIsSelected;

uniform vec3 lightDir;
uniform mat4 projection;
uniform mat4 view;

out vec4 outColor;

void main() {
    float d = dot(fragQuadPos, fragQuadPos);
    if (d > 1.0) discard;

    float z = sqrt(1.0 - d);
    vec3 normal = normalize(vec3(fragQuadPos, z));

    vec4 viewPos = view * vec4(fragAtomPos, 1.0);
    viewPos.z   += z * fragRadius;

    vec4 clipPos = projection * viewPos;
    gl_FragDepth = (clipPos.z / clipPos.w) * 0.5 + 0.5;

    vec3 light  = normalize(lightDir);
    float diff  = max(dot(normal, light), 0.0);
    vec3 refl   = reflect(-light, normal);
    float spec  = pow(max(dot(vec3(0,0,1), refl), 0.0), 32.0);

    vec3 ambient  = 0.2 * fragColor;
    vec3 diffuse  = 0.7 * diff * fragColor;
    vec3 specular = 0.4 * spec * vec3(1.0);

    vec3 color = ambient + diffuse + specular;

    if (vIsSelected > 0.5) {
        float rim = 1.0 - z;
        float ring = smoothstep(0.6, 0.7, rim);
        color = mix(color, vec3(1.0, 0.8, 0.0), ring);
    }

    outColor = vec4(color, 1.0);

}