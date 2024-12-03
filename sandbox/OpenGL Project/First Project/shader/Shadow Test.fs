#version 430 core
out vec4 FragColor;

struct Material {
    sampler2D diffuse;    // 漫反射纹理
    sampler2D specular;   // 镜面反射纹理
    float shininess;      // 光泽度
}; 

layout(std140, binding = 0) uniform LightBlock {
    vec4 position[16];   // 光源位置或方向
    vec4 direction[16];  // 光源方向
    vec4 color[16];      // 光源颜色和强度 (xyz: 颜色, w: 强度)
    vec4 params[16];      // 额外参数 (cutoffAngle, type, 保留)
};

uniform int lightCount;
uniform vec3 viewPos;
uniform Material material;
uniform sampler2D shadowMaps[16]; // 每个光源的阴影贴图
uniform int debugView;
uniform int debugLightIndex;

in VS_OUT {
    vec3 FragPos;
    vec3 Normal;
    vec2 TexCoords;
    vec4 FragPosLightSpace[16];
    mat4 model;
} fs_in;

// function prototypes
vec3 CalculateLight(vec3 normal, vec3 viewDir, vec3 fragPos, vec4 position, vec4 direction, vec4 color, vec4 param, int lightIndex);
float CalculateShadow(vec4 fragPosLightSpace, sampler2D shadowMap, vec3 normal, vec3 lightDir, float lightType);

void main()
{    
    // 获取法线和视角方向
    vec3 norm = normalize(mat3(transpose(inverse(fs_in.model))) * fs_in.Normal); // 在片段阶段重新计算法线
    vec3 viewDir = normalize(viewPos - fs_in.FragPos);

    vec3 debug1 = position[1].xyz;
    vec3 debug2 = direction[1].xyz;
    vec3 debug3 = color[1].xyz;
    vec3 debug4 = params[0].xyz;

    // 调试模式：直接显示深度贴图内容
    if (debugView == 1) {
        vec4 fragPosLightSpace = fs_in.FragPosLightSpace[debugLightIndex]; // 假设调试第一个光源
        vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w; // 透视除法
        projCoords = projCoords * 0.5 + 0.5; // 转换到 [0, 1] 范围


        float depth = texture(shadowMaps[debugLightIndex], projCoords.xy).r; // 假设调试第一个光源
        FragColor = vec4(vec3(depth), 1.0); // 将深度值映射为灰度颜色
        return;
    }

    // 正常渲染
    vec3 result = vec3(0.0);
    for(int i = 0; i < lightCount; ++i) {
        result += CalculateLight(norm, viewDir, fs_in.FragPos, position[i], direction[i], color[i], params[i], i);
    }

    // 对光照结果进行修剪到 [0, 1] 范围
    result = clamp(result, 0.0, 1.0);

    // 输出最终颜色
    FragColor = vec4(result, 1.0);

}

float CalculateShadow(vec4 fragPosLightSpace, sampler2D shadowMap, vec3 normal, vec3 lightDir, float lightType){
    vec3 projCoords = vec3(0.0);
    float shadow = 0.0;
    float bias = 0.005;

    if (lightType == 1.0) {  // Point Light
        vec3 fragToLight = fragPosLightSpace.xyz;
        float currentDepth = length(fragToLight);
        projCoords = fragToLight / currentDepth;
        bias = 0.05 * (1.0 - dot(normal, lightDir));
    } else {  // Directional or Spot Light
        projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
        projCoords = projCoords * 0.5 + 0.5;
        bias = max(0.05 * (1.0 - dot(normal, lightDir)), 0.005);
    }

    if (projCoords.x < 0.0 || projCoords.x > 1.0 || projCoords.y < 0.0 || projCoords.y > 1.0 || projCoords.z < 0.0 || projCoords.z > 1.0)
        return 0.0;

    float closestDepth = texture(shadowMap, projCoords.xy).r;
    shadow = projCoords.z - bias > closestDepth ? 1.0 : 0.0;

    // PCF (Percentage Closer Filtering)
    float texelSize = 1.0 / 1024.0;
    float totalShadow = 0.0;
    int samples = 4; // 2x2 grid for PCF
    for (int x = -1; x <= 1; ++x) {
        for (int y = -1; y <= 1; ++y) {
            float sampleDepth = texture(shadowMap, projCoords.xy + vec2(x, y) * texelSize).r;
            totalShadow += projCoords.z - bias > sampleDepth ? 1.0 : 0.0;
        }
    }
    return totalShadow / 9.0; // Normalize shadow intensity
}

vec3 CalculateLight(vec3 normal, vec3 viewDir, vec3 fragPos, vec4 position, vec4 direction, vec4 color, vec4 param, int lightIndex)
{
    vec3 lightColor = color.rgb;
    float intensity = color.w;
    vec3 result = vec3(0.0);
    vec3 lightDir;
    float shadow = 0.0;

    // Directional Light
    if (param.y == 0.0) {
        lightDir = normalize(-direction.xyz);
        shadow = CalculateShadow(fs_in.FragPosLightSpace[lightIndex], shadowMaps[lightIndex], normal, lightDir, param.y);
        float diff = max(dot(normal, lightDir), 0.0);
        vec3 reflectDir = reflect(-lightDir, normal);
        float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);

        // 环境光 + 漫反射 + 镜面反射
        vec3 ambient = lightColor * vec3(texture(material.diffuse, fs_in.TexCoords)) * 0.1;
        vec3 diffuse = lightColor * diff * vec3(texture(material.diffuse, fs_in.TexCoords))* (1.0 - shadow);
        vec3 specular = lightColor * spec * vec3(texture(material.specular, fs_in.TexCoords))* (1.0 - shadow);
        result = (ambient + diffuse + specular) * intensity * 1.0;
    }
    // Point Light
    else if (param.y == 1.0) {
        lightDir = normalize(position.xyz - fragPos);
        shadow = CalculateShadow(fs_in.FragPosLightSpace[lightIndex], shadowMaps[lightIndex], normal, lightDir, param.y);
        float diff = max(dot(normal, lightDir), 0.0);
        vec3 reflectDir = reflect(-lightDir, normal);
        float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
        float distance = length(position.xyz - fragPos);
        float attenuation = 1.0 / (1.0 + 0.09 * distance + 0.032 * distance * distance);

        // 环境光 + 漫反射 + 镜面反射
        vec3 ambient = lightColor * vec3(texture(material.diffuse, fs_in.TexCoords)) * 0.1;
        vec3 diffuse = lightColor * diff * vec3(texture(material.diffuse, fs_in.TexCoords))* (1.0 - shadow);
        vec3 specular = lightColor * spec * vec3(texture(material.specular, fs_in.TexCoords))* (1.0 - shadow);
        result = (ambient + diffuse + specular) * attenuation * intensity * 1.5;
    }
    // Spot Light
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
    
        // 增强聚光灯效果
        vec3 ambient = lightColor * vec3(texture(material.diffuse, fs_in.TexCoords)) * 0.05; // 降低环境光
        vec3 diffuse = lightColor * diff * vec3(texture(material.diffuse, fs_in.TexCoords))* (1.0 - shadow);
        vec3 specular = lightColor * spec * vec3(texture(material.specular, fs_in.TexCoords))* (1.0 - shadow);
    
        // 使用更强的衰减
        result = (ambient + diffuse + specular) * attenuation * intensityFactor * intensity * 2.0; // 加强整体强度
    }

    // 将阴影因子应用到光照计算结果
    return result;
}
