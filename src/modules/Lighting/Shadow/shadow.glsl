#version 330 core

// 最大支持的光源数量
#define MAX_LIGHTS 16

// 阴影贴图数组
uniform sampler2DArray shadowMaps;

// 光源的光源空间矩阵
uniform mat4 lightSpaceMatrices[MAX_LIGHTS];

// 阴影算法参数
uniform int lightCount;  // 光源数量
uniform float shadowBias; // 阴影偏移量，避免z-fighting

// 计算单个光源的阴影贡献 (简单硬阴影)
float ShadowCalculation(int lightIndex, vec4 fragPosLightSpace) {
    // 将片段位置从光源空间转化为 NDC 空间
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;

    // 转换到[0,1]范围
    projCoords = projCoords * 0.5 + 0.5;

    // 如果超出阴影贴图范围，则直接返回 0.0 (不投影)
    if (projCoords.z > 1.0 || projCoords.x < 0.0 || projCoords.x > 1.0 || projCoords.y < 0.0 || projCoords.y > 1.0) {
        return 0.0;
    }

    // 从阴影贴图数组中获取当前光源的阴影深度值
    float closestDepth = texture(shadowMaps, vec3(projCoords.xy, lightIndex)).r;

    // 计算是否在阴影中
    float shadow = projCoords.z > closestDepth + shadowBias ? 1.0 : 0.0;

    return shadow;
}

// 计算多个光源的总阴影贡献
float CalculateShadows(vec3 fragWorldPos) {
    float totalShadow = 0.0;

    for (int i = 0; i < lightCount; ++i) {
        // 计算片段在光源空间的坐标
        vec4 fragPosLightSpace = lightSpaceMatrices[i] * vec4(fragWorldPos, 1.0);

        // 计算当前光源的阴影贡献
        totalShadow += ShadowCalculation(i, fragPosLightSpace);
    }

    // 平均阴影贡献
    return totalShadow / lightCount;
}

float PCF_ShadowCalculation(int lightIndex, vec4 fragPosLightSpace) {
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    projCoords = projCoords * 0.5 + 0.5;

    if (projCoords.z > 1.0 || projCoords.x < 0.0 || projCoords.x > 1.0 || projCoords.y < 0.0 || projCoords.y > 1.0) {
        return 0.0;
    }

    float shadow = 0.0;
    float texelSize = 1.0 / textureSize(shadowMaps, 0).x;

    // PCF 采样
    for (int x = -1; x <= 1; ++x) {
        for (int y = -1; y <= 1; ++y) {
            vec2 offset = vec2(x, y) * texelSize;
            float closestDepth = texture(shadowMaps, vec3(projCoords.xy + offset, lightIndex)).r;
            shadow += projCoords.z > closestDepth + shadowBias ? 1.0 : 0.0;
        }
    }

    return shadow / 9.0; // 平均取样结果
}

