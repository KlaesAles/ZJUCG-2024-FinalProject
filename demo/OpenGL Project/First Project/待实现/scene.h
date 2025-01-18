#pragma once

#include <vector>
#include <memory>
#include <iostream>

#include "GameObject.h"
#include "Scene.h"
#include "LightManager.h"
#include "ShadowManager.h"

class LightManager;
class ShadowManager;

class Scene {
public:
    // 构造函数，接受光源管理器和阴影管理器的引用
    Scene(LightManager& lightMgr, ShadowManager& shadowMgr);

    // 初始化场景资源（如加载初始对象）
    bool Initialize();

    // 渲染一帧时获取所有场景对象
    const std::vector<std::shared_ptr<GameObject>>& GetGameObjects() const;

    // 实时添加 GameObject
    void AddGameObject(const std::shared_ptr<GameObject>& obj);

    // 实时删除 GameObject（通过指针）
    void RemoveGameObject(const std::shared_ptr<GameObject>& obj);

    // 实时删除 GameObject（通过索引）
    void RemoveGameObject(size_t index);

    // 清理资源
    void Cleanup();

private:
    // 光源和阴影管理器引用
    LightManager& lightManager;
    ShadowManager& shadowManager;

    // 场景中的游戏对象
    std::vector<std::shared_ptr<GameObject>> sceneObjects;

    // 私有方法，用于初始化初始场景对象
    bool setupInitialObjects();
};

Scene::Scene(LightManager& lightMgr, ShadowManager& shadowMgr)
    : lightManager(lightMgr), shadowManager(shadowMgr) {
}

bool Scene::Initialize()
{
    // 初始化初始场景对象，如地面和立方体
    return setupInitialObjects();
}

bool Scene::setupInitialObjects()
{
    // 示例：添加地面
    auto plane = std::make_shared<GameObject>(/* 参数: VAO, vertexCount, position, scale, rotation */);
    AddGameObject(plane);

    // 示例：添加多个立方体
    glm::vec3 cubePositions[] = {
        glm::vec3(0.0f,  0.0f,  0.0f),
        glm::vec3(2.0f,  5.0f, -15.0f),
        glm::vec3(-1.5f, -2.2f, -2.5f),
        // 添加更多位置
    };
    glm::vec3 cubeScale[] = {
        glm::vec3(1.0f),
        glm::vec3(0.5f),
        glm::vec3(0.75f),
        // 对应更多缩放
    };

    for (size_t i = 0; i < sizeof(cubePositions) / sizeof(cubePositions[0]); ++i) {
        auto cube = std::make_shared<GameObject>(/* 参数: VAO, vertexCount, position, scale, rotation */);
        cube->SetPosition(cubePositions[i]);
        cube->SetScale(cubeScale[i]);
        AddGameObject(cube);
    }

    return true;
}

const std::vector<std::shared_ptr<GameObject>>& Scene::GetGameObjects() const
{
    return sceneObjects;
}

void Scene::AddGameObject(const std::shared_ptr<GameObject>& obj)
{
    sceneObjects.emplace_back(obj);
}

void Scene::RemoveGameObject(const std::shared_ptr<GameObject>& obj)
{
    sceneObjects.erase(
        std::remove(sceneObjects.begin(), sceneObjects.end(), obj),
        sceneObjects.end()
    );
}

void Scene::RemoveGameObject(size_t index)
{
    if (index < sceneObjects.size()) {
        sceneObjects.erase(sceneObjects.begin() + index);
    }
}

void Scene::Cleanup()
{
    sceneObjects.clear();
}
