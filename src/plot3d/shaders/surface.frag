#version 330

in vec3 vNormal;
in vec4 vColor;
in vec3 vFragPos;

out vec4 fragColor;

uniform bool uUseLighting;
uniform vec3 uLightPos;
uniform vec3 uLightColor;
uniform float uShininess;
uniform bool uUseOverrideColor;
uniform vec4 uOverrideColor;

void main()
{
    if (uUseOverrideColor) {
        fragColor = uOverrideColor;
        return;
    }

    if (!uUseLighting) {
        fragColor = vColor;
        return;
    }

    // Simple Blinn-Phong lighting
    vec3 norm = normalize(vNormal);
    vec3 lightDir = normalize(uLightPos - vFragPos);

    // Ambient
    vec3 ambient = 0.3 * vColor.rgb;

    // Diffuse
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * uLightColor * vColor.rgb;

    // Specular (Blinn-Phong)
    vec3 viewDir = normalize(-vFragPos);
    vec3 halfwayDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(norm, halfwayDir), 0.0), uShininess);
    vec3 specular = spec * uLightColor * 0.5;

    fragColor = vec4(ambient + diffuse + specular, vColor.a);
}
