#version 430 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;

out VS_OUT {
    vec3 FragPos;
    vec3 Normal;
    vec2 TexCoords;
    vec4 FragPosLightSpace[16]; // 对应每个光源的光空间坐标
} vs_out;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform mat4 lightSpaceMatrices[16]; // 每个光源的光空间矩阵
uniform int lightCount;

void main()
{
    vs_out.FragPos = vec3(model * vec4(aPos, 1.0));
    vs_out.Normal = mat3(transpose(inverse(model))) * aNormal;  
    vs_out.TexCoords = aTexCoords;
    
    // 计算每个光源的光空间坐标
    for(int i = 0; i < lightCount; i++) {
        vs_out.FragPosLightSpace[i] = lightSpaceMatrices[i] * vec4(vs_out.FragPos, 1.0);
    }
    
    gl_Position = projection * view * vec4(vs_out.FragPos, 1.0);
}