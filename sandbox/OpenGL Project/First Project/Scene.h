#ifndef SCENE_H
#define SCENE_H

#include <vector>
#include <string>
#include <memory> // 引入智能指针
#include <nlohmann/json.hpp>
#include "GameObject.h"
#include "LightManager.h"

class Scene {
private:
    std::vector<std::shared_ptr<GameObject>> gameObjects; // 使用 shared_ptr 存储 GameObject
    LightManager& lightManager;                          // 引用光源管理器

public:
    // 构造函数
    Scene(LightManager& lightManager)
        : lightManager(lightManager) {
    }

    // 添加 GameObject
    void addGameObject(const std::shared_ptr<GameObject>& obj) {
        gameObjects.push_back(obj);
    }

    // 获取所有 GameObject
    std::vector<std::shared_ptr<GameObject>>& getGameObjects() {
        return gameObjects;
    }

    // 更新场景（包括动画）
    void update(float deltaTime, Shader& shader) {
        for (auto& obj : gameObjects) {
            obj->update(deltaTime, shader);
        }
    }

    // 渲染场景
    void draw(Shader& shader) {
        for (auto& obj : gameObjects) {
            shader.setMat4("model", obj->getModelMatrix());
            obj->uploadBoneUniforms(shader);
            obj->getModel().Draw(shader);
        }
    }

    // 渲染阴影贴图
    void drawShadowMaps(Shader& shadowShader)
    {
        for (const auto& obj : gameObjects)
        {
            shadowShader.setMat4("model", obj->getModelMatrix());
            obj->getModel().Draw(shadowShader);
        }
    }


    // 序列化场景到 JSON
    nlohmann::json serialize() {
        nlohmann::json sceneJson;

        // 序列化物体
        for (const auto& obj : gameObjects) {
            sceneJson["objects"].push_back({
                {"name", obj->getName()},
                {"modelPath", obj->getModel().getPath()},
                {"position", {obj->getPosition().x, obj->getPosition().y, obj->getPosition().z}},
                {"scale", {obj->getScale().x, obj->getScale().y, obj->getScale().z}},
                {"rotation", {obj->getRotation().x, obj->getRotation().y, obj->getRotation().z}}
                });
        }

        // 序列化光源
        sceneJson["lights"] = lightManager.serialize();

        return sceneJson;
    }

    // 从 JSON 加载场景
    void deserialize(const nlohmann::json& sceneJson) {
        // 清空现有数据
        gameObjects.clear();
        lightManager.clearLights();

        // 反序列化物体
        for (const auto& objJson : sceneJson["objects"]) {
            auto obj = std::make_shared<GameObject>(
                objJson["name"].get<std::string>(),
                objJson["modelPath"].get<std::string>(),
                glm::vec3(objJson["position"][0], objJson["position"][1], objJson["position"][2]),
                glm::vec3(objJson["scale"][0], objJson["scale"][1], objJson["scale"][2]),
                glm::vec3(objJson["rotation"][0], objJson["rotation"][1], objJson["rotation"][2])
            );
            addGameObject(obj);
        }

        // 反序列化光源
        lightManager.deserialize(sceneJson["lights"]);
    }
};

#endif // SCENE_H
