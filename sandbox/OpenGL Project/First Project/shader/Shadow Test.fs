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

in VS_OUT {
    vec3 FragPos;
    vec3 Normal;
    vec2 TexCoords;
    vec4 FragPosLightSpace[16];
} fs_in;

// function prototypes
vec3 CalculateLight(vec3 normal, vec3 viewDir, vec3 fragPos, vec4 position, vec4 direction, vec4 color, vec4 param, int lightIndex);

void main()
{    
    // 获取法线和视角方向
    vec3 norm = normalize(fs_in.Normal);
    vec3 viewDir = normalize(viewPos - fs_in.FragPos);

    vec3 debug1 = position[1].xyz;
    vec3 debug2 = direction[1].xyz;
    vec3 debug3 = color[1].xyz;
    vec3 debug4 = params[0].xyz;
    // 正常渲染
    vec3 result = vec3(0.0);
    for(int i = 0; i < lightCount; ++i) {
        result += CalculateLight(norm, viewDir, fs_in.FragPos, 
                               position[i], direction[i], color[i], params[i], i);
    }
    FragColor = vec4(result, 1.0); 
}

float CalculateShadow(vec4 fragPosLightSpace, sampler2D shadowMap, vec3 normal, vec3 lightDir)
{
    // 执行透视除法
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    projCoords = projCoords * 0.5 + 0.5;
    
    // 超出光空间范围时不产生阴影
    if(projCoords.x < 0.0 || projCoords.x > 1.0 || 
       projCoords.y < 0.0 || projCoords.y > 1.0 || 
       projCoords.z < 0.0 || projCoords.z > 1.0)
        return 0.0;
    
    float currentDepth = projCoords.z;
    
    // 计算偏移
    float bias = max(0.05 * (1.0 - dot(normal, lightDir)), 0.005);
    
    // 改进PCF采样
    float shadow = 0.0;
    vec2 texelSize = 1.0 / textureSize(shadowMap, 0);
    const int halfKernelSize = 2;
    
    for(int x = -halfKernelSize; x <= halfKernelSize; ++x) {
        for(int y = -halfKernelSize; y <= halfKernelSize; ++y) {
            float pcfDepth = texture(shadowMap, projCoords.xy + vec2(x, y) * texelSize).r;
            shadow += currentDepth - bias > pcfDepth ? 1.0 : 0.0;
        }
    }
    
    shadow /= pow(2 * halfKernelSize + 1, 2);
    
    // 边缘淡出
    float fadeStart = 0.9;
    shadow *= 1.0 - smoothstep(fadeStart, 1.0, currentDepth);
    
    return shadow;
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
        shadow = CalculateShadow(fs_in.FragPosLightSpace[lightIndex], shadowMaps[lightIndex], normal, lightDir);
        float diff = max(dot(normal, lightDir), 0.0);
        vec3 reflectDir = reflect(-lightDir, normal);
        float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);

        // 环境光 + 漫反射 + 镜面反射
        vec3 ambient = lightColor * vec3(texture(material.diffuse, fs_in.TexCoords)) * 0.1;
        vec3 diffuse = lightColor * diff * vec3(texture(material.diffuse, fs_in.TexCoords));
        vec3 specular = lightColor * spec * vec3(texture(material.specular, fs_in.TexCoords));
        result = (ambient + diffuse + specular) * intensity;
    }
    // Point Light
    else if (param.y == 1.0) {
        lightDir = normalize(position.xyz - fragPos);
        shadow = CalculateShadow(fs_in.FragPosLightSpace[lightIndex], shadowMaps[lightIndex], normal, lightDir);
        float diff = max(dot(normal, lightDir), 0.0);
        vec3 reflectDir = reflect(-lightDir, normal);
        float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
        float distance = length(position.xyz - fragPos);
        float attenuation = 1.0 / (1.0 + 0.09 * distance + 0.032 * distance * distance);

        // 环境光 + 漫反射 + 镜面反射
        vec3 ambient = lightColor * vec3(texture(material.diffuse, fs_in.TexCoords)) * 0.1;
        vec3 diffuse = lightColor * diff * vec3(texture(material.diffuse, fs_in.TexCoords));
        vec3 specular = lightColor * spec * vec3(texture(material.specular, fs_in.TexCoords));
        result = (ambient + diffuse + specular) * attenuation * intensity;
    }
    // Spot Light
    else if (param.y == 2.0) {
        lightDir = normalize(position.xyz - fragPos);
        shadow = CalculateShadow(fs_in.FragPosLightSpace[lightIndex], shadowMaps[lightIndex], normal, lightDir);
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
        vec3 diffuse = lightColor * diff * vec3(texture(material.diffuse, fs_in.TexCoords));
        vec3 specular = lightColor * spec * vec3(texture(material.specular, fs_in.TexCoords));
    
        // 使用更强的衰减
        result = (ambient + diffuse + specular) * attenuation * intensityFactor * intensity * 2.0; // 加强整体强度
    }

    // 将阴影因子应用到光照计算结果
    return result * (1.0 - shadow);
}
