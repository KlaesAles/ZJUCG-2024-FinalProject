#version 330 core

// in vec3 ourColor;
in vec2 TexCoord;

out vec4 FragColor;

uniform sampler2D texture1;
uniform sampler2D texture2;

//uniform vec4 ourColor; // we set this variable in the OpenGL code

void main()
{
    //FragColor = abs(ourColor + vertexColor);
    FragColor = mix(texture(texture1, TexCoord), texture(texture2, TexCoord), 0.2);
}