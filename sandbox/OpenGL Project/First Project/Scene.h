#ifndef SCENE_H
#define SCENE_H

#include <vector>
#include <string>
#include <memory> // ��������ָ��
#include <nlohmann/json.hpp>
#include "GameObject.h"
#include "LightManager.h"

class Scene {
private:
    std::vector<std::shared_ptr<GameObject>> gameObjects; // ʹ�� shared_ptr �洢 GameObject
    LightManager& lightManager;                          // ���ù�Դ������

public:
    // ���캯��
    Scene(LightManager& lightManager)
        : lightManager(lightManager) {
    }

    // ��� GameObject
    void addGameObject(const std::shared_ptr<GameObject>& obj) {
        gameObjects.push_back(obj);
    }

    // ��ȡ���� GameObject
    std::vector<std::shared_ptr<GameObject>>& getGameObjects() {
        return gameObjects;
    }

    // ���³���������������
    void update(float deltaTime, Shader& shader) {
        for (auto& obj : gameObjects) {
            obj->update(deltaTime, shader);
        }
    }

    // ��Ⱦ����
    void draw(Shader& shader) {
        for (auto& obj : gameObjects) {
            shader.setMat4("model", obj->getModelMatrix());
            obj->uploadBoneUniforms(shader);
            obj->getModel().Draw(shader);
        }
    }

    // ��Ⱦ��Ӱ��ͼ
    void drawShadowMaps(Shader& shadowShader)
    {
        for (const auto& obj : gameObjects)
        {
            shadowShader.setMat4("model", obj->getModelMatrix());
            obj->getModel().Draw(shadowShader);
        }
    }


    // ���л������� JSON
    nlohmann::json serialize() {
        nlohmann::json sceneJson;

        // ���л�����
        for (const auto& obj : gameObjects) {
            sceneJson["objects"].push_back({
                {"name", obj->getName()},
                {"modelPath", obj->getModel().getPath()},
                {"position", {obj->getPosition().x, obj->getPosition().y, obj->getPosition().z}},
                {"scale", {obj->getScale().x, obj->getScale().y, obj->getScale().z}},
                {"rotation", {obj->getRotation().x, obj->getRotation().y, obj->getRotation().z}}
                });
        }

        // ���л���Դ
        sceneJson["lights"] = lightManager.serialize();

        return sceneJson;
    }

    // �� JSON ���س���
    void deserialize(const nlohmann::json& sceneJson) {
        // �����������
        gameObjects.clear();
        lightManager.clearLights();

        // �����л�����
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

        // �����л���Դ
        lightManager.deserialize(sceneJson["lights"]);
    }
};

#endif // SCENE_H
