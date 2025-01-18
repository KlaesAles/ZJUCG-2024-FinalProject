#version 330 core
out vec4 FragColor;
in vec3 WorldPos;

uniform sampler2D panorama;

const vec2 invAtan = vec2(0.1591, 0.3183);
const float PI = 3.14159265359;

vec2 SampleSphericalMap(vec3 v)
{
    // 将方向向量转换为球面坐标
    vec2 uv = vec2(atan(v.z, v.x), asin(v.y));
    
    // 将范围从 [-PI, PI] x [-PI/2, PI/2] 转换到 [0, 1] x [0, 1]
    uv.x = uv.x / (2.0 * PI) + 0.5;
    uv.y = uv.y / PI + 0.5;
    
    return uv;
}

void main()
{
    vec3 normal = normalize(WorldPos);
    vec2 uv = SampleSphericalMap(normal);
    
    // 采样全景图
    vec3 color = texture(panorama, uv).rgb;
    
    // gamma校正
    color = pow(color, vec3(1.0/2.2));
    
    FragColor = vec4(color, 1.0);
}
