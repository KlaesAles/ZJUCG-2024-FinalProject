#ifndef BOUNDINGBOX_H
#define BOUNDINGBOX_H

#include <glm/glm.hpp>
#include <algorithm>
#include <vector>

class BoundingBox {
public:
    glm::vec3 min; // ��Χ�е���С��
    glm::vec3 max; // ��Χ�е�����

    // ���캯������ʼ����Χ��
    BoundingBox()
        : min(glm::vec3(FLT_MAX)), max(glm::vec3(-FLT_MAX)) {
    }

    // ʹ��ָ���ĵ㼯��ʼ����Χ��
    BoundingBox(const std::vector<glm::vec3>& points)
        : min(glm::vec3(FLT_MAX)), max(glm::vec3(-FLT_MAX)) {
        for (const auto& point : points) {
            update(point);
        }
    }

    // ���°�Χ�У�����һ���µĵ�λ��
    void update(const glm::vec3& point) {
        min = glm::min(min, point); // ������С��
        max = glm::max(max, point); // ��������
    }

    // ��ȡ��Χ�е����ĵ�
    glm::vec3 getCenter() const {
        return (min + max) * 0.5f; // ���ĵ� = (min + max) / 2
    }

    // ��ȡ��Χ�еĳߴ磨��ȡ��߶ȡ���ȣ�
    glm::vec3 getSize() const {
        return max - min; // �ߴ� = max - min
    }

    // �жϵ��Ƿ��ڰ�Χ����
    bool contains(const glm::vec3& point) const {
        return (point.x >= min.x && point.x <= max.x) &&
            (point.y >= min.y && point.y <= max.y) &&
            (point.z >= min.z && point.z <= max.z);
    }

    // ����Ƿ�����һ����Χ���ཻ
    bool intersects(const BoundingBox& other) const {
        return (min.x <= other.max.x && max.x >= other.min.x) &&
            (min.y <= other.max.y && max.y >= other.min.y) &&
            (min.z <= other.max.z && max.z >= other.min.z);
    }

    // �ϲ���ǰ��Χ������һ����Χ�У����ɰ������ߵİ�Χ�У�
    void merge(const BoundingBox& other) {
        min = glm::min(min, other.min);
        max = glm::max(max, other.max);
    }
};

#endif // BOUNDINGBOX_H
