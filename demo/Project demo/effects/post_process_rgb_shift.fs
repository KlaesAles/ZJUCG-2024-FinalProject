#version 330 core

out vec4 FragColor;
in vec2 TexCoords;

uniform sampler2D screenTexture;
uniform float strength;  // 分离强度

void main()
{
    // 假设只在水平方向上分离
    // 你也可以改成 vec2( strength, strength ) 做更复杂的效果
    vec2 offset = vec2(strength, 0.0);

    float r = texture(screenTexture, TexCoords + offset).r;
    float g = texture(screenTexture, TexCoords).g;
    float b = texture(screenTexture, TexCoords - offset).b;

    FragColor = vec4(r, g, b, 1.0);
}
