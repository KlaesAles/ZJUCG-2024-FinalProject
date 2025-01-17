#version 430 core

out vec4 FragColor;

// 材质结构体，支持多个漫反射和镜面反射纹理
struct Material {
    sampler2D diffuse[16];    // 漫反射纹理数组
    int diffuseCount;         // 漫反射纹理数量
    sampler2D specular[16];   // 镜面反射纹理数组
    int specularCount;        // 镜面反射纹理数量
    sampler2D normal[16];     // 法线纹理数组
    int normalCount;          // 法线纹理数量
    float shininess;          // 光泽度
}; 

// 光照块，支持最多16个光源
layout(std140, binding = 0) uniform LightBlock {
    vec4 position[16];   // 光源位置或方向
    vec4 direction[16];  // 光源方向
    vec4 color[16];      // 光源颜色和强度 (xyz: 颜色, w: 强度)
    vec4 params[16];     // 额外参数 (cutoffAngle, type, 保留)
};

uniform int lightCount;                 // 当前光源数量
uniform vec3 viewPos;                   // 观察者位置
uniform Material material;              // 材质
uniform sampler2D shadowMaps[16];       // 每个光源的阴影贴图
uniform int debugLightView;             // 调试光源模式开关
uniform int debugLightIndex;            // 调试的光源索引
uniform int debugMaterialView;          // 调试材质开关
uniform int debugMaterialIndex;         // 调试的材质索引

in VS_OUT {
    vec3 FragPos;                          // 世界空间中的片段位置
    vec3 Normal;                           // 世界空间中的法线
    vec2 TexCoords;                        // 纹理坐标
    vec4 FragPosLightSpace[16];            // 每个光源的光空间坐标
} fs_in;

// 函数原型
vec3 CalculateLight(vec3 normal, vec3 viewDir, vec3 fragPos, vec4 position, vec4 direction, vec4 color, vec4 param, int lightIndex);
float CalculateShadow(vec4 fragPosLightSpace, sampler2D shadowMap, vec3 normal, vec3 lightDir, float lightType);

void main()
{    
    // 获取法线和视角方向
    vec3 norm = normalize(fs_in.Normal);
    vec3 viewDir = normalize(viewPos - fs_in.FragPos);

    // 调试模式：直接显示深度贴图内容
    if (debugLightView == 1) {
        vec4 fragPosLightSpace = fs_in.FragPosLightSpace[debugLightIndex];
        vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w; // 透视除法
        projCoords = projCoords * 0.5 + 0.5; // 转换到 [0, 1] 范围

        float depth = texture(shadowMaps[debugLightIndex], projCoords.xy).r; // 采样深度贴图
        FragColor = vec4(vec3(depth), 1.0); // 将深度值映射为灰度颜色
        return;
    }

    // 调试模式：直接显示不同的材质贴图内容
    if (debugMaterialView == 1) {
        if (debugMaterialIndex < material.diffuseCount) {
            FragColor = texture(material.diffuse[debugMaterialIndex], fs_in.TexCoords); // 显示漫反射贴图
        } else if (debugMaterialIndex - material.diffuseCount < material.specularCount) {
            FragColor = texture(material.specular[debugMaterialIndex - material.diffuseCount], fs_in.TexCoords); // 显示镜面反射贴图
        } else if (debugMaterialIndex - material.diffuseCount - material.specularCount < material.normalCount) {
            FragColor = texture(material.normal[debugMaterialIndex - material.diffuseCount - material.specularCount], fs_in.TexCoords); // 显示法线贴图
        }
        return;
    }

    // 正常渲染
    vec3 result = vec3(0.0);
    for (int i = 0; i < lightCount; ++i) {
        result += CalculateLight(norm, viewDir, fs_in.FragPos, position[i], direction[i], color[i], params[i], i);
    }

    // 对光照结果进行修剪到 [0, 1] 范围
    result = clamp(result, 0.0, 1.0);

    // 输出最终颜色
    FragColor = vec4(result, 1.0);
}

