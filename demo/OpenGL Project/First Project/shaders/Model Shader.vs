#version 430 core

layout (location = 0) in vec3 aPos;      // 顶点位置
layout (location = 1) in vec3 aNormal;   // 法线
layout (location = 2) in vec2 aTexCoords;// 纹理坐标
layout (location = 3) in vec3 aTangent;  // 切线
layout (location = 4) in vec3 aBitangent;// 副切线
layout (location = 5) in ivec4 aBoneIDs;
layout (location = 6) in vec4 aWeights;

out VS_OUT {
    vec3 FragPos;                          // 世界空间中的片段位置
    vec3 Normal;                           // 世界空间中的法线
    vec2 TexCoords;                        // 纹理坐标
    vec4 FragPosLightSpace[16];            // 每个光源的光空间坐标
} fs_out;

out vec3 Tangent;
out vec3 Bitangent;

uniform mat4 model;                        // 模型矩阵
uniform mat4 view;                         // 视图矩阵
uniform mat4 projection;                   // 投影矩阵
uniform mat4 lightSpaceMatrices[16];       // 每个光源的光空间矩阵
uniform int lightCount;                    // 当前光源数量

const int MAX_BONES = 100;
uniform mat4 bones[MAX_BONES];
uniform bool useBones;  // 是否使用骨骼动画的开关

void main()
{
    // 条件应用骨骼变换
    mat4 boneTransform = mat4(1.0);
    if (useBones) {
        boneTransform = aWeights[0] * bones[aBoneIDs[0]] +
                        aWeights[1] * bones[aBoneIDs[1]] +
                        aWeights[2] * bones[aBoneIDs[2]] +
                        aWeights[3] * bones[aBoneIDs[3]];
    }

    // 组合模型变换与骨骼变换
    mat4 finalModel = model * boneTransform;

    // 计算初始位置
    vec4 pos = finalModel * vec4(aPos, 1.0);

    // 设置输出 FragPos
    fs_out.FragPos = vec3(pos);

    // 计算并传递世界空间中的法线
    fs_out.Normal = mat3(transpose(inverse(finalModel))) * aNormal;

    // 传递纹理坐标
    fs_out.TexCoords = aTexCoords;

    // 传递切线和副切线
    Tangent = mat3(finalModel) * aTangent;
    Bitangent = mat3(finalModel) * aBitangent;

    // 计算每个光源的光空间坐标，基于位移后的 FragPos
    for (int i = 0; i < lightCount; i++) {
        fs_out.FragPosLightSpace[i] = lightSpaceMatrices[i] * pos;
    }

    // 最终裁剪空间位置计算
    gl_Position = projection * view * pos;
}
