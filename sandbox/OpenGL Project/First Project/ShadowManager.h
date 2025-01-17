// ShadowManager.h
#ifndef SHADOW_MANAGER_H
#define SHADOW_MANAGER_H

#include <vector>
#include <memory>
#include <stdexcept>
#include <string>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glad/glad.h>
#include "Light.h"
#include "Scene.h"
#include "Shader.h"

class ShadowManager
{
private:
    class ShadowResource
    {
    public:
        GLuint framebuffer = 0;
        GLuint texture = 0;
        bool isCubeMap = false;

        ShadowResource() = default;

        ShadowResource(GLuint fb, GLuint tex, bool cubeMap = false)
            : framebuffer(fb), texture(tex), isCubeMap(cubeMap) {
        }

        ~ShadowResource()
        {
            if (framebuffer) glDeleteFramebuffers(1, &framebuffer);
            if (texture)
            {
                glDeleteTextures(1, &texture);
            }
        }

        GLuint getFramebuffer() const { return framebuffer; }
        GLuint getTexture() const { return texture; }

    };

    struct ShadowData
    {
        std::unique_ptr<ShadowResource> resource;
        LightType type;
        int resolution;
    };

    std::vector<ShadowData> shadowDatas;

    // Helper function to create shadow resources
    void setupShadowResources(ShadowData& data, int resolution, LightType type)
    {
        GLuint fb, tex;
        glGenFramebuffers(1, &fb);
        glGenTextures(1, &tex);

        if (type == LightType::Point)
        {
            // 创建立方体贴图用于点光源阴影
            glBindTexture(GL_TEXTURE_CUBE_MAP, tex);
            for (unsigned int i = 0; i < 6; ++i)
            {
                glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_DEPTH_COMPONENT,
                    resolution, resolution, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
            }
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_COMPARE_FUNC, GL_LESS);

            // 绑定到帧缓冲
            glBindFramebuffer(GL_FRAMEBUFFER, fb);
            glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, tex, 0);
            glDrawBuffer(GL_NONE);
            glReadBuffer(GL_NONE);
            checkFramebufferStatus(fb);

            glBindFramebuffer(GL_FRAMEBUFFER, 0);

