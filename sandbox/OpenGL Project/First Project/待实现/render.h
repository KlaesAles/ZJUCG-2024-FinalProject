#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <vector>
#include <string>
#include <memory>
#include <stb_image.h>       // ���ڼ�������
#include <iostream>

#include "Shader.h"          // ������һ�� Shader ���װ��ɫ������
#include "LightManager.h"    // ��Դ������
#include "ShadowManager.h"   // ��Ӱ������
#include "GameObject.h"      // ��Ϸ����
#include "Camera.h"          // ���
#include "Scene.h"

class Scene;

class Render {
public:
    // ���캯�������ܹ�Դ����������Ӱ�������ͳ���������
    Render(LightManager& lightMgr, ShadowManager& shadowMgr, Scene& scene);

    // ��ʼ����Ⱦ��Դ
    bool Initialize();

    // ��Ⱦһ֡
    void RenderFrame(const Camera& camera, bool debugView, int debugLightIndex, unsigned int diffuseMap, unsigned int specularMap);

    // ������Դ
    void Cleanup();

private:
    // ��Դ����Ӱ����������
    LightManager& lightManager;
    ShadowManager& shadowManager;

    // ��������
    Scene& scene;

    // ��ɫ��
    Shader lightingShader;
    Shader lightCubeShader;
    Shader shadowShader;

    // VAOs �� VBOs
    unsigned int planeVAO, planeVBO;
    unsigned int cubeVAO, cubeVBO;
    unsigned int lightCubeVAO;

    // ����
    unsigned int diffuseTexture;
    unsigned int specularTexture;

    // ˽�з���
    bool setupShaders();
    bool setupPlane();
    bool setupCube();
    bool setupLightCube();
    unsigned int loadTexture(const std::string& path);
};

Render::Render(LightManager& lightMgr, ShadowManager& shadowMgr, Scene& scn)
    : lightManager(lightMgr), shadowManager(shadowMgr), scene(scn),
    lightingShader("./shader/Shadow Test.vs", "./shader/Shadow Test.fs"),
    lightCubeShader("./shader/light_cube.vs", "./shader/light_cube.fs"),
    shadowShader("./Shadow/shadow.vs", "./Shadow/shadow.fs"),
    planeVAO(0), planeVBO(0),
    cubeVAO(0), cubeVBO(0),
    lightCubeVAO(0),
    diffuseTexture(0), specularTexture(0) {
}

bool Render::Initialize()
{
    if (!setupShaders()) {
        std::cerr << "Failed to set up shaders." << std::endl;
        return false;
    }

    if (!setupPlane()) {
        std::cerr << "Failed to set up plane." << std::endl;
        return false;
    }

    if (!setupCube()) {
        std::cerr << "Failed to set up cube." << std::endl;
        return false;
    }

    if (!setupLightCube()) {
        std::cerr << "Failed to set up light cube." << std::endl;
        return false;
    }

    // ��������
    diffuseTexture = loadTexture("./assets/container2.png");
    specularTexture = loadTexture("./assets/container2_specular.png");
    if (diffuseTexture == 0 || specularTexture == 0) {
        std::cerr << "Failed to load textures." << std::endl;
        return false;
    }

    // ������ɫ��
    lightingShader.use();
    lightingShader.setInt("material.diffuse", 0);
    lightingShader.setInt("material.specular", 1);

    return true;
}

bool Render::setupShaders()
{
    // ��ɫ���Ѿ��ڹ��캯���м��أ�����������
    // ����������������ɫ�����ã��� UBO��SSBO ��
    return true;
}

