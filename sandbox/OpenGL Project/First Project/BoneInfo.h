#ifndef BONE_INFO_H
#define BONE_INFO_H

#include <glm/glm.hpp>

// 单个骨骼的信息
struct BoneInfo {
    glm::mat4 offsetMatrix;    // 用于将骨骼空间转换到模型空间
    glm::mat4 finalTransform;  // 最终变换矩阵（包含动画变换）
};

#endif // BONE_INFO_H
