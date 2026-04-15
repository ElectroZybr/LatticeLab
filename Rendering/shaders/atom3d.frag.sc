$input v_fragColor, v_uv, v_isSelected, v_data0

#include <bgfx_shader.sh>

uniform vec4 u_lightDir;

void main() {
    float d = dot(v_uv, v_uv);
    if (d > 1.0) discard;

    float z = sqrt(1.0 - d);
    vec3 normal = normalize(vec3(v_uv, z));

    vec3 atomPos = v_data0.xyz;
    float radius  = v_data0.w;

    vec4 viewPos = mul(u_view, vec4(atomPos, 1.0));
    viewPos.z += z * radius;

    vec4 clipPos = mul(u_proj, viewPos);
    gl_FragDepth = (clipPos.z / clipPos.w) * 0.5 + 0.5;

    vec3 light  = normalize(u_lightDir.xyz);
    float diff  = max(dot(normal, light), 0.0);
    vec3 refl   = reflect(-light, normal);
    float spec  = pow(max(dot(vec3(0.0, 0.0, 1.0), refl), 0.0), 32.0);

    vec3 ambient  = 0.2 * v_fragColor;
    vec3 diffuse  = 0.7 * diff * v_fragColor;
    vec3 specular = 0.4 * spec * vec3(1.0, 1.0, 1.0);

    vec3 color = ambient + diffuse + specular;

    if (v_isSelected > 0.5) {
        float rim  = 1.0 - z;
        float ring = smoothstep(0.6, 0.7, rim);
        color = mix(color, vec3(0.95, 0.72, 0.28), ring);
    }

    gl_FragColor = vec4(color, 1.0);
}