bool Render::setupPlane()
{
    // ��ȡ����� VAO �� VBO ͨ����������
    // ���� Scene ���Ѿ������˵���� GameObject �������� VAO/VBO
    // �����Ҫ������������ VAO/VBO�������� Scene ���ṩ�ӿڻ�ȡ��Щ����
    // ������� Render ���𴴽������ VAO/VBO����ǰ������

    // ʾ������� Scene �����𴴽����� VAO/VBO���� Render ��Ҫ��������
    // �ο�֮ǰ�� setupPlane ����

    // ������� Render ������������
    float planeVertices[] = {
        // positions            // normals         // texture coords
         25.0f, -0.5f,  25.0f,  0.0f, 1.0f, 0.0f,  25.0f,  0.0f,
        -25.0f, -0.5f,  25.0f,  0.0f, 1.0f, 0.0f,   0.0f,  0.0f,
        -25.0f, -0.5f, -25.0f,  0.0f, 1.0f, 0.0f,   0.0f, 25.0f,

         25.0f, -0.5f,  25.0f,  0.0f, 1.0f, 0.0f,  25.0f,  0.0f,
        -25.0f, -0.5f, -25.0f,  0.0f, 1.0f, 0.0f,   0.0f, 25.0f,
         25.0f, -0.5f, -25.0f,  0.0f, 1.0f, 0.0f,  25.0f, 25.0f
    };

    glGenVertexArrays(1, &planeVAO);
    glGenBuffers(1, &planeVBO);

    glBindBuffer(GL_ARRAY_BUFFER, planeVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(planeVertices), planeVertices, GL_STATIC_DRAW);

    glBindVertexArray(planeVAO);
    // λ������
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    // ����������
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    // ������������
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    glBindVertexArray(0);
    return true;
}

bool Render::setupCube()
{
    float cubeVertices[] = {
        // positions          // normals           // texture coords
        -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f, 0.0f,
         0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f, 0.0f,
         0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f, 1.0f,
         0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f, 1.0f,
        -0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f, 1.0f,
        -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f, 0.0f,

        // ... �����涥�����ݣ�ʡ�ԣ� ...
        // �ܹ�36������
    };

    glGenVertexArrays(1, &cubeVAO);
    glGenBuffers(1, &cubeVBO);

    glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cubeVertices), cubeVertices, GL_STATIC_DRAW);

    glBindVertexArray(cubeVAO);
    // λ������
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    // ����������
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    // ������������
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    glBindVertexArray(0);
    return true;
}

bool Render::setupLightCube()
{
    glGenVertexArrays(1, &lightCubeVAO);
    glBindVertexArray(lightCubeVAO);

    glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
    // ֻ��Ҫλ������
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glBindVertexArray(0);
    return true;
}

unsigned int Render::loadTexture(const std::string& path)
{
    unsigned int textureID;
    glGenTextures(1, &textureID);

    int width, height, nrComponents;
    stbi_set_flip_vertically_on_load(true); // ������Ҫ�Ƿ�תͼƬ
    unsigned char* data = stbi_load(path.c_str(), &width, &height, &nrComponents, 0);
    if (data)
    {
        GLenum format;
        if (nrComponents == 1)
            format = GL_RED;
        else if (nrComponents == 3)
            format = GL_RGB;
        else if (nrComponents == 4)
            format = GL_RGBA;

        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        // �����������
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        stbi_image_free(data);
    }
    else
    {
        std::cerr << "Texture failed to load at path: " << path << std::endl;
        stbi_image_free(data);
        return 0;
    }

    return textureID;
}