// 计算阴影
float CalculateShadow(vec4 fragPosLightSpace, sampler2D shadowMap, vec3 normal, vec3 lightDir, float lightType){
    vec3 projCoords = vec3(0.0);
    float shadow = 0.0;
    float bias = 0.005;

    if (lightType == 1.0) {  // 点光源
        vec3 fragToLight = fragPosLightSpace.xyz;
        float currentDepth = length(fragToLight);
        projCoords = fragToLight / currentDepth;
        bias = 0.05 * (1.0 - dot(normal, lightDir));
    } else {  // 方向光或聚光灯
        projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
        projCoords = projCoords * 0.5 + 0.5;
        bias = max(0.05 * (1.0 - dot(normal, lightDir)), 0.005);
    }

    // 超出范围的坐标不产生阴影
    if (projCoords.x < 0.0 || projCoords.x > 1.0 || projCoords.y < 0.0 || projCoords.y > 1.0 || projCoords.z < 0.0 || projCoords.z > 1.0)
        return 0.0;

    float closestDepth = texture(shadowMap, projCoords.xy).r;
    shadow = projCoords.z - bias > closestDepth ? 1.0 : 0.0;

    // PCF (Percentage Closer Filtering)
    float texelSize = 1.0 / 1024.0;
    float totalShadow = 0.0;
    for (int x = -1; x <= 1; ++x) {
        for (int y = -1; y <= 1; ++y) {
            float sampleDepth = texture(shadowMap, projCoords.xy + vec2(x, y) * texelSize).r;
            totalShadow += projCoords.z - bias > sampleDepth ? 1.0 : 0.0;
        }
    }
    return totalShadow / 9.0; // 归一化阴影强度
}

