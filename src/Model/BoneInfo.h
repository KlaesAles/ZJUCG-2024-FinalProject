#ifndef BONE_INFO_H
#define BONE_INFO_H

#include <glm/glm.hpp>

// ������������Ϣ
struct BoneInfo {
    glm::mat4 offsetMatrix;    // ���ڽ������ռ�ת����ģ�Ϳռ�
    glm::mat4 finalTransform;  // ���ձ任���󣨰��������任��
};

#endif // BONE_INFO_H
