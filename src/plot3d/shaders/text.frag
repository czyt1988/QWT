#version 330

in vec2 vTexCoord;

out vec4 fragColor;

uniform sampler2D uTextTexture;
uniform vec4 uTextColor;

void main()
{
    float alpha = texture(uTextTexture, vTexCoord).a;
    fragColor = vec4(uTextColor.rgb, uTextColor.a * alpha);
}
