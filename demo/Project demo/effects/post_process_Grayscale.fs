#version 330 core
out vec4 FragColor;
in vec2 TexCoords;

uniform sampler2D screenTexture; // 场景渲染输出

void main()
{
    vec3 color = texture(screenTexture, TexCoords).rgb;
    float gray = dot(color, vec3(0.2126, 0.7152, 0.0722));
    FragColor = vec4(vec3(gray), 1.0);
}
