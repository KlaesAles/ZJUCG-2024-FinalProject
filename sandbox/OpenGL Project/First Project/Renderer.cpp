// Renderer.cpp
#include "Renderer.h"
#include <iostream>
#include <fstream>

char Renderer::saveFileName[128] = "scene"; // 默认保存文件名
std::vector<std::string> Renderer::availableScenes = {}; // 初始化为空
int Renderer::selectedSceneIndex = 0; // 默认选中的场景索引

// 构造函数
Renderer::Renderer(GLFWwindow* win, unsigned int width, unsigned int height, Camera& cam,
    LightManager& lm, ShadowManager& sm, Scene& sc)
    : window(win), SCR_WIDTH(width), SCR_HEIGHT(height),
    camera(cam), lightManager(lm), shadowManager(sm), scene(sc),
    lightingShader("./shader/Model Shader.vs", "./shader/Model Shader.fs"),
    shadowShader("./Shadow/shadow.vs", "./Shadow/shadow.fs"),
    deltaTime(0.0f), lastFrame(0.0f),
    mouseCaptured(true), lastX(width / 2.0f), lastY(height / 2.0f),
    firstMouse(true),
    debugLightView(false), debugLightIndex(0),
    debugMaterialView(false), debugMaterialIndex(0)
{
}

bool Renderer::initialize()
{
    if (window == nullptr) {
        std::cerr << "Renderer received a null window pointer." << std::endl;
        return false;
    }

    std::cout << "Configuring OpenGL..." << std::endl;
    // 配置OpenGL状态
    configureOpenGL();

    std::cout << "Setting callbacks..." << std::endl;

    // 设置回调函数
    setCallbacks();

    std::cout << "Initializing ImGui..." << std::endl;
    // 初始化 ImGui
    initImGui();

    // 绑定 Renderer 到窗口的用户指针
    glfwSetWindowUserPointer(window, this);

    std::cout << "Renderer initialized successfully." << std::endl;
    return true;
}

void Renderer::configureOpenGL()
{
    glEnable(GL_DEPTH_TEST);
}

void Renderer::initImGui()
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    if (!ImGui_ImplGlfw_InitForOpenGL(window, true)) {
        std::cerr << "Failed to initialize ImGui_ImplGlfw." << std::endl;
        return;
    }
    if (!ImGui_ImplOpenGL3_Init("#version 330")) { // 确保与您的 OpenGL 版本匹配
        std::cerr << "Failed to initialize ImGui_ImplOpenGL3." << std::endl;
        return;
    }
    ImGui::StyleColorsDark();
    std::cout << "ImGui initialized successfully." << std::endl;
}

void Renderer::setCallbacks()
{
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);

    // 设置鼠标捕获
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
}

