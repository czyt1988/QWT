#version 330

in vec3 vNormal;
in vec4 vColor;
in vec3 vFragPos;

out vec4 fragColor;

uniform bool uUseLighting;
uniform vec3 uLightPos;
uniform vec3 uLightAmbient;
uniform vec3 uLightDiffuse;
uniform vec3 uLightSpecular;
uniform vec3 uMaterialAmbient;
uniform vec3 uMaterialDiffuse;
uniform vec3 uMaterialSpecular;
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

    // Blinn-Phong lighting with the light/material components uploaded by
    // Qwt3DSurface::draw(). The defaults (light ambient 0.3, light diffuse
    // and specular white, material diffuse white, material specular 0.5,
    // shininess 32) reproduce the parameters that used to be hardcoded here.
    vec3 norm = normalize(vNormal);
    vec3 lightDir = normalize(uLightPos - vFragPos);

    // Ambient
    vec3 ambient = uLightAmbient * uMaterialAmbient * vColor.rgb;

    // Diffuse
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * uLightDiffuse * uMaterialDiffuse * vColor.rgb;

    // Specular (Blinn-Phong)
    vec3 viewDir = normalize(-vFragPos);
    vec3 halfwayDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(norm, halfwayDir), 0.0), uShininess);
    vec3 specular = spec * uLightSpecular * uMaterialSpecular;

    fragColor = vec4(ambient + diffuse + specular, vColor.a);
}
