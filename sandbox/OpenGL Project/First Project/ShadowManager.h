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
            // ������������ͼ���ڵ��Դ��Ӱ
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

            // �󶨵�֡����
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
        else // DirectionalLight �� SpotLight
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

            // �󶨵�֡����
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
        // ȷ�� shadowDatas �Ĵ�С�� lights һ��
        while (shadowDatas.size() < lights.size())
        {
            ShadowData newData;
            LightType type = lights[shadowDatas.size()]->getType();
            setupShadowResources(newData, 2048, type); // ��ʼ�ֱ���Ϊ4096
            shadowDatas.emplace_back(std::move(newData));
        }
        while (shadowDatas.size() > lights.size())
        {
            // ɾ���������Ӱ��Դ
            shadowDatas.back().resource.reset();
            shadowDatas.pop_back();
        }
    }

public:
    ShadowManager() = default;

    ~ShadowManager() = default;

    // �޸�Ϊ����������ͬ����ɫ��
    void generateShadowMaps(const std::vector<Light*>& lights, Scene& scene, Shader& shadowShader, Shader& pointShadowShader)
    {
        syncShadowDataWithLights(lights);

        GLint viewport[4];
        glGetIntegerv(GL_VIEWPORT, viewport);

        glCullFace(GL_FRONT); // ������Ӱ���ϵ�Peter PanningЧӦ
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
                // ���Դ��Ҫ��Ⱦ������
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

                // ����ͶӰ����
                glm::mat4 shadowProj = glm::perspective(glm::radians(90.0f), aspect, near, far_plane);

                // ����ÿ���������ͼ����
                std::vector<glm::mat4> shadowTransforms;
                shadowTransforms.push_back(shadowProj * glm::lookAt(pos, pos + glm::vec3(1.0, 0.0, 0.0), glm::vec3(0.0, -1.0, 0.0)));
                shadowTransforms.push_back(shadowProj * glm::lookAt(pos, pos + glm::vec3(-1.0, 0.0, 0.0), glm::vec3(0.0, -1.0, 0.0)));
                shadowTransforms.push_back(shadowProj * glm::lookAt(pos, pos + glm::vec3(0.0, 1.0, 0.0), glm::vec3(0.0, 0.0, 1.0)));
                shadowTransforms.push_back(shadowProj * glm::lookAt(pos, pos + glm::vec3(0.0, -1.0, 0.0), glm::vec3(0.0, 0.0, -1.0)));
                shadowTransforms.push_back(shadowProj * glm::lookAt(pos, pos + glm::vec3(0.0, 0.0, 1.0), glm::vec3(0.0, -1.0, 0.0)));
                shadowTransforms.push_back(shadowProj * glm::lookAt(pos, pos + glm::vec3(0.0, 0.0, -1.0), glm::vec3(0.0, -1.0, 0.0)));

                // ���ݾ��󵽼�����ɫ��
                for (unsigned int j = 0; j < 6; ++j)
                {
                    currentShadowShader.setMat4("shadowMatrices[" + std::to_string(j) + "]", shadowTransforms[j]);
                }
                currentShadowShader.setFloat("far_plane", far_plane);
                currentShadowShader.setVec3("lightPos", pos);

                // ��֡���������������������ͼ
                glBindFramebuffer(GL_FRAMEBUFFER, shadowData.resource->getFramebuffer());
                glViewport(0, 0, shadowData.resolution, shadowData.resolution);
                glClear(GL_DEPTH_BUFFER_BIT);

                // ��Ⱦ����
                scene.drawShadowMaps(currentShadowShader);

                glBindFramebuffer(GL_FRAMEBUFFER, 0);
            }
            else // DirectionalLight �� SpotLight
            {
                // ʹ�÷�����۹�Ƶ���ͼ����
                glm::mat4 lightSpaceMatrix = light->getProjectionMatrix() * light->getViewMatrix();
                currentShadowShader.setMat4("lightSpaceMatrix", lightSpaceMatrix);

                // ��2D��Ӱ��ͼ��֡����
                glBindFramebuffer(GL_FRAMEBUFFER, shadowData.resource->getFramebuffer());

                // ��Ⱦ��������Ӱ��ͼ
                glViewport(0, 0, shadowData.resolution, shadowData.resolution);
                glClear(GL_DEPTH_BUFFER_BIT);
                scene.drawShadowMaps(currentShadowShader);

                glBindFramebuffer(GL_FRAMEBUFFER, 0);
            }
        }

        // �ָ�OpenGL״̬
        glDisable(GL_POLYGON_OFFSET_FILL);
        glCullFace(GL_BACK);
        glViewport(viewport[0], viewport[1], viewport[2], viewport[3]);

        // ��鵱ǰ�󶨵�֡�����Ƿ�����
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
                // ɾ������Դ
                shadowData.resource.reset();
                // ���´�����Դ
                setupShadowResources(shadowData, newResolution, shadowData.type);
            }
        }
    }

    // ��ȡָ����Դ����Ӱ��ͼ
    GLuint getShadowTexture(int index) const
    {
        if (index < 0 || index >= static_cast<int>(shadowDatas.size()))
        {
            throw std::out_of_range("Invalid shadow texture index");
        }
        return shadowDatas[index].resource->getTexture();
    }

    // ��ȡָ����Դ�Ĺ�ռ���󣨽������ڷ����;۹�ƣ�
    glm::mat4 getLightSpaceMatrix(const std::vector<Light*>& lights, int index) const
    {
        if (index < 0 || index >= static_cast<int>(shadowDatas.size()))
        {
            throw std::out_of_range("Invalid light space matrix index");
        }

        const auto& light = lights[index];
        if (light->getType() == LightType::Point)
        {
            // ���Դû�е�һ�Ĺ�ռ����
            return glm::mat4(1.0f);
        }
        else
        {
            return light->getProjectionMatrix() * light->getViewMatrix();
        }
    }
};

#endif // SHADOW_MANAGER_H