void Renderer::run()
{
    std::cout << "Starting render loop..." << std::endl;
    while (!glfwWindowShouldClose(window))
    {
        // 计算时间差
        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        std::cout << "Frame time: " << deltaTime << " seconds." << std::endl;

        // 处理输入
        processInput();

        // 更新阴影贴图
        updateShadowMaps();

        // 渲染
        renderFrame();

        // 交换缓冲区和轮询事件
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    std::cout << "Exiting render loop." << std::endl;

    // 清理资源
    cleanup();
}

void Renderer::renderFrame()
{

    // 启动ImGui帧
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    // Sidebar toggle button
    ImGui::SetNextWindowPos(ImVec2(SCR_WIDTH - (sidebar_open ? sidebar_width : 0) - 30, 30), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(30, 30), ImGuiCond_Always);
    ImGui::Begin("ToggleSidebar", NULL, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoDecoration);
    if (ImGui::ArrowButton("##toggle", sidebar_open ? ImGuiDir_Right : ImGuiDir_Left)) {
        sidebar_open = !sidebar_open;
    }
    ImGui::End();

    // Sidebar window
    if (sidebar_open) {
        ImGui::SetNextWindowPos(ImVec2(SCR_WIDTH - sidebar_width, 0), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(sidebar_width, SCR_HEIGHT), ImGuiCond_Always);
        ImGui::Begin("Sidebar", NULL, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);

        // 保存场景
        ImGui::Text("Save Scene");
        ImGui::InputText("File Name", saveFileName, IM_ARRAYSIZE(saveFileName)); // 输入文件名
        if (ImGui::Button("Save")) {
            std::string filePath = "scenes/" + std::string(saveFileName) + ".json"; // 保存到 scenes 目录
            saveScene(filePath);
            availableScenes = getSceneFiles("scenes"); // 更新可用场景列表
        }
        ImGui::Separator();

        // 加载场景
        ImGui::Text("Load Scene");
        if (availableScenes.empty()) {
            availableScenes = getSceneFiles("scenes"); // 初始化可用场景列表
        }
        if (!availableScenes.empty()) {
            // 显示场景文件的下拉框
            if (ImGui::BeginCombo("Available Scenes", availableScenes[selectedSceneIndex].c_str())) {
                for (int i = 0; i < availableScenes.size(); ++i) {
                    bool isSelected = (selectedSceneIndex == i);
                    if (ImGui::Selectable(availableScenes[i].c_str(), isSelected)) {
                        selectedSceneIndex = i; // 更新选择的场景索引
                    }
                    if (isSelected) {
                        ImGui::SetItemDefaultFocus();
                    }
                }
                ImGui::EndCombo();
            }

            // 加载选中的场景
            if (ImGui::Button("Load")) {
                std::string filePath = "scenes/" + availableScenes[selectedSceneIndex];
                loadScene(filePath);
            }
        }
        else {
            ImGui::Text("No scenes available to load.");
        }
        ImGui::Separator(); // 分隔符

        ImGui::Text("Light Controls");
        if (lightManager.getLightCount() > 0) {
            auto spotLightPtr = std::dynamic_pointer_cast<SpotLight>(lightManager.getLight(lightManager.getLightCount() - 1));
            if (spotLightPtr) {
                // 强度控制
                float intensity = spotLightPtr->getIntensity();
                if (ImGui::SliderFloat("Spotlight Intensity", &intensity, 0.0f, 5.0f)) {
                    spotLightPtr->setIntensity(intensity);
                }

                // 切光角度控制
                float cutoffAngle = spotLightPtr->getCutoffAngle();
                if (ImGui::SliderFloat("Spotlight Cutoff Angle", &cutoffAngle, 1.0f, 45.0f)) {
                    spotLightPtr->setCutoffAngle(cutoffAngle);
                }

                // 颜色控制
                glm::vec3 color = spotLightPtr->getColor();
                float colorArray[3] = { color.r, color.g, color.b };
                if (ImGui::ColorEdit3("Spotlight Color", colorArray)) {
                    spotLightPtr->setColor(glm::vec3(colorArray[0], colorArray[1], colorArray[2]));
                }
            }
        }

        ImGui::Separator(); // 分隔符

        ImGui::Checkbox("Debug Light View", &debugLightView);
        if (debugLightView) {
            debugMaterialView = false;
            ImGui::SliderInt("Debug Light Index", &debugLightIndex, 0, static_cast<int>(lightManager.getLightCount()) - 1, "%d");
        }

        ImGui::Checkbox("Debug Material View", &debugMaterialView);
        if (debugMaterialView) {
            debugLightView = false;
            ImGui::SliderInt("Debug Material Index", &debugMaterialIndex, 0, 10, "%d");
        }

        ImGui::Separator(); // 分隔符

        // 帮助信息
        if (!mouseCaptured) {
            ImGui::Text("Controls Help");
            ImGui::Text("Press TAB to toggle mouse capture");
            ImGui::Text("Current Status: GUI Control Mode");
        }

        ImGui::End();
    }

    std::cout << "Rendering frame..." << std::endl;

    // 清除缓冲区
    glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // 更新聚光灯的位置和方向
    if (lightManager.getLightCount() > 0) {
        auto spotLightPtr = std::dynamic_pointer_cast<SpotLight>(lightManager.getLight(lightManager.getLightCount() - 1));
        if (spotLightPtr) {
            spotLightPtr->setPosition(camera.Position);
            spotLightPtr->setDirection(camera.Front);
            std::cout << "SpotLight position and direction updated." << std::endl;
        }
    }

    // 更新光照UBO
    lightManager.updateUBO();

    // 绑定UBO到着色器
    lightingShader.use();
    lightManager.bindUBOToShader(lightingShader, 0); // 假设 binding point 为 0

    // 视图/投影矩阵
    glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom),
        static_cast<float>(SCR_WIDTH) / static_cast<float>(SCR_HEIGHT), 0.1f, 100.0f);
    glm::mat4 view = camera.GetViewMatrix();

    // 设置着色器统一变量
    lightingShader.setInt("debugLightView", debugLightView);
    lightingShader.setInt("debugLightIndex", debugLightIndex);
    lightingShader.setInt("debugMaterialView", debugMaterialView);
    lightingShader.setInt("debugMaterialIndex", debugMaterialIndex);

    lightingShader.setMat4("projection", projection);
    lightingShader.setMat4("view", view);
    lightingShader.setVec3("viewPos", camera.Position);
    lightingShader.setFloat("material.shininess", 32.0f);

    // 设置所有光源的光空间矩阵
    std::vector<glm::mat4> lightSpaceMatrices;
	std::vector<Light*> lights;
    for (size_t i = 0; i < lightManager.getLightCount(); ++i) {
        lights.push_back(lightManager.getLight(i).get());
        lightSpaceMatrices.push_back(shadowManager.getLightSpaceMatrix(lights, i));
    }

    // 传递光空间矩阵到着色器
    for (size_t i = 0; i < lightSpaceMatrices.size(); ++i) {
        std::string matrixName = "lightSpaceMatrices[" + std::to_string(i) + "]";
        lightingShader.setMat4(matrixName, lightSpaceMatrices[i]);
    }

    // 绑定所有阴影贴图
    for (size_t i = 0; i < lightManager.getLightCount(); ++i) {
        glActiveTexture(GL_TEXTURE2 + static_cast<GLenum>(i)); // 从GL_TEXTURE2开始
        glBindTexture(GL_TEXTURE_2D, shadowManager.getShadowTexture(i));
        std::string shadowMapName = "shadowMaps[" + std::to_string(i) + "]";
        lightingShader.setInt(shadowMapName, 2 + static_cast<int>(i));
    }

    lightingShader.setInt("lightCount", static_cast<int>(lightManager.getLightCount()));

    // 渲染所有游戏对象
    scene.draw(lightingShader);

    std::cout << "Rendering ImGui..." << std::endl;

    // 渲染ImGui
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    std::cout << "Frame rendered." << std::endl;
}

