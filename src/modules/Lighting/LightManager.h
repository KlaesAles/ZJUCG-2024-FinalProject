#ifndef LIGHT_MANAGER_H
#define LIGHT_MANAGER_H

#include <vector>
#include <memory>
#include "Light.h"
#include "Shader.h"

class LightManager {
private:
    std::vector<std::shared_ptr<Light>> lights; // 存储光源
    unsigned int ubo;                           // UBO ID
    unsigned int maxLights;                     // 最大支持的光源数

    struct LightData {
        glm::vec4 position;    // 光源位置 (点光源、聚光灯)
        glm::vec4 direction;   // 光源方向 (方向光、聚光灯)
        glm::vec4 color;       // 光源颜色和强度 (xyz: 颜色, w: 强度)
        glm::vec4 params;      // 额外参数 (cutoffAngle, type, 保留)
    };

public:
    LightManager(unsigned int maxLights = 16)
        : maxLights(maxLights) {
        // 初始化 UBO
        glGenBuffers(1, &ubo);
        glBindBuffer(GL_UNIFORM_BUFFER, ubo);
        glBufferData(GL_UNIFORM_BUFFER, maxLights * sizeof(LightData), nullptr, GL_DYNAMIC_DRAW);
        glBindBuffer(GL_UNIFORM_BUFFER, 0);
    }

    ~LightManager() {
        glDeleteBuffers(1, &ubo);
    }

    // 添加光源
    void addLight(std::shared_ptr<Light> light) {
        if (lights.size() < maxLights) {
            lights.push_back(light);
        } else {
            throw std::runtime_error("Maximum number of lights exceeded!");
        }
    }

    // 移除光源
    void removeLight(int index) {
        if (index >= 0 && index < lights.size()) {
            lights.erase(lights.begin() + index);
        } else {
            throw std::runtime_error("Invalid light index!");
        }
    }

    // 更新 UBO 数据
    void updateUBO() const {
        std::vector<LightData> lightData(lights.size());

        for (size_t i = 0; i < lights.size(); ++i) {
            const auto& light = lights[i];
            LightData& data = lightData[i];

            data.color = glm::vec4(light->getColor(), light->getIntensity());

            switch (light->getType()) {
                case LightType::Directional:
                    data.direction = glm::vec4(light->getDirection(), 0.0f); // w=0表示方向光
                    data.position = glm::vec4(0.0f);                         // 方向光没有位置
                    data.params = glm::vec4(0.0f, static_cast<float>(LightType::Directional), 0.0f, 0.0f);
                    break;

                case LightType::Point:
                    data.position = glm::vec4(light->getPosition(), 1.0f);  // w=1表示点光源
                    data.direction = glm::vec4(0.0f);                       // 点光源没有方向
                    data.params = glm::vec4(0.0f, static_cast<float>(LightType::Point), 0.0f, 0.0f);
                    break;

                case LightType::Spot:
                    data.position = glm::vec4(light->getPosition(), 1.0f);  // 聚光灯有位置
                    data.direction = glm::vec4(light->getDirection(), 1.0f);
                    data.params = glm::vec4(static_cast<const SpotLight*>(light.get())->getCutoffAngle(),
                                            static_cast<float>(LightType::Spot), 0.0f, 0.0f);
                    break;
            }
        }

        // 更新 UBO 数据
        glBindBuffer(GL_UNIFORM_BUFFER, ubo);
        glBufferSubData(GL_UNIFORM_BUFFER, 0, lights.size() * sizeof(LightData), lightData.data());
        glBindBuffer(GL_UNIFORM_BUFFER, 0);
    }

    // 绑定 UBO 到指定 Shader 的绑定点
    void bindUBOToShader(const Shader& shader, unsigned int bindingPoint = 0) const {
        unsigned int blockIndex = glGetUniformBlockIndex(shader.ID, "LightBlock");
        glUniformBlockBinding(shader.ID, blockIndex, bindingPoint);

        // 将 UBO 绑定到绑定点
        glBindBufferBase(GL_UNIFORM_BUFFER, bindingPoint, ubo);
    }
};

#endif // LIGHT_MANAGER_H