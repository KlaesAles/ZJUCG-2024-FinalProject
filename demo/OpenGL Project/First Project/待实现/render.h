#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <vector>
#include <string>
#include <memory>
#include <stb_image.h>       // 用于加载纹理
#include <iostream>

#include "Shader.h"          // 假设有一个 Shader 类封装着色器操作
#include "LightManager.h"    // 光源管理器
#include "ShadowManager.h"   // 阴影管理器
#include "GameObject.h"      // 游戏对象
#include "Camera.h"          // 相机
#include "Scene.h"

class Scene;

class Render {
public:
    // 构造函数，接受光源管理器、阴影管理器和场景的引用
    Render(LightManager& lightMgr, ShadowManager& shadowMgr, Scene& scene);

    // 初始化渲染资源
    bool Initialize();

    // 渲染一帧
    void RenderFrame(const Camera& camera, bool debugView, int debugLightIndex, unsigned int diffuseMap, unsigned int specularMap);

    // 清理资源
    void Cleanup();

private:
    // 光源和阴影管理器引用
    LightManager& lightManager;
    ShadowManager& shadowManager;

    // 场景引用
    Scene& scene;

    // 着色器
    Shader lightingShader;
    Shader lightCubeShader;
    Shader shadowShader;

    // VAOs 和 VBOs
    unsigned int planeVAO, planeVBO;
    unsigned int cubeVAO, cubeVBO;
    unsigned int lightCubeVAO;

    // 纹理
    unsigned int diffuseTexture;
    unsigned int specularTexture;

    // 私有方法
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

    // 加载纹理
    diffuseTexture = loadTexture("./assets/container2.png");
    specularTexture = loadTexture("./assets/container2_specular.png");
    if (diffuseTexture == 0 || specularTexture == 0) {
        std::cerr << "Failed to load textures." << std::endl;
        return false;
    }

    // 配置着色器
    lightingShader.use();
    lightingShader.setInt("material.diffuse", 0);
    lightingShader.setInt("material.specular", 1);

    return true;
}

bool Render::setupShaders()
{
    // 着色器已经在构造函数中加载，无需额外操作
    // 这里可以添加其他着色器配置，如 UBO、SSBO 等
    return true;
}

bool Render::setupPlane()
{
    // 获取地面的 VAO 和 VBO 通过场景对象
    // 假设 Scene 中已经创建了地面的 GameObject 并设置了 VAO/VBO
    // 如果需要单独管理地面的 VAO/VBO，可以在 Scene 中提供接口获取这些数据
    // 这里假设 Render 负责创建地面的 VAO/VBO，如前述例子

    // 示例：如果 Scene 不负责创建地面 VAO/VBO，则 Render 需要独立创建
    // 参考之前的 setupPlane 方法

    // 这里假设 Render 独立创建地面
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
    // 位置属性
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    // 法向量属性
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    // 纹理坐标属性
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

        // ... 其他面顶点数据（省略） ...
        // 总共36个顶点
    };

    glGenVertexArrays(1, &cubeVAO);
    glGenBuffers(1, &cubeVBO);

    glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cubeVertices), cubeVertices, GL_STATIC_DRAW);

    glBindVertexArray(cubeVAO);
    // 位置属性
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    // 法向量属性
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    // 纹理坐标属性
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
    // 只需要位置属性
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
    stbi_set_flip_vertically_on_load(true); // 根据需要是否翻转图片
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

        // 设置纹理参数
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
    // 1. 渲染阴影贴图
    std::vector<Light*> lights;
    std::vector<GameObject*> objects;

    // 收集场景对象指针
    const auto& sceneObjects = scene.GetGameObjects();
    for (const auto& obj : sceneObjects) {
        objects.push_back(obj.get());
    }

    // 收集光源指针
    for (size_t i = 0; i < lightManager.getLightCount(); ++i) {
        lights.push_back(lightManager.getLight(i).get());
    }

    // 设置阴影贴图分辨率（可以根据需要调整）
    shadowManager.updateShadowResolution(4096);

    // 动态生成阴影贴图
    shadowManager.generateShadowMaps(lights, objects, shadowShader.ID);

    // 2. 清除屏幕
    glViewport(0, 0, Common::SCR_WIDTH, Common::SCR_HEIGHT);
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // 3. 更新光源的位置和方向（例如，聚光灯跟随相机）
    auto* spotLightPtr = dynamic_cast<SpotLight*>(lightManager.getLight(lightManager.getLightCount() - 1).get());
    if (spotLightPtr) {
        spotLightPtr->setPosition(camera.Position);
        spotLightPtr->setDirection(camera.Front);
    }

    // 4. 更新光源的 UBO 数据
    lightManager.updateUBO();

    // 5. 将光源的 UBO 绑定到着色器
    lightManager.bindUBOToShader(lightingShader, 0);

    // 6. 设置视图和投影矩阵
    glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom),
        static_cast<float>(Common::SCR_WIDTH) / static_cast<float>(Common::SCR_HEIGHT),
        0.1f, 100.0f);
    glm::mat4 view = camera.GetViewMatrix();
    lightingShader.use();
    lightingShader.setMat4("projection", projection);
    lightingShader.setMat4("view", view);

    // 7. 传递调试参数
    lightingShader.setInt("debugView", debugView);
    lightingShader.setInt("debugLightIndex", debugLightIndex);

    // 8. 设置所有光源的光空间矩阵
    std::vector<glm::mat4> lightSpaceMatrices;
    for (size_t i = 0; i < lightManager.getLightCount(); ++i) {
        lightSpaceMatrices.push_back(shadowManager.getLightSpaceMatrix(lights, i));
    }

    // 传递光空间矩阵到着色器
    for (size_t i = 0; i < lightManager.getLightCount(); ++i) {
        std::string matrixName = "lightSpaceMatrices[" + std::to_string(i) + "]";
        lightingShader.setMat4(matrixName, lightSpaceMatrices[i]);
    }

    // 9. 绑定所有阴影贴图
    for (size_t i = 0; i < lightManager.getLightCount(); ++i) {
        glActiveTexture(GL_TEXTURE2 + i); // 从GL_TEXTURE2开始绑定
        glBindTexture(GL_TEXTURE_2D, shadowManager.getShadowTexture(i));
        std::string shadowMapName = "shadowMaps[" + std::to_string(i) + "]";
        lightingShader.setInt(shadowMapName, 2 + i);
    }

    lightingShader.setInt("lightCount", lightManager.getLightCount());
    lightingShader.setFloat("material.shininess", 32.0f);
    lightingShader.setVec3("viewPos", camera.Position);

    // 10. 绑定纹理
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, diffuseMap);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, specularMap);

    // 11. 渲染所有 GameObjects
    for (const auto& object : sceneObjects) {
        object->draw(lightingShader.ID); // 假设 GameObject::draw() 接受着色器ID并设置模型矩阵
    }

    // 12. 渲染光源立方体
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
            model = glm::scale(model, glm::vec3(0.2f)); // 缩放为较小的立方体
            lightCubeShader.setMat4("model", model);
            lightCubeShader.setVec3("lightColor", pointLight->getColor()); // 使用光源颜色
            glDrawArrays(GL_TRIANGLES, 0, 36);
        }
        // 可以添加对 DirectionalLight 和 SpotLight 的处理
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

