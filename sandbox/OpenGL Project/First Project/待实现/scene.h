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
    // ���캯�������ܹ�Դ����������Ӱ������������
    Scene(LightManager& lightMgr, ShadowManager& shadowMgr);

    // ��ʼ��������Դ������س�ʼ����
    bool Initialize();

    // ��Ⱦһ֡ʱ��ȡ���г�������
    const std::vector<std::shared_ptr<GameObject>>& GetGameObjects() const;

    // ʵʱ��� GameObject
    void AddGameObject(const std::shared_ptr<GameObject>& obj);

    // ʵʱɾ�� GameObject��ͨ��ָ�룩
    void RemoveGameObject(const std::shared_ptr<GameObject>& obj);

    // ʵʱɾ�� GameObject��ͨ��������
    void RemoveGameObject(size_t index);

    // ������Դ
    void Cleanup();

private:
    // ��Դ����Ӱ����������
    LightManager& lightManager;
    ShadowManager& shadowManager;

    // �����е���Ϸ����
    std::vector<std::shared_ptr<GameObject>> sceneObjects;

    // ˽�з��������ڳ�ʼ����ʼ��������
    bool setupInitialObjects();
};

Scene::Scene(LightManager& lightMgr, ShadowManager& shadowMgr)
    : lightManager(lightMgr), shadowManager(shadowMgr) {
}

bool Scene::Initialize()
{
    // ��ʼ����ʼ��������������������
    return setupInitialObjects();
}

bool Scene::setupInitialObjects()
{
    // ʾ������ӵ���
    auto plane = std::make_shared<GameObject>(/* ����: VAO, vertexCount, position, scale, rotation */);
    AddGameObject(plane);

    // ʾ������Ӷ��������
    glm::vec3 cubePositions[] = {
        glm::vec3(0.0f,  0.0f,  0.0f),
        glm::vec3(2.0f,  5.0f, -15.0f),
        glm::vec3(-1.5f, -2.2f, -2.5f),
        // ��Ӹ���λ��
    };
    glm::vec3 cubeScale[] = {
        glm::vec3(1.0f),
        glm::vec3(0.5f),
        glm::vec3(0.75f),
        // ��Ӧ��������
    };

    for (size_t i = 0; i < sizeof(cubePositions) / sizeof(cubePositions[0]); ++i) {
        auto cube = std::make_shared<GameObject>(/* ����: VAO, vertexCount, position, scale, rotation */);
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
