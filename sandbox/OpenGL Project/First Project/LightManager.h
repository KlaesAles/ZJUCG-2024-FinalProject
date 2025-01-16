#ifndef LIGHT_MANAGER_H
#define LIGHT_MANAGER_H

#include <vector>
#include <memory>
#include <stdexcept>
#include "Light.h"
#include "shader.h"

class LightManager {
private:
    std::vector<std::shared_ptr<Light>> lights; // 存储光源
    unsigned int ubo;                           // UBO ID
    unsigned int maxLights;                     // 最大支持的光源数

    struct LightData {
        alignas(16) glm::vec4 position;    // 光源位置 (16字节对齐)
        alignas(16) glm::vec4 direction;   // 光源方向 (16字节对齐)
        alignas(16) glm::vec4 color;       // 光源颜色和强度 (16字节对齐)
        alignas(16) glm::vec4 params;      // 额外参数 (16字节对齐)
    };

public:
    LightManager(unsigned int maxLights = 16)
        : maxLights(maxLights) {
        const size_t blockSize = 4 * sizeof(glm::vec4) * maxLights; // 4个数组，每个数组maxLights个vec4
        // 初始化 UBO
        glGenBuffers(1, &ubo);
        glBindBuffer(GL_UNIFORM_BUFFER, ubo);
        glBufferData(GL_UNIFORM_BUFFER, blockSize, nullptr, GL_DYNAMIC_DRAW);
        glBindBuffer(GL_UNIFORM_BUFFER, 0);
    }

    ~LightManager() {
        glDeleteBuffers(1, &ubo);
    }

    // 添加光源
    void addLight(std::shared_ptr<Light> light) {
        if (lights.size() < maxLights) {
            lights.push_back(light);
        }
        else {
            throw std::runtime_error("Maximum number of lights exceeded!");
        }
    }

    // 移除光源
    void removeLight(int index) {
        if (index >= 0 && index < static_cast<int>(lights.size())) {
            lights.erase(lights.begin() + index);
        }
        else {
            throw std::runtime_error("Invalid light index!");
        }
    }

    // 获取光源数量
    size_t getLightCount() const {
        return lights.size();
    }

    // 获取指定索引的光源
    std::shared_ptr<Light> getLight(int index) const {
        if (index >= 0 && index < static_cast<int>(lights.size())) {
            return lights[index];
        }
        else {
            throw std::runtime_error("Invalid light index!");
        }
    }

    // 获取所有光源的引用
    const std::vector<std::shared_ptr<Light>>& getLights() const {
        return lights;
    }

    // 更新 UBO 数据
    void updateUBO() const {
        // 为每种数据类型创建临时数组
        std::vector<glm::vec4> positions(maxLights, glm::vec4(0.0f));
        std::vector<glm::vec4> directions(maxLights, glm::vec4(0.0f));
        std::vector<glm::vec4> colors(maxLights, glm::vec4(0.0f));
        std::vector<glm::vec4> params(maxLights, glm::vec4(0.0f));

        // 填充数据
        for (size_t i = 0; i < lights.size(); ++i) {
            const auto& light = lights[i];

            // 设置颜色和强度
            colors[i] = glm::vec4(light->getColor(), light->getIntensity());

            switch (light->getType()) {
            case LightType::Directional: {
                directions[i] = glm::vec4(glm::normalize(light->getDirection()), 0.0f);
                positions[i] = glm::vec4(0.0f);
                params[i] = glm::vec4(0.0f, 0.0f, 0.0f, 0.0f);
                break;
            }
            case LightType::Point: {
                positions[i] = glm::vec4(light->getPosition(), 1.0f);
                directions[i] = glm::vec4(0.0f);
                params[i] = glm::vec4(0.0f, 1.0f, 0.0f, 0.0f);
                break;
            }
            case LightType::Spot: {
                positions[i] = glm::vec4(light->getPosition(), 1.0f);
                directions[i] = glm::vec4(glm::normalize(light->getDirection()), 1.0f);
                auto* spotLight = static_cast<const SpotLight*>(light.get());
                params[i] = glm::vec4(spotLight->getCutoffAngle(), 2.0f, 0.0f, 0.0f);
                break;
            }
            }
        }

        // 绑定缓冲区并更新数据
        glBindBuffer(GL_UNIFORM_BUFFER, ubo);

        // 更新每个数组的数据，注意偏移量
        size_t offset = 0;
        const size_t arraySize = sizeof(glm::vec4) * maxLights;

        glBufferSubData(GL_UNIFORM_BUFFER, offset, arraySize, positions.data());
        offset += arraySize;
        glBufferSubData(GL_UNIFORM_BUFFER, offset, arraySize, directions.data());
        offset += arraySize;
        glBufferSubData(GL_UNIFORM_BUFFER, offset, arraySize, colors.data());
        offset += arraySize;
        glBufferSubData(GL_UNIFORM_BUFFER, offset, arraySize, params.data());

        glBindBuffer(GL_UNIFORM_BUFFER, 0);
    }

    // 绑定 UBO 到指定 Shader 的绑定点
    void bindUBOToShader(const Shader& shader, unsigned int bindingPoint = 0) const {
        // 获取Uniform块索引
        unsigned int blockIndex = glGetUniformBlockIndex(shader.ID, "LightBlock");
        if (blockIndex != GL_INVALID_INDEX) {
            // 将Uniform块绑定到绑定点
            glUniformBlockBinding(shader.ID, blockIndex, bindingPoint);
            // 将UBO绑定到相同的绑定点
            glBindBufferBase(GL_UNIFORM_BUFFER, bindingPoint, ubo);
        }
        else {
            std::cout << "Warning: LightBlock not found in shader program" << std::endl;
        }
    }
};

#endif // LIGHT_MANAGER_H
