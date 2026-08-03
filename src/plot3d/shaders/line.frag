#version 330

in vec4 vColor;

out vec4 fragColor;

uniform float uLineWidth;

void main()
{
    fragColor = vColor;
}