            data.resource = std::make_unique<ShadowResource>(fb, tex, true);
            data.resolution = resolution;
            data.type = LightType::Point;
        }
        else // DirectionalLight 或 SpotLight
        {
            glBindTexture(GL_TEXTURE_2D, tex);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, resolution, resolution, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
            float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
            glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LESS);

            // 绑定到帧缓冲
            glBindFramebuffer(GL_FRAMEBUFFER, fb);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, tex, 0);
            glDrawBuffer(GL_NONE);
            glReadBuffer(GL_NONE);
            checkFramebufferStatus(fb);

            glBindFramebuffer(GL_FRAMEBUFFER, 0);

            data.resource = std::make_unique<ShadowResource>(fb, tex, false);
            data.resolution = resolution;
            data.type = type;
        }
    }

    void checkFramebufferStatus(GLuint framebuffer)
    {
        glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
        GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
        if (status != GL_FRAMEBUFFER_COMPLETE)
        {
            throw std::runtime_error("Framebuffer incomplete: " + std::to_string(status));
        }
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    void syncShadowDataWithLights(const std::vector<Light*>& lights)
    {
        // 确保 shadowDatas 的大小与 lights 一致
        while (shadowDatas.size() < lights.size())
        {
            ShadowData newData;
            LightType type = lights[shadowDatas.size()]->getType();
            setupShadowResources(newData, 2048, type); // 初始分辨率为4096
            shadowDatas.emplace_back(std::move(newData));
        }
        while (shadowDatas.size() > lights.size())
        {
            // 删除多余的阴影资源
            shadowDatas.back().resource.reset();
            shadowDatas.pop_back();
        }
    }

public:
    ShadowManager() = default;

    ~ShadowManager() = default;

    // 修改为接受两个不同的着色器
    void generateShadowMaps(const std::vector<Light*>& lights, Scene& scene, Shader& shadowShader, Shader& pointShadowShader)
    {
        syncShadowDataWithLights(lights);

        GLint viewport[4];
        glGetIntegerv(GL_VIEWPORT, viewport);

        glCullFace(GL_FRONT); // 减少阴影面上的Peter Panning效应
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_POLYGON_OFFSET_FILL);
        glPolygonOffset(1.0f, 1.0f);

        for (size_t i = 0; i < lights.size(); ++i)
        {
            Light* light = lights[i];
            ShadowData& shadowData = shadowDatas[i];

            Shader& currentShadowShader = (light->getType() == LightType::Point) ? pointShadowShader : shadowShader;
            currentShadowShader.use();

            if (light->getType() == LightType::Point)
            {
                // 点光源需要渲染六个面
                PointLight* pointLight = dynamic_cast<PointLight*>(light);
                if (!pointLight)
                    continue;

                glm::vec3 pos = pointLight->getPosition();
                float near = pointLight->getNearPlane();
                float far_plane = pointLight->getFarPlane();
                GLfloat aspect = 1.0f;

                /*
                std::cout << "==== PointLight Params ====" << std::endl;
                std::cout << "pos       = (" << pos.x << ", " << pos.y << ", " << pos.z << ")" << std::endl;
                std::cout << "near      = " << near << std::endl;
                std::cout << "far_plane = " << far_plane << std::endl;
                std::cout << "aspect    = " << aspect << std::endl;
                std::cout << "===========================" << std::endl;
                */

                // 设置投影矩阵
                glm::mat4 shadowProj = glm::perspective(glm::radians(90.0f), aspect, near, far_plane);

                // 创建每个方向的视图矩阵
                std::vector<glm::mat4> shadowTransforms;
                shadowTransforms.push_back(shadowProj * glm::lookAt(pos, pos + glm::vec3(1.0, 0.0, 0.0), glm::vec3(0.0, -1.0, 0.0)));
                shadowTransforms.push_back(shadowProj * glm::lookAt(pos, pos + glm::vec3(-1.0, 0.0, 0.0), glm::vec3(0.0, -1.0, 0.0)));
                shadowTransforms.push_back(shadowProj * glm::lookAt(pos, pos + glm::vec3(0.0, 1.0, 0.0), glm::vec3(0.0, 0.0, 1.0)));
                shadowTransforms.push_back(shadowProj * glm::lookAt(pos, pos + glm::vec3(0.0, -1.0, 0.0), glm::vec3(0.0, 0.0, -1.0)));
                shadowTransforms.push_back(shadowProj * glm::lookAt(pos, pos + glm::vec3(0.0, 0.0, 1.0), glm::vec3(0.0, -1.0, 0.0)));
                shadowTransforms.push_back(shadowProj * glm::lookAt(pos, pos + glm::vec3(0.0, 0.0, -1.0), glm::vec3(0.0, -1.0, 0.0)));

                // 传递矩阵到几何着色器
                for (unsigned int j = 0; j < 6; ++j)
                {
                    currentShadowShader.setMat4("shadowMatrices[" + std::to_string(j) + "]", shadowTransforms[j]);
                }
                currentShadowShader.setFloat("far_plane", far_plane);
                currentShadowShader.setVec3("lightPos", pos);

                // 绑定帧缓冲以生成立方体深度贴图
                glBindFramebuffer(GL_FRAMEBUFFER, shadowData.resource->getFramebuffer());
                glViewport(0, 0, shadowData.resolution, shadowData.resolution);
                glClear(GL_DEPTH_BUFFER_BIT);

                // 渲染场景
                scene.drawShadowMaps(currentShadowShader);

                glBindFramebuffer(GL_FRAMEBUFFER, 0);
            }
            else // DirectionalLight 或 SpotLight
            {
                // 使用方向光或聚光灯的视图矩阵
                glm::mat4 lightSpaceMatrix = light->getProjectionMatrix() * light->getViewMatrix();
                currentShadowShader.setMat4("lightSpaceMatrix", lightSpaceMatrix);

                // 绑定2D阴影贴图的帧缓冲
                glBindFramebuffer(GL_FRAMEBUFFER, shadowData.resource->getFramebuffer());

                // 渲染场景到阴影贴图
                glViewport(0, 0, shadowData.resolution, shadowData.resolution);
                glClear(GL_DEPTH_BUFFER_BIT);
                scene.drawShadowMaps(currentShadowShader);

                glBindFramebuffer(GL_FRAMEBUFFER, 0);
            }
        }

        // 恢复OpenGL状态
        glDisable(GL_POLYGON_OFFSET_FILL);
        glCullFace(GL_BACK);
        glViewport(viewport[0], viewport[1], viewport[2], viewport[3]);

        // 检查当前绑定的帧缓冲是否完整
        GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
        if (status != GL_FRAMEBUFFER_COMPLETE)
        {
            std::cerr << "Framebuffer not complete! Status: " << status << std::endl;
        }
    }

    void updateShadowResolution(int newResolution)
    {
        for (auto& shadowData : shadowDatas)
        {
            if (shadowData.resolution != newResolution)
            {
                // 删除旧资源
                shadowData.resource.reset();
                // 重新创建资源
                setupShadowResources(shadowData, newResolution, shadowData.type);
            }
        }
    }

    // 获取指定光源的阴影贴图
    GLuint getShadowTexture(int index) const
    {
        if (index < 0 || index >= static_cast<int>(shadowDatas.size()))
        {
            throw std::out_of_range("Invalid shadow texture index");
        }
        return shadowDatas[index].resource->getTexture();
    }

    // 获取指定光源的光空间矩阵（仅适用于方向光和聚光灯）
    glm::mat4 getLightSpaceMatrix(const std::vector<Light*>& lights, int index) const
    {
        if (index < 0 || index >= static_cast<int>(shadowDatas.size()))
        {
            throw std::out_of_range("Invalid light space matrix index");
        }

        const auto& light = lights[index];
        if (light->getType() == LightType::Point)
        {
            // 点光源没有单一的光空间矩阵
            return glm::mat4(1.0f);
        }
        else
        {
            return light->getProjectionMatrix() * light->getViewMatrix();
        }
    }
};

#endif // SHADOW_MANAGER_H
