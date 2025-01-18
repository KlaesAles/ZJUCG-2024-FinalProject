#ifndef BOUNDINGBOX_H
#define BOUNDINGBOX_H

#include <glm/glm.hpp>
#include <algorithm>
#include <vector>

class BoundingBox {
public:
    glm::vec3 min; // 包围盒的最小点
    glm::vec3 max; // 包围盒的最大点

    // 构造函数，初始化包围盒
    BoundingBox()
        : min(glm::vec3(FLT_MAX)), max(glm::vec3(-FLT_MAX)) {
    }

    // 使用指定的点集初始化包围盒
    BoundingBox(const std::vector<glm::vec3>& points)
        : min(glm::vec3(FLT_MAX)), max(glm::vec3(-FLT_MAX)) {
        for (const auto& point : points) {
            update(point);
        }
    }

    // 更新包围盒，传入一个新的点位置
    void update(const glm::vec3& point) {
        min = glm::min(min, point); // 更新最小点
        max = glm::max(max, point); // 更新最大点
    }

    // 获取包围盒的中心点
    glm::vec3 getCenter() const {
        return (min + max) * 0.5f; // 中心点 = (min + max) / 2
    }

    // 获取包围盒的尺寸（宽度、高度、深度）
    glm::vec3 getSize() const {
        return max - min; // 尺寸 = max - min
    }

    // 判断点是否在包围盒内
    bool contains(const glm::vec3& point) const {
        return (point.x >= min.x && point.x <= max.x) &&
            (point.y >= min.y && point.y <= max.y) &&
            (point.z >= min.z && point.z <= max.z);
    }

    // 检查是否与另一个包围盒相交
    bool intersects(const BoundingBox& other) const {
        return (min.x <= other.max.x && max.x >= other.min.x) &&
            (min.y <= other.max.y && max.y >= other.min.y) &&
            (min.z <= other.max.z && max.z >= other.min.z);
    }

    // 合并当前包围盒与另一个包围盒（生成包含两者的包围盒）
    void merge(const BoundingBox& other) {
        min = glm::min(min, other.min);
        max = glm::max(max, other.max);
    }
};

#endif // BOUNDINGBOX_H
