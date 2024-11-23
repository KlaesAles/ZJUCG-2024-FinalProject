#ifndef SHADOW_MANAGER_H
#define SHADOW_MANAGER_H

#include <vector>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "Light.h"
#include <GLFW/glfw3.h>
#include <glad/glad.h>

class ShadowManager {
private:
    struct ShadowData {
        GLuint shadowFBO;              // 帧缓冲对象
        GLuint shadowTexture;          // 阴影贴图
        glm::mat4 lightProjection;     // 光源的投影矩阵
        glm::mat4 lightView;           // 光源的视图矩阵
        LightType type;                // 光源类型
        int resolution;                // 阴影贴图分辨率
    };

    std::vector<ShadowData> shadowDatas; // 所有光源的阴影数据

public:
    ShadowManager() = default;

    ~ShadowManager() {
        for (const auto& data : shadowDatas) {
            glDeleteFramebuffers(1, &data.shadowFBO);
            glDeleteTextures(1, &data.shadowTexture);
        }
    }

    // 添加光源及其阴影配置
    void addLight(const Light& light, int resolution = 1024) {
        ShadowData shadowData;
        shadowData.type = light.getType();
        shadowData.resolution = resolution;

        glGenFramebuffers(1, &shadowData.shadowFBO);

        if (shadowData.type == LightType::Point) {
            // 点光源：立方体贴图
            glGenTextures(1, &shadowData.shadowTexture);
            glBindTexture(GL_TEXTURE_CUBE_MAP, shadowData.shadowTexture);
            for (unsigned int i = 0; i < 6; ++i) {
                glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_DEPTH_COMPONENT,
                             resolution, resolution, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
            }
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
            glBindFramebuffer(GL_FRAMEBUFFER, shadowData.shadowFBO);
            glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, shadowData.shadowTexture, 0);
            glDrawBuffer(GL_NONE);
            glReadBuffer(GL_NONE);
        } else {
            // 方向光或聚光灯：2D 阴影贴图
            glGenTextures(1, &shadowData.shadowTexture);
            glBindTexture(GL_TEXTURE_2D, shadowData.shadowTexture);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, resolution, resolution, 0,
                         GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
            float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
            glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
            glBindFramebuffer(GL_FRAMEBUFFER, shadowData.shadowFBO);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, shadowData.shadowTexture, 0);
            glDrawBuffer(GL_NONE);
            glReadBuffer(GL_NONE);
        }

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        shadowData.lightProjection = light.getProjectionMatrix();
        shadowData.lightView = light.getViewMatrix();

        shadowDatas.push_back(shadowData);
    }

    // 生成所有光源的阴影贴图
    void generateShadowMaps(const std::vector<Light*>& lights, const std::vector<GameObject*>& sceneObjects, GLuint shadowShader) {
        for (size_t i = 0; i < lights.size(); ++i) {
            const auto& shadowData = shadowDatas[i];
            const auto* light = lights[i];

            glBindFramebuffer(GL_FRAMEBUFFER, shadowData.shadowFBO);
            glViewport(0, 0, shadowData.resolution, shadowData.resolution);
            glClear(GL_DEPTH_BUFFER_BIT);

            glUseProgram(shadowShader);
            GLuint lightSpaceMatrixLoc = glGetUniformLocation(shadowShader, "lightSpaceMatrix");
            glUniformMatrix4fv(lightSpaceMatrixLoc, 1, GL_FALSE, glm::value_ptr(shadowData.lightProjection * shadowData.lightView));

            for (const auto& object : sceneObjects) {
                object->draw(shadowShader); // 假设每个 GameObject 实现了 draw() 方法
            }

            glBindFramebuffer(GL_FRAMEBUFFER, 0);
        }
    }

    // 获取光源的阴影贴图
    GLuint getShadowTexture(int index) const {
        if (index < 0 || index >= shadowDatas.size()) {
            throw std::out_of_range("Invalid shadow texture index");
        }
        return shadowDatas[index].shadowTexture;
    }

    // 获取光源的光源空间矩阵
    glm::mat4 getLightSpaceMatrix(int index) const {
        if (index < 0 || index >= shadowDatas.size()) {
            throw std::out_of_range("Invalid light space matrix index");
        }
        return shadowDatas[index].lightProjection * shadowDatas[index].lightView;
    }
};

#endif