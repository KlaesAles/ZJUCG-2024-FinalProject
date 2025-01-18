#ifndef COLLISION_MANAGER_H
#define COLLISION_MANAGER_H

#include "BoundingBox.h"
#include "GameObject.h"
#include <vector>
#include <memory>
#include <iostream>

class CollisionManager {
public:
    // ���������Χ���Ƿ�����ײ
    static bool checkCollision(const BoundingBox& a, const BoundingBox& b) {
        return (a.min.x <= b.max.x && a.max.x >= b.min.x) &&
            (a.min.y <= b.max.y && a.max.y >= b.min.y) &&
            (a.min.z <= b.max.z && a.max.z >= b.min.z);
    }

    // ��ⳡ������������֮�����ײ
    static void detectCollisions(const std::vector<std::shared_ptr<GameObject>>& objects) {
        for (size_t i = 0; i < objects.size(); ++i) {
            for (size_t j = i + 1; j < objects.size(); ++j) {
                if (checkCollision(objects[i]->getBoundingBox(), objects[j]->getBoundingBox())) {
                    // Collision detected between objects[i] and objects[j]
                    // ������������Ӹ����ӵ���ײ��Ӧ�߼�
                    // ʾ�������
                    std::cout << "Collision detected between \""
                        << objects[i]->getName() << "\" and \""
                        << objects[j]->getName() << "\"" << std::endl;
                }
            }
        }
    }

    static bool detectCollisions(const BoundingBox& a, std::vector<std::shared_ptr<GameObject>>& objects) {
        for (size_t i = 0; i < objects.size(); ++i) {
            if (checkCollision(a, objects[i]->getBoundingBox())) {
                return true;
            }
        }
        return false;
    }
};

#endif // COLLISION_MANAGER_H