// 计算光照
vec3 CalculateLight(vec3 normal, vec3 viewDir, vec3 fragPos, vec4 position, vec4 direction, vec4 color, vec4 param, int lightIndex)
{
    vec3 lightColor = color.rgb;
    float intensity = color.w;
    vec3 result = vec3(0.0);
    vec3 lightDir;
    float shadow = 0.0;

    // 处理法线贴图
    vec3 norm = normal;
    for (int i = 0; i < material.normalCount; ++i) {
        vec3 normalTex = texture(material.normal[i], fs_in.TexCoords).rgb;
        normalTex = normalize(normalTex * 2.0 - 1.0);  // 转换法线贴图颜色值到 [-1, 1] 范围
        norm = normalize(norm + normalTex); // 法线叠加
    }
    norm = normalize(norm); // 确保最终法线是单位向量

    // 方向光
    if (param.y == 0.0) {
        lightDir = normalize(-direction.xyz);
        shadow = CalculateShadow(fs_in.FragPosLightSpace[lightIndex], shadowMaps[lightIndex], normal, lightDir, param.y);
        float diff = max(dot(normal, lightDir), 0.0);
        vec3 reflectDir = reflect(-lightDir, normal);
        float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);

        // 结合所有漫反射纹理
        vec3 diffuseTex = vec3(0.0);
        if (material.diffuseCount > 0) {
            for (int i = 0; i < material.diffuseCount; ++i) {
                diffuseTex += texture(material.diffuse[i], fs_in.TexCoords).rgb;
            }
            diffuseTex /= float(material.diffuseCount); // 平均
        } else {
            diffuseTex = vec3(1.0); // 如果没有漫反射贴图，使用白色（默认）
        }

        // 结合所有镜面反射纹理
        vec3 specularTex = vec3(0.0);
        if (material.specularCount > 0) {
            for (int i = 0; i < material.specularCount; ++i) {
                specularTex += texture(material.specular[i], fs_in.TexCoords).rgb;
            }
            specularTex /= float(material.specularCount); // 平均
        } else {
            specularTex = vec3(0.5); // 如果没有镜面反射贴图，使用默认值（如中等反射）
        }

        // 环境光 + 漫反射 + 镜面反射
        vec3 ambient = lightColor * diffuseTex * 0.1;
        vec3 diffuse = lightColor * diff * diffuseTex * (1.0 - shadow);
        vec3 specular = lightColor * spec * specularTex * (1.0 - shadow);
        result = (ambient + diffuse + specular) * intensity;
    }
    // 点光源
    else if (param.y == 1.0) {
        lightDir = normalize(position.xyz - fragPos);
        shadow = CalculateShadow(fs_in.FragPosLightSpace[lightIndex], shadowMaps[lightIndex], normal, lightDir, param.y);
        float diff = max(dot(normal, lightDir), 0.0);
        vec3 reflectDir = reflect(-lightDir, normal);
        float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
        float distance = length(position.xyz - fragPos);
        float attenuation = 1.0 / (1.0 + 0.09 * distance + 0.032 * distance * distance);

        // 结合所有漫反射纹理
        vec3 diffuseTex = vec3(0.0);
        if (material.diffuseCount > 0) {
            for (int i = 0; i < material.diffuseCount; ++i) {
                diffuseTex += texture(material.diffuse[i], fs_in.TexCoords).rgb;
            }
            diffuseTex /= float(material.diffuseCount); // 平均
        } else {
            diffuseTex = vec3(1.0); // 默认白色
        }

        // 结合所有镜面反射纹理
        vec3 specularTex = vec3(0.0);
        if (material.specularCount > 0) {
            for (int i = 0; i < material.specularCount; ++i) {
                specularTex += texture(material.specular[i], fs_in.TexCoords).rgb;
            }
            specularTex /= float(material.specularCount); // 平均
        } else {
            specularTex = vec3(0.5); // 默认中等反射
        }

        // 环境光 + 漫反射 + 镜面反射
        vec3 ambient = lightColor * diffuseTex * 0.1;
        vec3 diffuse = lightColor * diff * diffuseTex * (1.0 - shadow);
        vec3 specular = lightColor * spec * specularTex * (1.0 - shadow);
        result = (ambient + diffuse + specular) * attenuation * intensity * 1.5;
    }
    // 聚光灯
    else if (param.y == 2.0) {
        lightDir = normalize(position.xyz - fragPos);
        shadow = CalculateShadow(fs_in.FragPosLightSpace[lightIndex], shadowMaps[lightIndex], normal, lightDir, param.y);
        float diff = max(dot(normal, lightDir), 0.0);
        vec3 reflectDir = reflect(-lightDir, normal);
        float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
    
        // 调整距离衰减参数
        float distance = length(position.xyz - fragPos);
        float attenuation = 1.0 / (1.0 + 0.045 * distance + 0.0075 * distance * distance);
    
        // 调整聚光灯边缘软化
        float theta = dot(lightDir, normalize(-direction.xyz));
        float epsilon = cos(radians(param.x)) - cos(radians(param.x + 2.5)); // 减小边缘过渡区域
        float intensityFactor = clamp((theta - cos(radians(param.x + 2.5))) / epsilon, 0.0, 1.0);
    
        // 结合所有漫反射纹理
        vec3 diffuseTex = vec3(0.0);
        if (material.diffuseCount > 0) {
            for (int i = 0; i < material.diffuseCount; ++i) {
                diffuseTex += texture(material.diffuse[i], fs_in.TexCoords).rgb;
            }
            diffuseTex /= float(material.diffuseCount); // 平均
        } else {
            diffuseTex = vec3(1.0); // 默认白色
        }

        // 结合所有镜面反射纹理
        vec3 specularTex = vec3(0.0);
        if (material.specularCount > 0) {
            for (int i = 0; i < material.specularCount; ++i) {
                specularTex += texture(material.specular[i], fs_in.TexCoords).rgb;
            }
            specularTex /= float(material.specularCount); // 平均
        } else {
            specularTex = vec3(0.5); // 默认中等反射
        }

        // 环境光 + 漫反射 + 镜面反射
        vec3 ambient = lightColor * diffuseTex * 0.05; // 降低环境光
        vec3 diffuse = lightColor * diff * diffuseTex * (1.0 - shadow);
        vec3 specular = lightColor * spec * specularTex * (1.0 - shadow);
    
        // 使用更强的衰减和强度
        result = (ambient + diffuse + specular) * attenuation * intensityFactor * intensity * 2.0;
    }

    // 返回光照计算结果
    return result;
}

