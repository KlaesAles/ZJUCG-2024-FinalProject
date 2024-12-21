// main.cpp
#include "Renderer.h"

#include "Shader.h"
#include "Camera.h"
#include "ShadowManager.h"
#include "LightManager.h"
#include "Light.h"
#include "GameObject.h"
#include "CollisionManager.h"

#include <iostream>
#include <vector>
#include <memory>

int main()
{
    // 设置屏幕尺寸
    unsigned int SCR_WIDTH = 1600;
    unsigned int SCR_HEIGHT = 1200;

    // 初始化 GLFW
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW." << std::endl;
        return -1;
    }

    // 设置 GLFW 配置
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // 创建 GLFW 窗口
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "LearnOpenGL", NULL, NULL);
    if (window == NULL)
    {
        std::cerr << "Failed to create GLFW window." << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);

    // 初始化 GLAD
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cerr << "Failed to initialize GLAD." << std::endl;
        glfwDestroyWindow(window);
        glfwTerminate();
        return -1;
    }

    // 配置全局 OpenGL 状态
    glEnable(GL_DEPTH_TEST);

    // 创建相机
    Camera camera(glm::vec3(0.0f, 0.0f, 3.0f));

    // 创建 LightManager
    LightManager lightManager(16);

    // 创建 ShadowManager
    ShadowManager shadowManager;

    // 创建场景物体容器
    std::vector<GameObject> sceneObjects;
    sceneObjects.emplace_back("./resources/objects/test/test.obj");
    // 根据需要添加更多的游戏对象
    // sceneObjects.emplace_back("./resources/objects/nanosuit/nanosuit.obj");
    // sceneObjects.emplace_back("./resources/objects/Golden tree disc/Golden tree disc.obj");

    // 初始化光源
    auto dirLight = std::make_shared<DirectionalLight>(glm::vec3(10.0f, -4.0f, 1.0f), glm::vec3(1.0f));
    lightManager.addLight(dirLight);

    auto spotLight = std::make_shared<SpotLight>(camera.Position, camera.Front, glm::vec3(1.0f), 1.0f, 12.5f);
    lightManager.addLight(spotLight);

    // 初始化点光源（如果需要）
    /*
    glm::vec3 pointLightPositions[] = {
        glm::vec3(0.0f, 0.0f, 0.0f),
        // 添加更多的位置
    };
    for (const auto& position : pointLightPositions) {
        auto pointLight = std::make_shared<PointLight>(position, glm::vec3(1.0f));
        lightManager.addLight(pointLight);
    }
    */

    // 创建 Renderer，传递已初始化的资源
    Renderer renderer(window, SCR_WIDTH, SCR_HEIGHT, camera, lightManager, shadowManager, sceneObjects);
    if (!renderer.initialize()) {
        std::cerr << "Failed to initialize Renderer." << std::endl;
        glfwDestroyWindow(window);
        glfwTerminate();
        return -1;
    }

    // 定义游戏逻辑
    auto gameLogic = [&sceneObjects]() {
        // 更新物体的包围盒
        for (auto& object : sceneObjects) {
            object.setPosition(object.getPosition()); // 更新位置和包围盒
        }

        // 检测物体之间的碰撞
        CollisionManager::detectCollisions(sceneObjects);
    };

    // 设置游戏逻辑回调
    renderer.setGameLogicCallback(gameLogic);

    // 运行渲染循环
    renderer.run();

    // 清理 GLFW
    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
