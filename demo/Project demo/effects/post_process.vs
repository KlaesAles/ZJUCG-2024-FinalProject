#version 330 core
layout (location = 0) in vec2 aPos;       // 屏幕空间上的顶点坐标
layout (location = 1) in vec2 aTexCoords; // 传入的纹理坐标

out vec2 TexCoords;

void main()
{
    TexCoords = aTexCoords;
    gl_Position = vec4(aPos, 0.0, 1.0);
}
