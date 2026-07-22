#version 330

in vec4 vColor;

out vec4 fragColor;

uniform float uAlpha;

void main()
{
    fragColor = vec4(vColor.rgb, vColor.a * uAlpha);
}
