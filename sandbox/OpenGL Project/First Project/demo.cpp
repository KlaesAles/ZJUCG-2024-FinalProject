// main.cpp
#include "Renderer.h"

#include "Shader.h"
#include "Camera.h"
#include "ShadowManager.h"
#include "LightManager.h"
#include "Light.h"
#include "GameObject.h"
#include "CollisionManager.h"
#include "Scene.h"

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

    // 创建 ShadowManager
    ShadowManager shadowManager;

    // 创建 LightManager
    LightManager lightManager(16);

    // 创建场景
    Scene scene(lightManager);

    // 添加场景物体
    //scene.addGameObject(GameObject("./resources/objects/Cube/Cube.obj"));
    //scene.addGameObject(GameObject("./resources/objects/Cylinder/Cylinder.obj"));
    //scene.addGameObject(GameObject("./resources/objects/Cone/Cone.obj"));
    //scene.addGameObject(GameObject("./resources/objects/Sphere/Sphere.obj"));
    //scene.addGameObject(GameObject("./resources/objects/nanosuit/nanosuit.obj"));
    //scene.addGameObject(GameObject("./resources/objects/Plane/Plane.obj", glm::vec3(0.0f, 0.0f, 0.0f)));
    //scene.addGameObject(GameObject("./resources/objects/Mini Tokyo/Mini Tokyo.obj", glm::vec3(0.0f, -4.0f, -8.0f), glm::vec3(0.1f)));

    // 创建 character 并使用 shared_ptr
    auto character = std::make_shared<GameObject>(
        "character",
        "./resources/objects/character/robot.fbx",
        glm::vec3(0.0f),
        glm::vec3(0.05f)
    );

    // 输出所有可用的动画名称以供参考
    for (const auto& anim : character->getModel().animations) {
        std::cout << "Available animation: " << anim.getName() << std::endl;
    }
    scene.addGameObject(character);

    scene.addGameObject(std::make_shared<GameObject>("Plane", "./resources/objects/Plane/Plane.obj", glm::vec3(0.0f, 0.0f, 0.0f)));
    
    // 添加基础几何体
    const std::vector<std::string> objectPaths = {
        "./resources/objects/Cube/Cube.obj",
        "./resources/objects/Cylinder/Cylinder.obj",
        "./resources/objects/Cone/Cone.obj",
        "./resources/objects/Sphere/Sphere.obj"
    };

    for (size_t i = 0; i < objectPaths.size(); ++i) {
        std::string objectName = "Object_" + std::to_string(i);
        glm::vec3 position = glm::vec3(-7.5f + i * 5.0f, 0.0f, 0.0f); // 一字排开
        glm::vec3 scale = glm::vec3(1.5f); // 统一缩放大小
        auto object = std::make_shared<GameObject>(objectName, objectPaths[i], position, scale);
        scene.addGameObject(object);
    }

    // 添加点光源（放置在几何体正上方）
    for (size_t i = 0; i < objectPaths.size(); ++i) {
        glm::vec3 position = glm::vec3(-7.5f + i * 5.0f, 5.0f, 0.0f); // 点光源位置：几何体上方
        glm::vec3 color = glm::vec3((i % 3) == 0 ? 1.0f : 0.0f, (i % 3) == 1 ? 1.0f : 0.0f, (i % 3) == 2 ? 1.0f : 0.0f); // RGB 循环颜色
        auto pointLight = std::make_shared<PointLight>(position, color);
        lightManager.addLight(pointLight);
    }

    /*
    // 添加聚光灯
    auto spotLight1 = std::make_shared<SpotLight>(
        glm::vec3(0.0f, 10.0f, 10.0f),  // 聚光灯位置
        glm::normalize(glm::vec3(0.0f, -1.0f, -1.0f)), // 聚光灯方向
        glm::vec3(1.0f, 1.0f, 1.0f),   // 白色光
        0.8f, 45.0f                    // 衰减参数
    );
    lightManager.addLight(spotLight1);

    auto spotLight2 = std::make_shared<SpotLight>(
        glm::vec3(-10.0f, 10.0f, 5.0f),
        glm::normalize(glm::vec3(1.0f, -1.0f, -1.0f)),
        glm::vec3(1.0f, 1.0f, 0.0f),   // 黄色光
        0.8f, 45.0f
    );
    lightManager.addLight(spotLight2);

    auto spotLight3 = std::make_shared<SpotLight>(
        glm::vec3(10.0f, 10.0f, 5.0f),
        glm::normalize(glm::vec3(-1.0f, -1.0f, -1.0f)),
        glm::vec3(0.0f, 1.0f, 1.0f),   // 青色光
        0.8f, 45.0f
    );
    lightManager.addLight(spotLight3); 
    */

    // 添加定向光
    auto dirLight = std::make_shared<DirectionalLight>(
        glm::vec3(-2.0f, -4.0f, -2.0f), // 定向光方向
        glm::vec3(1.0f, 1.0f, 1.0f)    // 白色光
    );
    lightManager.addLight(dirLight);


    /*
    scene.addGameObject(GameObject("./resources/objects/Mini Tokyo/Mini Tokyo.obj", glm::vec3(-27.0f, -3.0f, 0.0f), glm::vec3(0.1f)));
    scene.addGameObject(GameObject("./resources/objects/nanosuit/nanosuit.obj", glm::vec3(-15.0f, -5.0f, 0.0f), glm::vec3(0.4f), glm::vec3(0.0f,90.0f, 0)));
    */

    // 初始化光源
    auto spotLight = std::make_shared<SpotLight>(camera.Position, camera.Front, glm::vec3(1.0f), 0.0f, 0.0f);
    lightManager.addLight(spotLight);

    /*
    auto dirLight = std::make_shared<DirectionalLight>(glm::vec3(10.0f, -4.0f, 1.0f), glm::vec3(1.0f));
    lightManager.addLight(dirLight);

    // 初始化点光源（如果需要）
    glm::vec3 pointLightPositions[] = {
        glm::vec3(5.0f, 5.0f, 0.0f),
        // 添加更多的位置
    };
    for (const auto& position : pointLightPositions) {
        auto pointLight = std::make_shared<PointLight>(position, glm::vec3(1.0f, 0.5f, 0.5f));
        lightManager.addLight(pointLight);
    }
    */


    // 创建 Renderer，传递已初始化的资源
    Renderer renderer(window, SCR_WIDTH, SCR_HEIGHT, camera, lightManager, shadowManager, scene);
    renderer.setCharacter(character);
    if (!renderer.initialize()) {
        std::cerr << "Failed to initialize Renderer." << std::endl;
        glfwDestroyWindow(window);
        glfwTerminate();
        return -1;
    }

    // 定义游戏逻辑
    auto gameLogic = [&scene]() {
        // 更新物体的包围盒
        for (auto& object : scene.getGameObjects()) {
            object->setPosition(object->getPosition()); // 更新位置和包围盒
        }

        // 检测物体之间的碰撞
        //CollisionManager::detectCollisions(scene.getGameObjects());
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
