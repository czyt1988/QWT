#version 330

layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec4 aColor;

uniform mat4 uModelView;
uniform mat4 uProjection;
uniform mat3 uNormalMatrix;

out vec3 vNormal;
out vec4 vColor;
out vec3 vFragPos;

void main()
{
    vec4 viewPos = uModelView * vec4(aPosition, 1.0);
    vFragPos = viewPos.xyz;
    vNormal = normalize(uNormalMatrix * aNormal);
    vColor = aColor;
    gl_Position = uProjection * viewPos;
}
