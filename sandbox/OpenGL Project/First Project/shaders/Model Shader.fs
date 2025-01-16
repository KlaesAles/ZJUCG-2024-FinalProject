#version 430 core

out vec4 FragColor;

// 材质结构体，支持多个漫反射和镜面反射纹理
struct Material {
    vec3 albedo;
    float metallic;
    float roughness;
    float ao;

    sampler2D albedoMap;
    sampler2D metallicMap;
    sampler2D roughnessMap;
    sampler2D normalMap;
    sampler2D aoMap;

    bool useAlbedoMap;
    bool useMetallicMap;
    bool useRoughnessMap;
    bool useNormalMap;
    bool useAOMap;
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
uniform samplerCube shadowCubeMaps[16]; // 每个点光源的阴影立方体贴图
uniform float far_plane;                // 点光源的远裁剪面
uniform int debugLightView;             // 调试光源模式开关
uniform int debugLightIndex;            // 调试的光源索引
uniform int debugMaterialView;          // 调试材质开关
uniform int debugMaterialIndex;         // 调试的材质索引

// 常量
const float PI = 3.14159265359;

in VS_OUT {
    vec3 FragPos;                          // 世界空间中的片段位置
    vec3 Normal;                           // 世界空间中的法线
    vec2 TexCoords;                        // 纹理坐标
    vec4 FragPosLightSpace[16];            // 每个光源的光空间坐标
} fs_in;

in vec3 Tangent;
in vec3 Bitangent;

// 函数原型
vec3 getNormalFromMap();
float DistributionGGX(vec3 N, vec3 H, float roughness);
float GeometrySchlickGGX(float NdotV, float roughness);
float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness);
vec3 fresnelSchlick(float cosTheta, vec3 F0);
float CalculateShadow(float type, vec3 FragPos, vec3 normal, vec3 lightDir, int index);

