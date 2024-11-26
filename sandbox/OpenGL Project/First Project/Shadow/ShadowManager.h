#ifndef SHADOW_MANAGER_H
#define SHADOW_MANAGER_H

#include <vector>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "Light.h"
#include "GameObject.h"
#include <GLFW/glfw3.h>
#include <glad/glad.h>

class ShadowManager
{
private:
    struct ShadowData
    {
        GLuint shadowFBO;          // 帧缓冲对象
        GLuint shadowTexture;      // 阴影贴图
        glm::mat4 lightProjection; // 光源的投影矩阵
        glm::mat4 lightView;       // 光源的视图矩阵
        LightType type;            // 光源类型
        int resolution;            // 阴影贴图分辨率
    };

    std::vector<ShadowData> shadowDatas; // 所有光源的阴影数据

public:
    ShadowManager() = default;

    ~ShadowManager()
    {
        for (const auto &data : shadowDatas)
        {
            glDeleteFramebuffers(1, &data.shadowFBO);
            glDeleteTextures(1, &data.shadowTexture);
        }
    }

    // 添加光源及其阴影配置
    void addLight(const Light &light, int resolution = 1024)
    {
        ShadowData shadowData;
        shadowData.type = light.getType();
        shadowData.resolution = resolution;

        glGenFramebuffers(1, &shadowData.shadowFBO);

        if (shadowData.type == LightType::Point)
        {
            // 点光源：立方体贴图
            glGenTextures(1, &shadowData.shadowTexture);
            glBindTexture(GL_TEXTURE_CUBE_MAP, shadowData.shadowTexture);
            for (unsigned int i = 0; i < 6; ++i)
            {
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

            shadowData.lightProjection = light.getProjectionMatrix();
            shadowData.lightView = glm::mat4(1.0f); // 点光源的视图矩阵由6个面动态计算
        }
        else
        {
            // 方向光或聚光灯：2D 阴影贴图
            glGenTextures(1, &shadowData.shadowTexture);
            glBindTexture(GL_TEXTURE_2D, shadowData.shadowTexture);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, resolution, resolution, 0,
                         GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
            float borderColor[] = {1.0f, 1.0f, 1.0f, 1.0f};
            glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
            glBindFramebuffer(GL_FRAMEBUFFER, shadowData.shadowFBO);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, shadowData.shadowTexture, 0);
            glDrawBuffer(GL_NONE);
            glReadBuffer(GL_NONE);

            shadowData.lightProjection = light.getProjectionMatrix();
            shadowData.lightView = light.getViewMatrix();
        }

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        shadowDatas.push_back(shadowData);
    }