void Render::RenderFrame(const Camera& camera, bool debugView, int debugLightIndex, unsigned int diffuseMap, unsigned int specularMap)
{
    // 1. ��Ⱦ��Ӱ��ͼ
    std::vector<Light*> lights;
    std::vector<GameObject*> objects;

    // �ռ���������ָ��
    const auto& sceneObjects = scene.GetGameObjects();
    for (const auto& obj : sceneObjects) {
        objects.push_back(obj.get());
    }

    // �ռ���Դָ��
    for (size_t i = 0; i < lightManager.getLightCount(); ++i) {
        lights.push_back(lightManager.getLight(i).get());
    }

    // ������Ӱ��ͼ�ֱ��ʣ����Ը�����Ҫ������
    shadowManager.updateShadowResolution(4096);

    // ��̬������Ӱ��ͼ
    shadowManager.generateShadowMaps(lights, objects, shadowShader.ID);

    // 2. �����Ļ
    glViewport(0, 0, Common::SCR_WIDTH, Common::SCR_HEIGHT);
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // 3. ���¹�Դ��λ�úͷ������磬�۹�Ƹ��������
    auto* spotLightPtr = dynamic_cast<SpotLight*>(lightManager.getLight(lightManager.getLightCount() - 1).get());
    if (spotLightPtr) {
        spotLightPtr->setPosition(camera.Position);
        spotLightPtr->setDirection(camera.Front);
    }

    // 4. ���¹�Դ�� UBO ����
    lightManager.updateUBO();

    // 5. ����Դ�� UBO �󶨵���ɫ��
    lightManager.bindUBOToShader(lightingShader, 0);

    // 6. ������ͼ��ͶӰ����
    glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom),
        static_cast<float>(Common::SCR_WIDTH) / static_cast<float>(Common::SCR_HEIGHT),
        0.1f, 100.0f);
    glm::mat4 view = camera.GetViewMatrix();
    lightingShader.use();
    lightingShader.setMat4("projection", projection);
    lightingShader.setMat4("view", view);

    // 7. ���ݵ��Բ���
    lightingShader.setInt("debugView", debugView);
    lightingShader.setInt("debugLightIndex", debugLightIndex);

    // 8. �������й�Դ�Ĺ�ռ����
    std::vector<glm::mat4> lightSpaceMatrices;
    for (size_t i = 0; i < lightManager.getLightCount(); ++i) {
        lightSpaceMatrices.push_back(shadowManager.getLightSpaceMatrix(lights, i));
    }

    // ���ݹ�ռ������ɫ��
    for (size_t i = 0; i < lightManager.getLightCount(); ++i) {
        std::string matrixName = "lightSpaceMatrices[" + std::to_string(i) + "]";
        lightingShader.setMat4(matrixName, lightSpaceMatrices[i]);
    }

    // 9. ��������Ӱ��ͼ
    for (size_t i = 0; i < lightManager.getLightCount(); ++i) {
        glActiveTexture(GL_TEXTURE2 + i); // ��GL_TEXTURE2��ʼ��
        glBindTexture(GL_TEXTURE_2D, shadowManager.getShadowTexture(i));
        std::string shadowMapName = "shadowMaps[" + std::to_string(i) + "]";
        lightingShader.setInt(shadowMapName, 2 + i);
    }

    lightingShader.setInt("lightCount", lightManager.getLightCount());
    lightingShader.setFloat("material.shininess", 32.0f);
    lightingShader.setVec3("viewPos", camera.Position);

    // 10. ������
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, diffuseMap);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, specularMap);

    // 11. ��Ⱦ���� GameObjects
    for (const auto& object : sceneObjects) {
        object->draw(lightingShader.ID); // ���� GameObject::draw() ������ɫ��ID������ģ�;���
    }

    // 12. ��Ⱦ��Դ������
    lightCubeShader.use();
    lightCubeShader.setMat4("projection", projection);
    lightCubeShader.setMat4("view", view);
    glBindVertexArray(lightCubeVAO);
    for (size_t i = 0; i < lightManager.getLightCount(); ++i) {
        Light* light = lightManager.getLight(i).get();
        if (light->getType() == LightType::PointLight) {
            PointLight* pointLight = dynamic_cast<PointLight*>(light);
            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, pointLight->getPosition());
            model = glm::scale(model, glm::vec3(0.2f)); // ����Ϊ��С��������
            lightCubeShader.setMat4("model", model);
            lightCubeShader.setVec3("lightColor", pointLight->getColor()); // ʹ�ù�Դ��ɫ
            glDrawArrays(GL_TRIANGLES, 0, 36);
        }
        // ������Ӷ� DirectionalLight �� SpotLight �Ĵ���
    }
    glBindVertexArray(0);
}

void Render::Cleanup()
{
    glDeleteVertexArrays(1, &planeVAO);
    glDeleteBuffers(1, &planeVBO);
    glDeleteVertexArrays(1, &cubeVAO);
    glDeleteBuffers(1, &cubeVBO);
    glDeleteVertexArrays(1, &lightCubeVAO);
    glDeleteTextures(1, &diffuseTexture);
    glDeleteTextures(1, &specularTexture);
}

