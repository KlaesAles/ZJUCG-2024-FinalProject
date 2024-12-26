#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <shader.h>
#include <camera.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include <ShadowManager.h>
#include <Light.h>
#include <LightManager.h>

#include <iostream>
#include <vector>

#include <GameObject.h>
#include <CollisionManager.h>

// Function declarations
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow* window);
void drawBoundingBox(const BoundingBox& box, const glm::mat4& view, const glm::mat4& projection);

bool mouseCaptured = true;  // 初始状态为捕获鼠标

// Settings
unsigned int SCR_WIDTH = 1600;
unsigned int SCR_HEIGHT = 1200;

// Camera
Camera camera(glm::vec3(0.0f, 0.0f, 3.0f));
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

// Timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;

// positions of the point lights
glm::vec3 pointLightPositions[] = {
    glm::vec3((0.0f, 0.0f, 0.0f)),
};

// Main
int main()
{
    // glfw: initialize and configure
    // ------------------------------
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // glfw window creation
    // --------------------
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "LearnOpenGL", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);

    // tell GLFW to capture our mouse
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // glad: load all OpenGL function pointers
    // ---------------------------------------
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    // configure global opengl state
    // -----------------------------
    glEnable(GL_DEPTH_TEST);

    // ImGui setup
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init();

    // Load shaders
    Shader lightingShader("./shader/Model Shader.vs", "./shader/Model Shader.fs");
    Shader ShadowShader("./Shadow/shadow.vs", "./Shadow/shadow.fs");

    // Bind GameObjects
    // ---------------
    // 创建场景物体容器
    std::vector<GameObject> sceneObjects;

    sceneObjects.emplace_back("./resources/objects/test/test.obj");
    //sceneObjects.emplace_back("./resources/objects/nanosuit/nanosuit.obj");
    //sceneObjects.emplace_back("./resources/objects/Golden tree disc/Golden tree disc.obj");

    // 光源管理器
    LightManager lightManager(16); // 最多支持16个光源

    // 初始化点光源
    /*
    for (const auto& position : pointLightPositions) {
        auto pointLight = std::make_shared<PointLight>(position, glm::vec3(1.0f));
        lightManager.addLight(pointLight);
    }
    */

    // 初始化定向光
    auto dirLight = std::make_shared<DirectionalLight>(glm::vec3(10.0f, -4.0f, 1.0f), glm::vec3(1.0f));
    lightManager.addLight(dirLight);

    // 初始化聚光灯
    auto spotLight = std::make_shared<SpotLight>(camera.Position, camera.Front, glm::vec3(1.0f), 1.0f, 12.5f);
    lightManager.addLight(spotLight);

    // 创建阴影管理器
    ShadowManager shadowManager;

    // 将 shadowManager 传递给窗口
    glfwSetWindowUserPointer(window, &shadowManager);

    bool debugLightView = 0;
    int debugLightIndex = 0;

    bool debugMaterialView = 0;
    int debugMaterialIndex = 0;

    Renderer renderer(SCR_WIDTH, SCR_HEIGHT, camera, lightManager, shadowManager);
    renderer.initialize();

    // 渲染循环
    while (!glfwWindowShouldClose(window))
    {
        // per-frame time logic
        // --------------------
        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // input
        // -----
        processInput(window);

        // 获取聚光灯
        auto* spotLightPtr = dynamic_cast<SpotLight*>(lightManager.getLight(lightManager.getLightCount() - 1).get());

        // Start ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        if (!mouseCaptured) {
            ImGui::Begin("Controls Help");
            ImGui::Text("Press TAB to toggle mouse capture");
            ImGui::Text("Current Status: GUI Control Mode");
            ImGui::End();
        }

        // 添加ImGui控制（如果需要）：
        ImGui::Begin("Light Controls");
        if (spotLightPtr) {
            // 强度控制
            float intensity = spotLightPtr->getIntensity();
            if (ImGui::SliderFloat("Spotlight Intensity", &intensity, 0.0f, 5.0f)) {
                spotLightPtr->setIntensity(intensity);
            }

            // 切光角度控制
            float cutoffAngle = spotLightPtr->getCutoffAngle();
            if (ImGui::SliderFloat("Spotlight Cutoff Angle", &cutoffAngle, 1.0f, 45.0f)) {
                // 需要在 SpotLight 类中添加 setCutoffAngle 方法
                spotLightPtr->setCutoffAngle(cutoffAngle);
            }

            // 颜色控制
            glm::vec3 color = spotLightPtr->getColor();
            float colorArray[3] = { color.r, color.g, color.b };
            if (ImGui::ColorEdit3("Spotlight Color", colorArray)) {
                spotLightPtr->setColor(glm::vec3(colorArray[0], colorArray[1], colorArray[2]));
            }
        }

        ImGui::Checkbox("Debug Light View", &debugLightView);
        if (debugLightView) {
			debugMaterialView = false;
            ImGui::SliderInt("debug Light Index", &debugLightIndex, 0, lightManager.getLightCount() - 1, "%d");
        }

        ImGui::Checkbox("Debug Material View", &debugMaterialView);
        if (debugMaterialView) {
			debugLightView = false;
            ImGui::SliderInt("debug Material Index", &debugMaterialIndex, 0, 47, "%d");
        }

        ImGui::End();

        // 更新物体的包围盒
        for (auto& object : sceneObjects) {
            object.setPosition(object.getPosition()); // 更新位置和包围盒
        }

        // 检测物体之间的碰撞
        CollisionManager::detectCollisions(sceneObjects);

        // 渲染阴影
        // 1. 首先渲染深度贴图
        std::vector<Light*> lights;
        std::vector<GameObject*> objects;

        // 收集场景对象指针
        for (auto& obj : sceneObjects) {
            objects.push_back(&obj);
        }

        // 收集光源指针
        for (size_t i = 0; i < lightManager.getLightCount(); ++i) {
            lights.push_back(lightManager.getLight(i).get());
        }

        // 设置阴影贴图分辨率
        shadowManager.updateShadowResolution(4096);

        // 动态生成阴影贴图
        shadowManager.generateShadowMaps(lights, objects, ShadowShader.ID);

        // render
        // ------
        glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        if (spotLightPtr) {
            // 更新聚光灯位置和方向
            spotLightPtr->setPosition(camera.Position);
            spotLightPtr->setDirection(camera.Front);
        }

        // 更新UBO数据
        lightManager.updateUBO();

        // 将UBO绑定到着色器
        lightManager.bindUBOToShader(lightingShader, 0);

        // view/projection transformations
        glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
        glm::mat4 view = camera.GetViewMatrix();
        lightingShader.use();

        // 传递Debug参数
        lightingShader.setInt("debugLightView", debugLightView);
        lightingShader.setInt("debugLightIndex", debugLightIndex);
        lightingShader.setInt("debugMaterialView", debugMaterialView);
        lightingShader.setInt("debugMaterialIndex", debugMaterialIndex);

        // 设置所有光源的光空间矩阵
        std::vector<glm::mat4> lightSpaceMatrices;
        for (size_t i = 0; i < lightManager.getLightCount(); ++i) {
            lightSpaceMatrices.push_back(shadowManager.getLightSpaceMatrix(lights, i));
        }

        // 传递光空间矩阵到着色器
        for (size_t i = 0; i < lightManager.getLightCount(); ++i) {
            std::string matrixName = "lightSpaceMatrices[" + std::to_string(i) + "]";
            lightingShader.setMat4(matrixName, lightSpaceMatrices[i]);
        }

        // 绑定所有阴影贴图
        for (size_t i = 0; i < lightManager.getLightCount(); ++i) {
            glActiveTexture(GL_TEXTURE2 + i); // 从GL_TEXTURE2开始
            glBindTexture(GL_TEXTURE_2D, shadowManager.getShadowTexture(i));
            std::string shadowMapName = "shadowMaps[" + std::to_string(i) + "]";
            lightingShader.setInt(shadowMapName, 2 + i);
        }

        lightingShader.setInt("lightCount", lightManager.getLightCount());
        lightingShader.setFloat("material.shininess", 32.0f);
        lightingShader.setVec3("viewPos", camera.Position);
        lightingShader.setMat4("projection", projection);
        lightingShader.setMat4("view", view);

        // Render all GameObjects
        for (auto& object : sceneObjects) {
            object.draw(lightingShader.ID); // 使用 GameObject 的 draw 方法
        }

        // Render ImGui
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
        // -------------------------------------------------------------------------------
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwTerminate();
    return 0;

}

// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
// ---------------------------------------------------------------------------------------------------------
void processInput(GLFWwindow* window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    // 使用 Tab 键切换鼠标捕获状态
    static bool tabPressed = false;
    if (glfwGetKey(window, GLFW_KEY_TAB) == GLFW_PRESS) {
        if (!tabPressed) {  // 确保只在按下的瞬间切换一次
            tabPressed = true;
            mouseCaptured = !mouseCaptured;
            if (mouseCaptured) {
                glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
                firstMouse = true;  // 重置鼠标状态，防止视角跳转
            }
            else {
                glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
            }
        }
    }
    else {
        tabPressed = false;
    }

    // 只在鼠标被捕获时处理相机移动
    if (mouseCaptured) {
        float cameraSpeed = 2.5f * deltaTime;
        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
            camera.ProcessKeyboard(FORWARD, deltaTime);
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
            camera.ProcessKeyboard(BACKWARD, deltaTime);
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
            camera.ProcessKeyboard(LEFT, deltaTime);
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
            camera.ProcessKeyboard(RIGHT, deltaTime);
    }
}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    // 更新全局窗口大小
    SCR_WIDTH = width;
    SCR_HEIGHT = height;

    // 更新视口
    glViewport(0, 0, width, height);

    // 获取存储在窗口中的 ShadowManager 指针
    ShadowManager* shadowManager = static_cast<ShadowManager*>(glfwGetWindowUserPointer(window));
    if (shadowManager) {
        // 更新阴影贴图分辨率
        int newResolution = std::max(width, height);
        shadowManager->updateShadowResolution(newResolution);
    }
}
// glfw: whenever the mouse moves, this callback is called
// -------------------------------------------------------
void mouse_callback(GLFWwindow* window, double xposIn, double yposIn)
{
    // 只在鼠标被捕获时处理视角移动
    if (!mouseCaptured)
        return;

    float xpos = static_cast<float>(xposIn);
    float ypos = static_cast<float>(yposIn);

    if (firstMouse)
    {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos;

    lastX = xpos;
    lastY = ypos;

    camera.ProcessMouseMovement(xoffset, yoffset);
}

// glfw: whenever the mouse scroll wheel scrolls, this callback is called
// ----------------------------------------------------------------------
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    // 只在鼠标被捕获时处理滚轮缩放
    if (!mouseCaptured)
        return;

    camera.ProcessMouseScroll(static_cast<float>(yoffset));
}