#ifndef COLLISION_MANAGER_H
#define COLLISION_MANAGER_H

#include "BoundingBox.h"
#include "GameObject.h"
#include <vector>
#include <iostream>

class CollisionManager {
public:
    // 检查两个包围盒是否发生碰撞
    static bool checkCollision(const BoundingBox& a, const BoundingBox& b) {
        return (a.min.x <= b.max.x && a.max.x >= b.min.x) &&
            (a.min.y <= b.max.y && a.max.y >= b.min.y) &&
            (a.min.z <= b.max.z && a.max.z >= b.min.z);
    }

    // 检测场景中所有物体之间的碰撞
    static void detectCollisions(const std::vector<GameObject>& objects) {
        for (size_t i = 0; i < objects.size(); ++i) {
            for (size_t j = i + 1; j < objects.size(); ++j) {
                if (checkCollision(objects[i].getBoundingBox(), objects[j].getBoundingBox())) {
                    std::cout << "Collision detected between object " << i << " and object " << j << std::endl;
                    // 可以在这里添加更复杂的碰撞响应逻辑
                }
            }
        }
    }
};

#endif // COLLISION_MANAGER_H
