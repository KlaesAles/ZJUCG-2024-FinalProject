#version 330 core

layout(location = 0) in vec3 aPos; // the position variable has attribute position 0
// layout(location = 1) in vec3 aColor; // the color variable has attribute position 1
layout(location = 1) in vec2 aTexCoord; // the texture variable has attribute position 2

// out vec3 ourColor; // specify a color output to the fragment shader
out vec2 TexCoord; // specify a texture coordinate output to the fragment shader

//uniform float xOffset;
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
    //gl_Position = vec4(aPos.x + xOffset, -aPos.y, aPos.z, 1.0);
    gl_Position = projection * view * model * vec4(aPos, 1.0); // see how we directly give aPos to gl_Position
    // ourColor = aColor; // set ourColor to the input color we got from the vertex data
    TexCoord = aTexCoord; // set TexCoord to the input texture coordinate
}