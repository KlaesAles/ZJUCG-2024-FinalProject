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

uniform int lightCount; // 当前有效光源数量

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords;

uniform vec3 viewPos;
uniform Material material;

// function prototypes
vec3 CalculateLight(vec3 normal, vec3 viewDir, vec3 fragPos, vec4 position, vec4 direction, vec4 color, vec4 param);

void main()
{    
    // 获取法线和视角方向
    vec3 norm = normalize(Normal);
    vec3 viewDir = normalize(viewPos - FragPos);

    // 累加光照效果
    vec3 result = vec3(0.0);

    // 迭代光源
    for (int i = 0; i < lightCount; ++i) {
        // if (color[i].w == 0.0) break; // 如果强度为0，表示无效光源
        result += CalculateLight(norm, viewDir, FragPos, position[i], direction[i], color[i], params[i]);
    }

    // 输出最终颜色
    vec3 debug1 = position[1].xyz;
    vec3 debug2 = direction[1].xyz;
    vec3 debug3 = color[1].xyz;
    vec3 debug4 = params[0].xyz;
    FragColor = vec4(result, 1.0); 
}

vec3 CalculateLight(vec3 normal, vec3 viewDir, vec3 fragPos, vec4 position, vec4 direction, vec4 color, vec4 param)
{
    vec3 lightColor = color.rgb;
    float intensity = color.w;

    vec3 lightDir;
    vec3 result = vec3(0.0);

    // Directional Light
    if (param.y == 0.0) {
        lightDir = normalize(-direction.xyz);
        float diff = max(dot(normal, lightDir), 0.0);
        vec3 reflectDir = reflect(-lightDir, normal);
        float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);

        // 环境光 + 漫反射 + 镜面反射
        vec3 ambient = lightColor * vec3(texture(material.diffuse, TexCoords)) * 0.1;
        vec3 diffuse = lightColor * diff * vec3(texture(material.diffuse, TexCoords));
        vec3 specular = lightColor * spec * vec3(texture(material.specular, TexCoords));
        result = (ambient + diffuse + specular) * intensity;
    }
    // Point Light
    else if (param.y == 1.0) {
        lightDir = normalize(position.xyz - fragPos);
        float diff = max(dot(normal, lightDir), 0.0);
        vec3 reflectDir = reflect(-lightDir, normal);
        float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
        float distance = length(position.xyz - fragPos);
        float attenuation = 1.0 / (1.0 + 0.09 * distance + 0.032 * distance * distance);

        // 环境光 + 漫反射 + 镜面反射
        vec3 ambient = lightColor * vec3(texture(material.diffuse, TexCoords)) * 0.1;
        vec3 diffuse = lightColor * diff * vec3(texture(material.diffuse, TexCoords));
        vec3 specular = lightColor * spec * vec3(texture(material.specular, TexCoords));
        result = (ambient + diffuse + specular) * attenuation * intensity;
    }
    // Spot Light
    else if (param.y == 2.0) {
    lightDir = normalize(position.xyz - fragPos);
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
    vec3 ambient = lightColor * vec3(texture(material.diffuse, TexCoords)) * 0.05; // 降低环境光
    vec3 diffuse = lightColor * diff * vec3(texture(material.diffuse, TexCoords));
    vec3 specular = lightColor * spec * vec3(texture(material.specular, TexCoords));
    
    // 使用更强的衰减
    result = (ambient + diffuse + specular) * attenuation * intensityFactor * intensity * 2.0; // 加强整体强度
    }

    return result;
}