void main()
{    
    // 获取法线和视角方向
    vec3 N = normalize(fs_in.Normal);
    if (material.useNormalMap) {
        N = getNormalFromMap();
    }

    vec3 V = normalize(viewPos - fs_in.FragPos);
    vec3 R = reflect(-V, N);

  // 调试模式：显示深度贴图内容
    if (debugLightView == 1) {
        float lightType = params[debugLightIndex].y;

        if (lightType == 0.0 || lightType == 2.0) { // 方向光或聚光灯
            vec4 fragPosLightSpace = fs_in.FragPosLightSpace[debugLightIndex];
            vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w; // 透视除法
            projCoords = projCoords * 0.5 + 0.5; // 转换到 [0, 1] 范围

            // 防止越界
            if (projCoords.x >= 0.0 && projCoords.x <= 1.0 && projCoords.y >= 0.0 && projCoords.y <= 1.0) {
                float depth = texture(shadowMaps[debugLightIndex], projCoords.xy).r; // 采样深度贴图
                FragColor = vec4(vec3(depth), 1.0); // 将深度值映射为灰度颜色
            } else {
                FragColor = vec4(1.0, 0.0, 0.0, 1.0); // 越界时显示红色
            }
        } else if (lightType == 1.0) { // 点光源
            vec3 fragToLight = fs_in.FragPos - position[debugLightIndex].xyz;
            float currentDepth = length(fragToLight); // 当前片段到光源的距离
            float depthSample = texture(shadowCubeMaps[debugLightIndex], fragToLight).r; // 采样深度
            depthSample *= far_plane; // 转换到实际深度范围

            // 将深度映射为灰度值
            float depthValue = currentDepth / far_plane;
            FragColor = vec4(vec3(depthValue), 1.0);
        }
        return;
    }

    // 调试模式：显示材质贴图内容
    if (debugMaterialView == 1) {
        if (debugMaterialIndex == 0 && material.useAlbedoMap) {
            FragColor = texture(material.albedoMap, fs_in.TexCoords); // 显示漫反射贴图
        } else if (debugMaterialIndex == 1 && material.useMetallicMap) {
            float metallic = texture(material.metallicMap, fs_in.TexCoords).r;
            FragColor = vec4(vec3(metallic), 1.0); // 显示金属度
        } else if (debugMaterialIndex == 2 && material.useRoughnessMap) {
            float roughness = texture(material.roughnessMap, fs_in.TexCoords).r;
            FragColor = vec4(vec3(roughness), 1.0); // 显示粗糙度
        } else if (debugMaterialIndex == 3 && material.useAOMap) {
            float ao = texture(material.aoMap, fs_in.TexCoords).r;
            FragColor = vec4(vec3(ao), 1.0); // 显示环境光遮蔽
        } else if (debugMaterialIndex == 4 && material.useNormalMap) {
            vec3 tangentNormal = texture(material.normalMap, fs_in.TexCoords).rgb;
            FragColor = vec4(tangentNormal, 1.0); // 显示法线贴图
        } else {
            FragColor = vec4(1.0, 0.0, 0.0, 1.0); // 无效的材质索引显示红色
        }
        return;
    }

    // 获取材质属性
    vec3 albedo = material.albedo;
    if (material.useAlbedoMap)
        albedo = pow(texture(material.albedoMap, fs_in.TexCoords).rgb, vec3(2.2)); // 线性空间

    float metallic = material.metallic;
    if (material.useMetallicMap)
        metallic = texture(material.metallicMap, fs_in.TexCoords).r;

    float roughness = material.roughness;
    if (material.useRoughnessMap)
        roughness = texture(material.roughnessMap, fs_in.TexCoords).r;

    float ao = material.ao;
    if (material.useAOMap)
        ao = texture(material.aoMap, fs_in.TexCoords).r;

    // 计算基础反射率
    vec3 F0 = vec3(0.04);
    F0 = mix(F0, albedo, metallic);

    // 初始化最终颜色
    vec3 Lo = vec3(0.0);

    // 遍历所有光源
    for(int i = 0; i < lightCount; ++i) {
        vec3 L;
        float cutoff = params[i].x; // 切光角度，单位为度
        float intensity = 1.0;

        if(params[i].y == 1.0) { // 点光源
            L = normalize(position[i].xyz - fs_in.FragPos);
        } 
        else if(params[i].y == 2.0) { // SpotLight
            L = normalize(position[i].xyz - fs_in.FragPos); // 从片段指向光源的位置

            float theta = dot(normalize(direction[i].xyz), normalize(-L)); // 计算光线与聚光灯方向的夹角余弦
            float cutoffCos = cos(radians(cutoff)); // 切光角度的余弦值
            float epsilon = 0.1; // 渐变范围，可根据需要调整
            intensity = clamp((theta - cutoffCos) / epsilon, 0.0, 1.0);
        } 
        else { // DirectionalLight
            L = normalize(-direction[i].xyz);
        }
        
        vec3 H = normalize(V + L);
        float distance;
        float attenuation;
        vec3 radiance;
        
        if(params[i].y == 1.0) { // 点光源
            distance = length(position[i].xyz - fs_in.FragPos);
            attenuation = 1.0 / (distance * distance);
            radiance = color[i].rgb * attenuation;
        } else { // 方向光或聚光灯
            attenuation = 1.0; // 方向光没有衰减
            radiance = color[i].rgb * attenuation;
        }
        
        // Cook-Torrance BRDF
        float NDF = DistributionGGX(N, H, roughness);
        float G = GeometrySmith(N, V, L, roughness);
        vec3 F = fresnelSchlick(max(dot(H, V), 0.0), F0);

        vec3 numerator = NDF * G * F;
        float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.001;
        vec3 specular = numerator / denominator;

        // kS 是镜面反射分量
        vec3 kS = F;
        // kD 是漫反射分量
        vec3 kD = vec3(1.0) - kS;
        kD *= 1.0 - metallic;

        // NdotL
        float NdotL = max(dot(N, L), 0.0);

        // 计算阴影
        float shadow = CalculateShadow(params[i].y, fs_in.FragPos, N, L, i);

        // 累加光照
        Lo += (kD * albedo / PI + specular) * radiance * NdotL * intensity * color[i].w * (1.0 - shadow);
    }


    // 环境光
    vec3 ambient = vec3(0.03) * albedo * ao;

    // 最终颜色
    vec3 colorResult = ambient + Lo;

    // HDR tonemapping
    colorResult = colorResult / (colorResult + vec3(1.0));
    // Gamma correction
    colorResult = pow(colorResult, vec3(1.0/2.2));

    FragColor = vec4(colorResult, 1.0);
}