    void generateShadowMaps(const std::vector<Light *> &lights, const std::vector<GameObject *> &sceneObjects, GLuint shadowShader)
    {
        // 保存当前视口状态
        GLint viewport[4];
        glGetIntegerv(GL_VIEWPORT, viewport);

        // 设置面剔除
        glCullFace(GL_FRONT);

        // 启用深度偏移
        glEnable(GL_POLYGON_OFFSET_FILL);
        glPolygonOffset(4.0f, 4.0f);

        for (size_t i = 0; i < lights.size(); ++i)
        {
            const auto &shadowData = shadowDatas[i];
            const auto *light = lights[i];

            // 绑定当前光源的阴影FBO
            glBindFramebuffer(GL_FRAMEBUFFER, shadowData.shadowFBO);
            glViewport(0, 0, shadowData.resolution, shadowData.resolution);
            glClear(GL_DEPTH_BUFFER_BIT);

            // 设置视图和投影矩阵
            std::vector<glm::mat4> lightSpaceMatrices;
            if (light->getType() == LightType::Point)
            {
                // 点光源需要6个方向的深度贴图
                // 创建6个不同方向的视图矩阵
                std::vector<glm::vec3> directions = {
                    glm::vec3(1.0f, 0.0f, 0.0f),  // +X
                    glm::vec3(-1.0f, 0.0f, 0.0f), // -X
                    glm::vec3(0.0f, 1.0f, 0.0f),  // +Y
                    glm::vec3(0.0f, -1.0f, 0.0f), // -Y
                    glm::vec3(0.0f, 0.0f, 1.0f),  // +Z
                    glm::vec3(0.0f, 0.0f, -1.0f)  // -Z
                };

                std::vector<glm::vec3> upVectors = {
                    glm::vec3(0.0f, -1.0f, 0.0f), // +X
                    glm::vec3(0.0f, -1.0f, 0.0f), // -X
                    glm::vec3(0.0f, 0.0f, 1.0f),  // +Y
                    glm::vec3(0.0f, 0.0f, -1.0f), // -Y
                    glm::vec3(0.0f, -1.0f, 0.0f), // +Z
                    glm::vec3(0.0f, -1.0f, 0.0f)  // -Z
                };

                // 为每个方向计算光空间矩阵
                for (int i = 0; i < 6; ++i)
                {
                    glm::mat4 lightView = glm::lookAt(light->getPosition(), light->getPosition() + directions[i], upVectors[i]);
                    lightSpaceMatrices.push_back(shadowData.lightProjection * lightView);
                }
            }
            else
            {
                lightSpaceMatrices.push_back(shadowData.lightProjection * shadowData.lightView);
            }

            // 使用阴影映射着色器
            for (int i = 0; i < lightSpaceMatrices.size(); ++i)
            {
                std::string uniformName = "lightSpaceMatrices[" + std::to_string(i) + "]";
                glUniformMatrix4fv(glGetUniformLocation(shadowShader, uniformName.c_str()), 1, GL_FALSE, glm::value_ptr(lightSpaceMatrices[i]));
            }

            // 渲染场景物体
            for (const auto &object : sceneObjects)
            {
                object->draw(shadowShader);
            }
        }

        // 恢复默认帧缓冲
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        // 恢复状态
        glDisable(GL_POLYGON_OFFSET_FILL);
        glCullFace(GL_BACK);
        glViewport(viewport[0], viewport[1], viewport[2], viewport[3]);
    }

    // 获取光源的阴影贴图
    GLuint getShadowTexture(int index) const
    {
        if (index < 0 || index >= shadowDatas.size())
        {
            throw std::out_of_range("Invalid shadow texture index");
        }
        return shadowDatas[index].shadowTexture;
    }

    // 获取光源的光源空间矩阵
    glm::mat4 getLightSpaceMatrix(int index) const
    {
        if (index < 0 || index >= shadowDatas.size())
        {
            throw std::out_of_range("Invalid light space matrix index");
        }
        return shadowDatas[index].lightProjection * shadowDatas[index].lightView;
    }

    void updateShadowResolution(int newResolution)
    {
        for (auto &shadowData : shadowDatas)
        {
            shadowData.resolution = newResolution;

            // 删除原来的纹理和 FBO
            glDeleteFramebuffers(1, &shadowData.shadowFBO);
            glDeleteTextures(1, &shadowData.shadowTexture);

            // 重新生成 FBO 和阴影纹理
            glGenFramebuffers(1, &shadowData.shadowFBO);

            if (shadowData.type == LightType::Point)
            {
                // 点光源：立方体贴图
                glGenTextures(1, &shadowData.shadowTexture);
                glBindTexture(GL_TEXTURE_CUBE_MAP, shadowData.shadowTexture);
                for (unsigned int i = 0; i < 6; ++i)
                {
                    glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_DEPTH_COMPONENT,
                                 newResolution, newResolution, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
                }
                glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
                glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
                glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
                glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
                glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
            }
            else
            {
                // 平行光或者聚光灯：二维深度纹理
                glGenTextures(1, &shadowData.shadowTexture);
                glBindTexture(GL_TEXTURE_2D, shadowData.shadowTexture);
                glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, newResolution, newResolution, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
                float borderColor[] = {1.0f, 1.0f, 1.0f, 1.0f};
                glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
            }

            // 重新绑定 FBO
            glBindFramebuffer(GL_FRAMEBUFFER, shadowData.shadowFBO);
            if (shadowData.type == LightType::Point)
            {
                glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, shadowData.shadowTexture, 0);
            }
            else
            {
                glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, shadowData.shadowTexture, 0);
            }
            glDrawBuffer(GL_NONE);
            glReadBuffer(GL_NONE);
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
        }
    }

};

#endif