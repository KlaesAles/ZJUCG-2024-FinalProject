#ifndef SCENE_H
#define SCENE_H

#include <vector>
#include <string>
#include <nlohmann/json.hpp>
#include "GameObject.h"

class Scene {
private:
    std::vector<GameObject> gameObjects; // 场景中的所有物体

public:
    // 构造函数
    Scene() = default;

    // 添加 GameObject
    void addGameObject(const GameObject& obj) {
        gameObjects.push_back(obj);
    }

    // 获取所有 GameObject
    std::vector<GameObject>& getGameObjects() {
        return gameObjects;
    }

    // 渲染场景
    void draw(Shader& shader) {
        for (const auto& obj : gameObjects) {
            shader.setMat4("model", obj.getModelMatrix());
            obj.getModel().Draw(shader.ID);
        }
    }

    // 渲染阴影贴图
    void drawShadowMaps(GLuint shadowShader) {
        for (const auto& obj : gameObjects) {
            GLint modelLoc = glGetUniformLocation(shadowShader, "model");
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, &obj.getModelMatrix()[0][0]);
            obj.getModel().Draw(shadowShader);
        }
    }

    // 序列化场景到 JSON
    nlohmann::json serialize() {
        nlohmann::json sceneJson;
        for (const auto& obj : gameObjects) {
            sceneJson["objects"].push_back({
                {"modelPath", obj.getModel().getPath()},
                {"position", {obj.getPosition().x, obj.getPosition().y, obj.getPosition().z}},
                {"scale", {obj.getScale().x, obj.getScale().y, obj.getScale().z}},
                {"rotation", {obj.getRotation().x, obj.getRotation().y, obj.getRotation().z}}
                });
        }
        return sceneJson;
    }

    // 从 JSON 加载场景
    void deserialize(const nlohmann::json& sceneJson) {
        gameObjects.clear();
        for (const auto& objJson : sceneJson["objects"]) {
            GameObject obj(
                objJson["modelPath"],
                glm::vec3(objJson["position"][0], objJson["position"][1], objJson["position"][2]),
                glm::vec3(objJson["scale"][0], objJson["scale"][1], objJson["scale"][2]),
                glm::vec3(objJson["rotation"][0], objJson["rotation"][1], objJson["rotation"][2])
            );
            addGameObject(obj);
        }
    }
};

#endif // SCENE_H
