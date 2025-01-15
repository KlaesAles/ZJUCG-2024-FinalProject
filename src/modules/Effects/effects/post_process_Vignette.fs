#version 330 core
out vec4 FragColor;
in vec2 TexCoords;

uniform sampler2D screenTexture;
uniform float radius;   // 暗角半径，越大越不明显，建议在 [0.0, 1.0]
uniform float softness; // 暗角软硬程度

void main()
{
    vec3 color = texture(screenTexture, TexCoords).rgb;

    // 计算与屏幕中心的距离 (0.5, 0.5)
    float dist = distance(TexCoords, vec2(0.5, 0.5));

    // 根据距离衰减
    float vignette = 1.0 - smoothstep(radius - softness, radius + softness, dist);

    // 将颜色乘以衰减，中心保留，边缘变暗
    FragColor = vec4(color * vignette, 1.0);
}