void Renderer::processInput()
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
        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
            camera.ProcessKeyboard(FORWARD, deltaTime);
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
            camera.ProcessKeyboard(BACKWARD, deltaTime);
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
            camera.ProcessKeyboard(LEFT, deltaTime);
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
            camera.ProcessKeyboard(RIGHT, deltaTime);
    }

    // 如果有游戏逻辑回调，执行它
    if (gameLogicCallback) {
        gameLogicCallback();
    }
}

void Renderer::updateShadowMaps()
{
    // 动态生成阴影贴图
    std::vector<Light*> lights;
    const auto& gameObjects = scene.getGameObjects(); // Scene 提供对象列表

    // 收集光源指针
    for (size_t i = 0; i < lightManager.getLightCount(); ++i) {
        lights.push_back(lightManager.getLight(i).get());
    }

    // 设置阴影贴图分辨率（可根据需要调整）
    shadowManager.updateShadowResolution(4096);

    // 生成阴影贴图
    shadowManager.generateShadowMaps(lights, scene, shadowShader.ID);
}

void Renderer::saveScene(const std::string& filePath) {
    try {
        nlohmann::json sceneJson = scene.serialize();
        std::ofstream file(filePath);
        if (file.is_open()) {
            file << sceneJson.dump(4); // 格式化输出
            file.close();
            std::cout << "Scene saved to " << filePath << std::endl;
        }
        else {
            std::cerr << "Failed to open file for saving: " << filePath << std::endl;
        }
    }
    catch (const std::exception& e) {
        std::cerr << "Error saving scene: " << e.what() << std::endl;
    }
}

void Renderer::loadScene(const std::string& filePath) {
    try {
        std::ifstream file(filePath);
        if (file.is_open()) {
            nlohmann::json sceneJson;
            file >> sceneJson;
            file.close();
            scene.deserialize(sceneJson);
            std::cout << "Scene loaded from " << filePath << std::endl;
        }
        else {
            std::cerr << "Failed to open file for loading: " << filePath << std::endl;
        }
    }
    catch (const std::exception& e) {
        std::cerr << "Error loading scene: " << e.what() << std::endl;
    }
}

std::vector<std::string> Renderer::getSceneFiles(const std::string& directory) {
    std::vector<std::string> sceneFiles;
    for (const auto& entry : std::filesystem::directory_iterator(directory)) {
        if (entry.is_regular_file() && entry.path().extension() == ".json") {
            sceneFiles.push_back(entry.path().filename().string());
        }
    }
    return sceneFiles;
}

void Renderer::cleanup()
{
    std::cout << "Cleaning up ImGui..." << std::endl;
    // 清理ImGui
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    std::cout << "ImGui cleaned up successfully." << std::endl;
}

// 静态回调函数 - 窗口大小变化
void Renderer::framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    // 获取 Renderer 实例
    Renderer* renderer = static_cast<Renderer*>(glfwGetWindowUserPointer(window));
    if (renderer) {
        renderer->SCR_WIDTH = width;
        renderer->SCR_HEIGHT = height;
        glViewport(0, 0, width, height);

        // 更新阴影贴图分辨率
        int newResolution = std::max(width, height);
        renderer->shadowManager.updateShadowResolution(newResolution);
    }
}

// 静态回调函数 - 鼠标移动
void Renderer::mouse_callback(GLFWwindow* window, double xposIn, double yposIn)
{
    Renderer* renderer = static_cast<Renderer*>(glfwGetWindowUserPointer(window));

    if (renderer && renderer->mouseCaptured)
    {
        float xpos = static_cast<float>(xposIn);
        float ypos = static_cast<float>(yposIn);

        if (renderer->firstMouse)
        {
            renderer->lastX = xpos;
            renderer->lastY = ypos;
            renderer->firstMouse = false;
        }

        float xoffset = xpos - renderer->lastX;
        float yoffset = renderer->lastY - ypos; // y坐标是反向的

        renderer->lastX = xpos;
        renderer->lastY = ypos;

        renderer->camera.ProcessMouseMovement(xoffset, yoffset);
    }
}

// 静态回调函数 - 鼠标滚轮
void Renderer::scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    Renderer* renderer = static_cast<Renderer*>(glfwGetWindowUserPointer(window));
    if (renderer && renderer->mouseCaptured)
    {
        renderer->camera.ProcessMouseScroll(static_cast<float>(yoffset));
    }
}
