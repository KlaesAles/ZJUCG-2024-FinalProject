#ifndef SHADOW_MANAGER_H
#define SHADOW_MANAGER_H

#include <vector>
#include <memory>
#include <stdexcept>
#include <string>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "Light.h"
#include "Scene.h"
#include <GLFW/glfw3.h>
#include <glad/glad.h>

class ShadowManager
{
private:
    class ShadowResource
    {
    public:
        GLuint framebuffer = 0;
        GLuint texture = 0;

        ShadowResource() = default;

        ShadowResource(GLuint fb, GLuint tex) : framebuffer(fb), texture(tex) {}

        ~ShadowResource()
        {
            if (framebuffer) glDeleteFramebuffers(1, &framebuffer);
            if (texture) glDeleteTextures(1, &texture);
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
    void setupShadowResources(ShadowData &data, int resolution, bool isPointLight)
    {
        GLuint fb, tex;
        glGenFramebuffers(1, &fb);
        glGenTextures(1, &tex);

        if (isPointLight)
        {
            glBindTexture(GL_TEXTURE_CUBE_MAP, tex);
            for (int i = 0; i < 6; ++i)
            {
                glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_DEPTH_COMPONENT,
                             resolution, resolution, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
            }
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

            glBindFramebuffer(GL_FRAMEBUFFER, fb);
            glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, tex, 0);
        }
        else
        {
            glBindTexture(GL_TEXTURE_2D, tex);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, resolution, resolution, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
            float borderColor[] = {1.0f, 1.0f, 1.0f, 1.0f};
            glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

            glBindFramebuffer(GL_FRAMEBUFFER, fb);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, tex, 0);
        }

        glDrawBuffer(GL_NONE);
        glReadBuffer(GL_NONE);
        checkFramebufferStatus(fb);

        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        data.resource = std::make_unique<ShadowResource>(fb, tex);
        data.resolution = resolution;
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

    void syncShadowDataWithLights(const std::vector<Light *> &lights)
    {
        while (shadowDatas.size() < lights.size())
        {
            shadowDatas.emplace_back(ShadowData{});
            setupShadowResources(shadowDatas.back(), 1024, lights[shadowDatas.size() - 1]->getType() == LightType::Point);
        }
        while (shadowDatas.size() > lights.size())
        {
            shadowDatas.pop_back();
        }
    }

public:
    ShadowManager() = default;

    ~ShadowManager() = default;

    void generateShadowMaps(const std::vector<Light *> &lights, Scene& scene, GLuint shadowShader)
    {
        syncShadowDataWithLights(lights);

        GLint viewport[4];
        glGetIntegerv(GL_VIEWPORT, viewport);

        glCullFace(GL_FRONT);
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_POLYGON_OFFSET_FILL);
        glPolygonOffset(1.0f, 0.001f);

        for (size_t i = 0; i < lights.size(); ++i)
        {
            const auto &light = lights[i];
            auto &shadowData = shadowDatas[i];

            glBindFramebuffer(GL_FRAMEBUFFER, shadowData.resource->getFramebuffer());
            glViewport(0, 0, shadowData.resolution, shadowData.resolution);
            glClear(GL_DEPTH_BUFFER_BIT);


            if (light->getType() == LightType::Point)
            {
                /*
                glUseProgram(shadowShader);

                const auto viewMatrices = static_cast<PointLight *>(light)->getViewMatrices();
                const auto projectionMatrix = light->getProjectionMatrix();
                std::vector<glm::mat4> lightSpaceMatrices;

                for (const auto &viewMatrix : viewMatrices)
                {
                    lightSpaceMatrices.push_back(projectionMatrix * viewMatrix);
                }

                glUniformMatrix4fv(glGetUniformLocation(shadowShader, "lightSpaceMatrices"), 6, GL_FALSE, glm::value_ptr(lightSpaceMatrices[0]));
                */
                continue;
            }
            else
            {
                glUseProgram(shadowShader);

                glm::mat4 lightSpaceMatrix = light->getProjectionMatrix() * light->getViewMatrix();
                glUniformMatrix4fv(glGetUniformLocation(shadowShader, "lightSpaceMatrix"), 1, GL_FALSE, glm::value_ptr(lightSpaceMatrix));
            }

            scene.drawShadowMaps(shadowShader);
        }

        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        {
            std::cerr << "Framebuffer not complete!" << std::endl;
        }

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glDisable(GL_POLYGON_OFFSET_FILL);
        glCullFace(GL_BACK);
        glViewport(viewport[0], viewport[1], viewport[2], viewport[3]);
    }

    void updateShadowResolution(int newResolution)
    {
        for (auto &shadowData : shadowDatas)
        {
            if (shadowData.resolution != newResolution)
            {
                setupShadowResources(shadowData, newResolution, shadowData.type == LightType::Point);
            }
        }
    }

    GLuint getShadowTexture(int index) const
    {
        if (index < 0 || index >= shadowDatas.size())
        {
            throw std::out_of_range("Invalid shadow texture index");
        }
        return shadowDatas[index].resource->getTexture();
    }

    glm::mat4 getLightSpaceMatrix(const std::vector<Light *> &lights, int index) const
    {
        const auto &light = lights[index];
        if (index < 0 || index >= shadowDatas.size())
        {
            throw std::out_of_range("Invalid light space matrix index");
        }

        // 检查光源类型
        if (light->getType() == LightType::Point)
        {
            return glm::mat4(1.0f);
        }
        else
        {
            return light->getProjectionMatrix() * light->getViewMatrix();
        }
    }
};

#endif