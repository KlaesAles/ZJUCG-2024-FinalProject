#ifndef SCENE_H
#define SCENE_H

#include <vector>
#include <string>
#include <memory> // 引入智能指针
#include <nlohmann/json.hpp>
#include "GameObject.h"

class Scene {
private:
    std::vector<std::shared_ptr<GameObject>> gameObjects; // 使用 shared_ptr 存储 GameObject

public:
    // 构造函数
    Scene() = default;

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
            obj->getModel().Draw(shadowShader); // 注意传递 `Shader::ID`
        }
    }


    // 序列化场景到 JSON
    nlohmann::json serialize() {
        nlohmann::json sceneJson;
        for (const auto& obj : gameObjects) {
            sceneJson["objects"].push_back({
                {"name", obj->getName()}, // 新增 name 字段
                {"modelPath", obj->getModel().getPath()},
                {"position", {obj->getPosition().x, obj->getPosition().y, obj->getPosition().z}},
                {"scale", {obj->getScale().x, obj->getScale().y, obj->getScale().z}},
                {"rotation", {obj->getRotation().x, obj->getRotation().y, obj->getRotation().z}}
                });
        }
        return sceneJson;
    }

    // 从 JSON 加载场景
    void deserialize(const nlohmann::json& sceneJson) {
        gameObjects.clear();
        for (const auto& objJson : sceneJson["objects"]) {
            auto obj = std::make_shared<GameObject>(
                objJson["name"].get<std::string>(), // 传递 name 参数
                objJson["modelPath"].get<std::string>(),
                glm::vec3(objJson["position"][0], objJson["position"][1], objJson["position"][2]),
                glm::vec3(objJson["scale"][0], objJson["scale"][1], objJson["scale"][2]),
                glm::vec3(objJson["rotation"][0], objJson["rotation"][1], objJson["rotation"][2])
            );
            addGameObject(obj);
        }
    }
};

#endif // SCENE_H
