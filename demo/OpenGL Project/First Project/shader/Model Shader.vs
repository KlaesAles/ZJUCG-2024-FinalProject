#version 430 core

layout (location = 0) in vec3 aPos;      // 顶点位置
layout (location = 1) in vec3 aNormal;   // 法线
layout (location = 2) in vec2 aTexCoords;// 纹理坐标

out VS_OUT {
    vec3 FragPos;                          // 世界空间中的片段位置
    vec3 Normal;                           // 世界空间中的法线
    vec2 TexCoords;                        // 纹理坐标
    vec4 FragPosLightSpace[16];            // 每个光源的光空间坐标
} fs_out;

uniform mat4 model;                        // 模型矩阵
uniform mat4 view;                         // 视图矩阵
uniform mat4 projection;                   // 投影矩阵
uniform mat4 lightSpaceMatrices[16];       // 每个光源的光空间矩阵
uniform int lightCount;                    // 当前光源数量

uniform sampler2D height[16];              // 位移纹理
uniform int heightCount;                   // 位移纹理数量

void main()
{
    // 计算世界空间中的顶点位置
    vec4 worldPos = model * vec4(aPos, 1.0);
    fs_out.FragPos = vec3(worldPos);
    
    // 计算并传递世界空间中的法线
    fs_out.Normal = mat3(transpose(inverse(model))) * aNormal;
    
    // 传递纹理坐标
    fs_out.TexCoords = aTexCoords;
    
    // 计算每个光源的光空间坐标
    for (int i = 0; i < lightCount; i++) {
        fs_out.FragPosLightSpace[i] = lightSpaceMatrices[i] * vec4(aPos, 1.0);
    }


    // 位移计算
    float displacement = 0.0;
    for (int i = 0; i < heightCount; ++i) {
        displacement += texture(height[i], aTexCoords).r;
    }
    displacement = displacement / float(heightCount);  // 平均位移
    displacement *= 0.1;  // 位移强度调整

    // 基于法线的位移
    if (heightCount > 0) {
        fs_out.FragPos = fs_out.FragPos + fs_out.Normal * displacement;
    }

    // 计算裁剪空间位置
    gl_Position = projection * view * worldPos;
}