// 函数实现

// 计算阴影（方向光、聚光灯和点光源）
float CalculateShadow(float type, vec3 FragPos, vec3 normal, vec3 lightDir, int index)
{
   float shadow = 0.0;
    if (type == 0.0 || type == 2.0) // 方向光或聚光灯
    {
       vec4 fragPosLightSpace = fs_in.FragPosLightSpace[index];
        // 透视除法
        vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
        // 转换到 [0,1] 范围
        projCoords = projCoords * 0.5 + 0.5;
        // 检查是否在阴影贴图范围内
        if(projCoords.z > 1.0)
            return 0.0;
        // 采样阴影贴图
        float closestDepth = texture(shadowMaps[index], projCoords.xy).r; 
        float currentDepth = projCoords.z;
        // 深度偏移
        float bias = 0.005 * (1.0 - dot(normal, lightDir)); // 动态偏移
        // PCF
        float texelSize = 1.0 / 1024.0;
        for(int x = -1; x <= 1; ++x)
        {
            for(int y = -1; y <= 1; ++y)
            {
                float pcfDepth = texture(shadowMaps[index], projCoords.xy + vec2(x, y) * texelSize).r; 
                shadow += currentDepth - bias > pcfDepth ? 1.0 : 0.0; 
            }
        }
        shadow /= 9.0;
    }
    else if (type == 1.0) // 点光源
    {
        return 0.0; // 点光源阴影暂不支持
        vec3 fragToLight = FragPos - position[index].xyz;
        float currentDepth = length(fragToLight);
        float bias = 0.05; // 适当调整偏移值
        float closestDepth = texture(shadowCubeMaps[index], fragToLight).r * far_plane;
        shadow = currentDepth - bias > closestDepth ? 1.0 : 0.0;
        /*
        float shadow = 0.0;
        int samples = 20;
        float diskRadius = 0.05;
        for(int i = 0; i < samples; ++i)
        {
            float closestDepth = texture(shadowCubeMaps[index], fragToLight + sampleOffsetDirections[i] * diskRadius).r;
            closestDepth *= far_plane; // Undo mapping [0;1]
            if(currentDepth - bias > closestDepth)
                shadow += 1.0;
        }
        shadow /= float(samples);
        */
    }

    return shadow;
}

// 从法线贴图获取法线
vec3 getNormalFromMap()
{
    vec3 tangentNormal = texture(material.normalMap, fs_in.TexCoords).rgb;
    tangentNormal = tangentNormal * 2.0 - 1.0;

    mat3 TBN = mat3(normalize(Tangent), normalize(Bitangent), normalize(fs_in.Normal));
    return normalize(TBN * tangentNormal);
}

// GGX分布函数
float DistributionGGX(vec3 N, vec3 H, float roughness)
{
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;

    float nom   = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;

    return nom / denom;
}

// Schlick-GGX几何遮蔽函数
float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r * r) / 8.0;

    float nom   = NdotV;
    float denom = NdotV * (1.0 - k) + k;

    return nom / denom;
}

// Smith几何遮蔽函数
float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = GeometrySchlickGGX(NdotV, roughness);
    float ggx1 = GeometrySchlickGGX(NdotL, roughness);

    return ggx1 * ggx2;
}

// Schlick Fresnel
vec3 fresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}
