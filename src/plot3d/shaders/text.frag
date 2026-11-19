#version 330

in vec2 vTexCoord;
in vec4 vColor;

out vec4 fragColor;

uniform sampler2D uTextTexture;

void main()
{
    float alpha = texture(uTextTexture, vTexCoord).a;
    fragColor   = vec4(vColor.rgb, vColor.a * alpha);
}
