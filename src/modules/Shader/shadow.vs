#version 330 core
layout(location = 0) in vec3 aPos;  // 顶点位置

uniform mat4 model;                 // 模型矩阵
uniform mat4 lightSpaceMatrix;      // 光源的投影 * 视图矩阵

void main() {
    gl_Position = lightSpaceMatrix * model * vec4(aPos, 1.0);
}
