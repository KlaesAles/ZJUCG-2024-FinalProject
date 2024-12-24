#ifndef SCENE_H
#define SCENE_H

#include <vector>
#include <string>
#include <nlohmann/json.hpp>
#include "GameObject.h"

class Scene {
private:
    std::vector<GameObject> gameObjects; // �����е���������

public:
    // ���캯��
    Scene() = default;

    // ��� GameObject
    void addGameObject(const GameObject& obj) {
        gameObjects.push_back(obj);
    }

    // ��ȡ���� GameObject
    std::vector<GameObject>& getGameObjects() {
        return gameObjects;
    }

    // ��Ⱦ����
    void draw(Shader& shader) {
        for (const auto& obj : gameObjects) {
            shader.setMat4("model", obj.getModelMatrix());
            obj.getModel().Draw(shader.ID);
        }
    }

    // ��Ⱦ��Ӱ��ͼ
    void drawShadowMaps(GLuint shadowShader) {
        for (const auto& obj : gameObjects) {
            GLint modelLoc = glGetUniformLocation(shadowShader, "model");
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, &obj.getModelMatrix()[0][0]);
            obj.getModel().Draw(shadowShader);
        }
    }

    // ���л������� JSON
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

    // �� JSON ���س���
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
