#version 330 core
out vec4 FragColor;
in vec2 TexCoords;

uniform sampler2D screenTexture;

void main()
{
    vec3 color = texture(screenTexture, TexCoords).rgb;
    vec3 inverted = vec3(1.0) - color; 
    FragColor = vec4(inverted, 1.0);
}
