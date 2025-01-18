#version 330 core
in vec4 FragPos;

uniform vec3 lightPos;
uniform float far_plane;

void main()
{
    // 计算片段与光源之间的距离
    float lightDistance = length(FragPos.xyz - lightPos);

    // 将距离映射到 [0, 1] 范围
    lightDistance = lightDistance / far_plane;

    // 将映射后的距离作为修改后的深度写入
    gl_FragDepth = lightDistance;
}
