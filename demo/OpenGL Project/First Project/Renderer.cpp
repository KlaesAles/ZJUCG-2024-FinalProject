#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#define RPC_NO_WINDOWS_H
#include <SDKDDKVer.h>
#include <Windows.h>
#include <commdlg.h>

// 确保在包含其他头文件之前取消这些宏定义
#undef byte
#undef FAR
#undef near

// Renderer.cpp
#include "Renderer.h"
#include <iostream>
#include <fstream>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/matrix_decompose.hpp>

char Renderer::saveFileName[128] = "scene"; // 默认保存文件名
std::vector<std::string> Renderer::availableScenes = {}; // 初始化为空
int Renderer::selectedSceneIndex = 0; // 默认选中的场景索引
float Resolution = 5000.0f;
float far_plane = 100.0f;

// 构造函数
Renderer::Renderer(GLFWwindow* win, unsigned int width, unsigned int height, Camera& cam,
    LightManager& lm, ShadowManager& sm, Scene& sc)
    : window(win), SCR_WIDTH(width), SCR_HEIGHT(height),
    camera(cam), lightManager(lm), shadowManager(sm), scene(sc),
    lightingShader("./shaders/Model Shader.vs", "./shaders/Model Shader.fs"),
    shadowShader("./shadow/shadow.vs", "./shadow/shadow.fs"),
    pointshadowShader("./shadow/shadow_point.vs", "./shadow/shadow_point.fs", "./shadow/shadow_point.gs"),
    deltaTime(0.0f), lastFrame(0.0f),
    mouseCaptured(true), lastX(width / 2.0f), lastY(height / 2.0f),
    firstMouse(true), mouseLeftButtonDown(false), selectedObject(nullptr), dragOffset(0.0f),
    debugLightView(false), debugLightIndex(0),
    debugMaterialView(false), debugMaterialIndex(0),
    postProcessing(width, height),
    showBoundingSpheres(false), // 新增：包围球显示标志
    enableSkybox(true) // 默认启用天空盒
{
    // 初始化天空盒
    loadSkybox(getDefaultSkyboxPaths());
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

    // 初始化后处理
    if (!postProcessing.initialize()) {
        std::cerr << "Failed to initialize post-processing!" << std::endl;
        return false;
    }

    // 初始化天空盒着色器
    skyboxShader = std::make_unique<Shader>("./shaders/skybox.vs", "./shaders/skybox.fs");
    if (!skyboxShader) {
        std::cerr << "Failed to create skybox shader!" << std::endl;
        return false;
    }

    // 1) 加载Shader
    Shader GrayShader("./effects/post_process.vs", "./effects/post_process_Grayscale.fs");
    Shader InvertShader("./effects/post_process.vs", "./effects/post_process_Invert.fs");
    Shader SepiaShader("./effects/post_process.vs", "./effects/post_process_Sepia.fs");
    Shader VignetteShader("./effects/post_process.vs", "./effects/post_process_Vignette.fs");
    Shader RgbShiftShader("./effects/post_process.vs", "./effects/post_process_rgb_shift.fs");

    // 2) 注册Shader到后处理系统
    // 对于无参数调节的效果，可以直接写 nullptr 或不写配置回调
    postProcessing.registerEffect("Grayscale", GrayShader, nullptr);
    postProcessing.registerEffect("Invert", InvertShader, nullptr);
    postProcessing.registerEffect("Sepia", SepiaShader, nullptr);

    // 对于有默认配置的效果，可传入初始 lambda
    postProcessing.registerEffect("Vignette", VignetteShader, [](Shader& shader) {
        shader.setFloat("radius", 0.5f);  // 初始暗角半径
        shader.setFloat("softness", 0.2f);  // 初始暗角柔和度
    });

    postProcessing.registerEffect("RGB Shift", RgbShiftShader, [](Shader& shader) {
        shader.setFloat("strength", 0.01f); // 初始色差强度
    });

    // 初始化截图/录制管理器
    captureManager = std::make_unique<CaptureManager>(SCR_WIDTH, SCR_HEIGHT);

    // 绑定 Renderer 到窗口的用户指针
    glfwSetWindowUserPointer(window, this);

    // 初始化基础着色器
    basicShader = std::make_unique<Shader>("./shaders/basic.vs", "./shaders/basic.fs");
    if (!basicShader) {
        std::cerr << "Failed to create basic shader!" << std::endl;
        return false;
    }

    // 创建球体网格
    sphereMesh = createSphereMesh(1.0f, 16, 16);  // 半径为1，16x16的细分
    if (!sphereMesh) {
        std::cerr << "Failed to create sphere mesh!" << std::endl;
        return false;
    }

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
    //std::cout << "ImGui initialized successfully." << std::endl;
}

void Renderer::setCallbacks()
{
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);  // 鼠标按钮回调

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

        //std::cout << "Frame time: " << deltaTime << " seconds." << std::endl;

        // 处理输入
        processInput();

        // 更新阴影贴图
        updateShadowMaps();

        // 更新场景
        scene.update(deltaTime, lightingShader);

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
    ImGui::Begin("ToggleSidebar", nullptr,
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBackground |
        ImGuiWindowFlags_NoDecoration);
    if (ImGui::ArrowButton("##toggle", sidebar_open ? ImGuiDir_Right : ImGuiDir_Left)) {
        sidebar_open = !sidebar_open;
    }
    ImGui::End();

    // Sidebar window
    if (sidebar_open) {
        ImGui::SetNextWindowPos(ImVec2(SCR_WIDTH - sidebar_width, 0), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(sidebar_width, SCR_HEIGHT), ImGuiCond_Always);
        ImGui::Begin("Sidebar", nullptr,
            ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoMove);

        //------------------------------------------------------
        // 后处理效果控制
        //------------------------------------------------------
        {
            ImGui::Text("Post-Processing Effects");
            auto effectsState = postProcessing.getEffectsState();
            for (const auto& effect : effectsState) {
                bool enabled = effect.second;
                if (ImGui::Checkbox(effect.first.c_str(), &enabled)) {
                    postProcessing.enableEffect(effect.first, enabled);
                }
                // 如果启用效果则调节它的强度等参数
                if (effect.first == "RGB Shift" && enabled) {
                    static float rgbShiftStrength = 0.01f;
                    if (ImGui::SliderFloat("RGB Shift Strength", &rgbShiftStrength, 0.0f, 0.1f)) {
                        postProcessing.setEffectConfig("RGB Shift", [=](Shader& shader) {
                            shader.setFloat("strength", rgbShiftStrength);
                            });
                    }
                }
                else if (effect.first == "Vignette" && enabled) {
                    static float vignetteRadius = 0.5f;
                    static float vignetteSoftness = 0.2f;
                    bool changed = false;
                    changed |= ImGui::SliderFloat("Vignette Radius", &vignetteRadius, 0.0f, 1.0f);
                    changed |= ImGui::SliderFloat("Vignette Softness", &vignetteSoftness, 0.0f, 1.0f);
                    if (changed) {
                        postProcessing.setEffectConfig("Vignette", [=](Shader& shader) {
                            shader.setFloat("radius", vignetteRadius);
                            shader.setFloat("softness", vignetteSoftness);
                            });
                    }
                }
            }
        }

        ImGui::Separator();

        //------------------------------------------------------
        // 保存与加载场景
        //------------------------------------------------------
        {
            // 保存场景
            ImGui::Text("Save Scene");
            ImGui::InputText("File Name", saveFileName, IM_ARRAYSIZE(saveFileName));
            if (ImGui::Button("Save")) {
                std::string filePath = "scenes/" + std::string(saveFileName) + ".json";
                saveScene(filePath);
                availableScenes = getSceneFiles("scenes");
            }
            ImGui::Separator();

            // 加载场景
            ImGui::Text("Load Scene");
            if (availableScenes.empty()) {
                availableScenes = getSceneFiles("scenes");
            }
            if (!availableScenes.empty()) {
                if (ImGui::BeginCombo("Available Scenes", availableScenes[selectedSceneIndex].c_str())) {
                    for (int i = 0; i < (int)availableScenes.size(); ++i) {
                        bool isSelected = (selectedSceneIndex == i);
                        if (ImGui::Selectable(availableScenes[i].c_str(), isSelected)) {
                            selectedSceneIndex = i;
                        }
                        if (isSelected) ImGui::SetItemDefaultFocus();
                    }
                    ImGui::EndCombo();
                }
                if (ImGui::Button("Load")) {
                    std::string filePath = "scenes/" + availableScenes[selectedSceneIndex];
                    loadScene(filePath);
                }
            }
            else {
                ImGui::Text("No scenes available to load.");
            }
        }

        ImGui::Separator();

        //------------------------------------------------------
        // 调试选项
        //------------------------------------------------------
        {
            ImGui::Checkbox("Debug Light View", &debugLightView);
            if (debugLightView) {
                debugMaterialView = false;
                ImGui::SliderInt("Debug Light Index", &debugLightIndex, 0,
                    (int)lightManager.getLightCount() - 1);
            }

            ImGui::Checkbox("Debug Material View", &debugMaterialView);
            if (debugMaterialView) {
                debugLightView = false;
                ImGui::SliderInt("Debug Material Index", &debugMaterialIndex, 0, 4);
            }
        }

        ImGui::Separator();

        //------------------------------------------------------
        // 材质参数编辑
        //------------------------------------------------------
        {
            ImGui::Text("Material Parameters");
            static int selectedObjectM = 0;
            static int selectedMesh = 0;

            auto& gameObjects = scene.getGameObjects();
            if (!gameObjects.empty()) {
                // 展示所有 GameObject 及其 Mesh
                ImGui::BeginChild("GameObjects", ImVec2(0, 200), true);

                for (int objIndex = 0; objIndex < (int)gameObjects.size(); ++objIndex) {
                    const auto& obj = gameObjects[objIndex];
                    std::string nodeLabel = obj->getName() + "###GameObject_" + std::to_string(objIndex);

                    // 动态计算宽度，确保名称不会被截断
                    ImVec2 textSize = ImGui::CalcTextSize(nodeLabel.c_str());
                    ImGui::PushItemWidth(std::max(textSize.x + 20, 200.0f)); // 最小宽度 200，动态调整

                    bool isSelected = (selectedObjectM == objIndex);
                    if (ImGui::TreeNodeEx((void*)(intptr_t)objIndex,
                        ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen |
                        (isSelected ? ImGuiTreeNodeFlags_Selected : 0),
                        nodeLabel.c_str())) {
                        const auto& model = obj->getModel();
                        for (int m = 0; m < (int)model.meshes.size(); ++m) {
                            std::string meshLabel = "Mesh " + std::to_string(m) +
                                "###Mesh_" + std::to_string(objIndex) +
                                "_" + std::to_string(m);

                            // 动态调整 Selectable 宽度
                            ImVec2 meshTextSize = ImGui::CalcTextSize(meshLabel.c_str());
                            ImGui::PushItemWidth(std::max(meshTextSize.x + 20, 200.0f));

                            bool meshSelected = (selectedObjectM == objIndex && selectedMesh == m);
                            if (ImGui::Selectable(meshLabel.c_str(), meshSelected)) {
                                selectedObjectM = objIndex;
                                selectedMesh = m;
                            }

                            ImGui::PopItemWidth(); // 恢复默认宽度
                        }
                    }
                    ImGui::PopItemWidth(); // 恢复默认宽度
                }
                ImGui::EndChild();

                // 获取选定的材质
                const auto& selectedObjPtr = gameObjects[selectedObjectM];
                if (selectedMesh < selectedObjPtr->getModel().meshes.size()) {
                    PBRMaterial& material = selectedObjPtr->getPBRMaterial(selectedMesh);

                    ImGui::Separator();
                    ImGui::Text("Editing: %s - Mesh %d",
                        selectedObjPtr->getName().c_str(),
                        selectedMesh);

                    float albedo[3] = { material.albedo.r, material.albedo.g, material.albedo.b };
                    if (ImGui::ColorEdit3("Albedo", albedo)) {
                        material.albedo = glm::vec3(albedo[0], albedo[1], albedo[2]);
                    }
                    ImGui::SliderFloat("Metallic", &material.metallic, 0.0f, 1.0f);
                    ImGui::SliderFloat("Roughness", &material.roughness, 0.05f, 1.0f);
                    ImGui::SliderFloat("AO", &material.ao, 0.0f, 1.0f);

                    bool hasAlbedo = (material.albedoMap != 0);
                    bool hasMetallic = (material.metallicMap != 0);
                    bool hasRoughness = (material.roughnessMap != 0);
                    bool hasNormal = (material.normalMap != 0);
                    bool hasAO = (material.aoMap != 0);

                    if (hasAlbedo) {
                        ImGui::Checkbox("Use Albedo Map", &material.useAlbedoMap);
                    }
                    else {
                        ImGui::Text("Use Albedo Map: No Texture");
                    }
                    if (hasMetallic) {
                        ImGui::Checkbox("Use Metallic Map", &material.useMetallicMap);
                    }
                    else {
                        ImGui::Text("Use Metallic Map: No Texture");
                    }
                    if (hasRoughness) {
                        ImGui::Checkbox("Use Roughness Map", &material.useRoughnessMap);
                    }
                    else {
                        ImGui::Text("Use Roughness Map: No Texture");
                    }
                    if (hasNormal) {
                        ImGui::Checkbox("Use Normal Map", &material.useNormalMap);
                    }
                    else {
                        ImGui::Text("Use Normal Map: No Texture");
                    }
                    if (hasAO) {
                        ImGui::Checkbox("Use AO Map", &material.useAOMap);
                    }
                    else {
                        ImGui::Text("Use AO Map: No Texture");
                    }
                }
            }
            else {
                ImGui::Text("No objects in the scene.");
            }
        }

        ImGui::Separator();

        //------------------------------------------------------
        // 物体的变换和删除
        //------------------------------------------------------
        {
            ImGui::Text("Scene Objects");
            auto& gameObjects = scene.getGameObjects();

            static int transformSelectedObjIndex = 0;
            if (!gameObjects.empty()) {
                ImGui::BeginChild("Object List Transform", ImVec2(0, 200), true);
                for (int i = 0; i < (int)gameObjects.size(); ++i) {
                    auto& gameObject = gameObjects[i];
                    bool isSelected = (i == transformSelectedObjIndex);
                    if (ImGui::Selectable(gameObject->getName().c_str(), isSelected)) {
                        transformSelectedObjIndex = i;
                    }
                }
                ImGui::EndChild();

                ImGui::Separator();

                // 显示并修改位置、旋转、缩放
                auto& targetObj = gameObjects[transformSelectedObjIndex];
                glm::vec3 position = targetObj->getPosition();
                if (ImGui::DragFloat3("Position", &position.x, 0.1f)) {
                    targetObj->setPosition(position);
                }
                glm::vec3 scale = targetObj->getScale();
                if (ImGui::DragFloat3("Scale", &scale.x, 0.1f, 0.1f, 10.0f)) {
                    targetObj->setScale(scale);
                }
                static float uniformScale = (scale.x + scale.y + scale.z) / 3.0f;
                if (ImGui::SliderFloat("Uniform Scale", &uniformScale, 0.001f, 5.0f)) {
                    targetObj->setScale(glm::vec3(uniformScale));
                }
                glm::vec3 rotation = targetObj->getRotation();
                if (ImGui::DragFloat3("Rotation", &rotation.x, 1.0f, 0.0f, 360.0f)) {
                    targetObj->setRotation(rotation);
                }

                // 删除该物体
                if (ImGui::Button("Delete This Model")) {
                    scene.getGameObjects().erase(scene.getGameObjects().begin() + transformSelectedObjIndex);
                    transformSelectedObjIndex = 0;
                }
            }
            else {
                ImGui::Text("No objects in the scene.");
            }
        }

        ImGui::Separator();

        //------------------------------------------------------
        // 添加新模型
        //------------------------------------------------------
        static int selectedModelIndex = 0;
        static glm::vec3 initialPosition = glm::vec3(0.0f);
        static glm::vec3 initialRotation = glm::vec3(0.0f);
        static glm::vec3 initialScale = glm::vec3(1.0f);

        // 遍历文件夹并递归查找 .obj 文件
        std::vector<std::string> availableModels;
        std::vector<std::string> modelPaths;
        for (const auto& entry : std::filesystem::recursive_directory_iterator("./resources/objects")) {
            if (entry.is_regular_file() && entry.path().extension() == ".obj") {
                // 使用 std::filesystem::path 生成统一的路径
                std::string fullPath = entry.path().string();
                std::replace(fullPath.begin(), fullPath.end(), '\\', '/'); // 替换 \ 为 /

                modelPaths.push_back(fullPath); // 保存完整路径
                availableModels.push_back(entry.path().filename().string()); // 保存文件名
            }
        }

        // 自动生成唯一名称
        auto generateUniqueName = [](const std::string& baseName, Scene& scene) {
            int counter = 1;
            std::string uniqueName = baseName;
            while (true) {
                bool exists = false;
                for (const auto& obj : scene.getGameObjects()) {
                    if (obj->getName() == uniqueName) {
                        exists = true;
                        break;
                    }
                }
                if (!exists) {
                    break;
                }
                uniqueName = baseName + "_" + std::to_string(counter++);
            }
            return uniqueName;
            };

        // 选择模型文件
        ImGui::Text("Add New Model");

        if (!availableModels.empty()) {
            if (ImGui::BeginCombo("Available Models", availableModels[selectedModelIndex].c_str())) {
                for (int i = 0; i < (int)availableModels.size(); ++i) {
                    bool isSelected = (i == selectedModelIndex);
                    if (ImGui::Selectable(availableModels[i].c_str(), isSelected)) {
                        selectedModelIndex = i;
                    }
                    if (isSelected) {
                        ImGui::SetItemDefaultFocus();
                    }
                }
                ImGui::EndCombo();
            }

            static char newObjectName[128] = "NewModel";
            ImGui::InputText("Object Name", newObjectName, IM_ARRAYSIZE(newObjectName));

            // 设置模型初始参数
            ImGui::Text("Initial Transform");
            ImGui::DragFloat3("addPosition", &initialPosition.x, 0.1f);
            ImGui::DragFloat3("addRotation", &initialRotation.x, 1.0f, 0.0f, 360.0f);
            ImGui::DragFloat3("addScale", &initialScale.x, 0.1f, 0.1f, 10.0f);

            // 添加模型按钮
            if (ImGui::Button("Add Model")) {
                std::string modelPath = modelPaths[selectedModelIndex];
                std::string baseName = std::string(newObjectName);
                std::string objectName = generateUniqueName(baseName, scene);

                try {
                    // 检查贴图加载逻辑在 Model 类中是否完整
                    auto newObject = std::make_shared<GameObject>(objectName, modelPath, initialPosition, initialScale, initialRotation);
                    scene.addGameObject(newObject);
                    ImGui::Text("Model added successfully!");
                }
                catch (const std::exception& e) {
                    ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Error: %s", e.what());
                }
            }
        }
        else {
            ImGui::Text("No models available in './resources/objects'.");
        }

        ImGui::Separator();

        // 物体名称
        if (selectedObject != nullptr) {
            std::string objName = selectedObject->getName();

            // 导出 OBJ 按钮
            if (ImGui::Button("Export as OBJ...")) {
                char filename[128] = "";
                strcpy_s(filename, sizeof(filename), (objName + ".obj").c_str());

                // 打开文件对话框
                OPENFILENAMEA ofn;
                ZeroMemory(&ofn, sizeof(ofn));
                ofn.lStructSize = sizeof(ofn);
                ofn.hwndOwner = glfwGetWin32Window(window);
                ofn.lpstrFilter = "OBJ Files\0*.obj\0All Files\0*.*\0";
                ofn.lpstrFile = filename;
                ofn.nMaxFile = sizeof(filename);
                ofn.lpstrTitle = "Save OBJ File";
                ofn.Flags = OFN_OVERWRITEPROMPT;
                ofn.lpstrDefExt = "obj";

                if (GetSaveFileNameA(&ofn)) {
                    if (exportToObj(selectedObject, filename)) {
                        std::cout << "Object exported successfully to: " << filename << std::endl;
                    }
                }
            }
        }
        else {
            // 处理 selectedObject 为 null 的情况
            ImGui::Text("No object selected.");
        }

        ImGui::Separator(); // 分隔符

        //------------------------------------------------------
        // 光源的添加、编辑、删除
        //------------------------------------------------------
        {
            ImGui::Text("Scene Lights");
            auto& lights = lightManager.getLights();

            // 添加光源
            static int selectedLightType = 0;
            const char* lightTypes[] = {
                "Directional Light", "Point Light", "Spot Light"
            };
            ImGui::Combo("Light Type", &selectedLightType,
                lightTypes, IM_ARRAYSIZE(lightTypes));
            if (ImGui::Button("Add Light")) {
                if (selectedLightType == 0) {
                    // 定向光
                    auto dirLight = std::make_shared<DirectionalLight>(
                        glm::vec3(-1.0f, -1.0f, -1.0f),
                        glm::vec3(1.0f));
                    lightManager.addLight(dirLight);
                    shadowManager.syncShadowDataWithLights(lightManager.getRawLights()); // 同步 ShadowManager
                }
                else if (selectedLightType == 1) {
                    // 点光源
                    auto pointLight = std::make_shared<PointLight>(
                        glm::vec3(0.0f, 5.0f, 0.0f),
                        glm::vec3(1.0f));
                    lightManager.addLight(pointLight);
                    shadowManager.syncShadowDataWithLights(lightManager.getRawLights()); // 同步 ShadowManager
                }
                else if (selectedLightType == 2) {
                    // 聚光灯
                    auto spotLight = std::make_shared<SpotLight>(
                        glm::vec3(0.0f, 5.0f, 0.0f),
                        glm::vec3(0.0f, -1.0f, 0.0f),
                        glm::vec3(1.0f), 1.0f, 45.0f);
                    lightManager.addLight(spotLight);
                    shadowManager.syncShadowDataWithLights(lightManager.getRawLights()); // 同步 ShadowManager
                }
            }

            ImGui::Separator();

            // 编辑和删除光源
            static int lightSelectedIndex = 0;
            if (!lights.empty()) {
                // 列表
                ImGui::BeginChild("Light List", ImVec2(0, 200), true);
                for (int i = 0; i < (int)lights.size(); ++i) {
                    std::string label = "Light " + std::to_string(i);
                    if (lights[i]->getType() == LightType::Directional) label += " (Directional)";
                    else if (lights[i]->getType() == LightType::Point) label += " (Point)";
                    else if (lights[i]->getType() == LightType::Spot) label += " (Spot)";
                    bool isSelected = (i == lightSelectedIndex);
                    if (ImGui::Selectable(label.c_str(), isSelected)) {
                        lightSelectedIndex = i;
                    }
                }
                ImGui::EndChild();

                ImGui::Text("Light Controls");
                auto& selectedLight = lights[lightSelectedIndex];
                auto lightType = selectedLight->getType();
                glm::vec3 color = selectedLight->getColor();
                float intensity = selectedLight->getIntensity();

                // 通用颜色和强度
                if (ImGui::ColorEdit3("LightColor", &color.x)) {
                    selectedLight->setColor(color);
                }
                if (ImGui::SliderFloat("LightIntensity", &intensity, 0.0f, 10.0f)) {
                    selectedLight->setIntensity(intensity);
                }

                // 根据类型显示不同内容
                if (lightType == LightType::Directional) {
                    glm::vec3 direction = selectedLight->getDirection();
                    if (ImGui::DragFloat3("LightDirection", &direction.x, 0.1f)) {
                        selectedLight->setDirection(direction);
                    }
                }
                else if (lightType == LightType::Point) {
                    glm::vec3 position = selectedLight->getPosition();
                    if (ImGui::DragFloat3("LightPosition", &position.x, 0.1f)) {
                        selectedLight->setPosition(position);
                    }
                }
                else if (lightType == LightType::Spot) {
                    glm::vec3 position = selectedLight->getPosition();
                    glm::vec3 direction = selectedLight->getDirection();
                    if (ImGui::DragFloat3("LightPosition", &position.x, 0.1f)) {
                        selectedLight->setPosition(position);
                    }
                    if (ImGui::DragFloat3("LightDirection", &direction.x, 0.1f)) {
                        selectedLight->setDirection(direction);
                    }
                    // 聚光灯的Cutoff
                    float cutoffAngle = dynamic_cast<SpotLight*>(selectedLight.get())->getCutoffAngle();
                    if (ImGui::SliderFloat("Cutoff Angle", &cutoffAngle, 1.0f, 90.0f)) {
                        dynamic_cast<SpotLight*>(selectedLight.get())->setCutoffAngle(cutoffAngle);
                    }
                }

                // 删除该光源
                if (ImGui::Button("Delete This Light")) {
                    if (lightSelectedIndex >= 0 && lightSelectedIndex < lightManager.getLightCount()) {
                        lightManager.removeLight(lightSelectedIndex);
                        shadowManager.syncShadowDataWithLights(lightManager.getRawLights());
                        lightSelectedIndex = std::max(0, lightSelectedIndex - 1); // 更新光源索引
                    }
                }
            }
            else {
                ImGui::Text("No lights in the scene.");
            }
        }

        //------------------------------------------------------
        // 天空盒
        //------------------------------------------------------

        ImGui::Separator(); // 分隔符

        // 天空盒控制
        if (ImGui::CollapsingHeader("Skybox Settings")) {
            ImGui::Checkbox("Enable Skybox", &enableSkybox);

            static bool usePanorama = false;
            ImGui::Checkbox("Use Panorama", &usePanorama);

            if (usePanorama) {
                static std::vector<std::string> availablePanoramas = getPanoramaList("textures/panorama");
                static int currentPanorama = 0;

                if (ImGui::BeginCombo("Panorama", availablePanoramas.empty() ? "No panoramas available" : availablePanoramas[currentPanorama].c_str())) {
                    for (int i = 0; i < availablePanoramas.size(); i++) {
                        bool isSelected = (currentPanorama == i);
                        if (ImGui::Selectable(availablePanoramas[i].c_str(), isSelected)) {
                            currentPanorama = i;
                            // 加载全景图
                            std::string panoramaPath = "textures/panorama/" + availablePanoramas[i];
                            loadSkyboxFromPanorama(panoramaPath);
                        }
                        if (isSelected) {
                            ImGui::SetItemDefaultFocus();
                        }
                    }
                    ImGui::EndCombo();
                }

                // 刷新全景图列表按钮
                if (ImGui::Button("Refresh Panorama List")) {
                    availablePanoramas = getPanoramaList("textures/panorama");
                }

                ImGui::Text("Panorama Directory: textures/panorama");
                ImGui::Text("Supported formats: .jpg, .png, .hdr");
            }
            else {
                static std::vector<std::string> availableSkyboxes = getSkyboxList("textures/skybox");
                static int currentSkybox = 0;

                if (ImGui::BeginCombo("Skybox", availableSkyboxes.empty() ? "No skyboxes available" : availableSkyboxes[currentSkybox].c_str())) {
                    for (int i = 0; i < availableSkyboxes.size(); i++) {
                        bool isSelected = (currentSkybox == i);
                        if (ImGui::Selectable(availableSkyboxes[i].c_str(), isSelected)) {
                            currentSkybox = i;
                            // 构建新的天空盒路径
                            std::vector<std::string> newPaths;
                            std::string basePath = "textures/skybox/" + availableSkyboxes[i] + "/";
                            newPaths.push_back(basePath + "right.jpg");
                            newPaths.push_back(basePath + "left.jpg");
                            newPaths.push_back(basePath + "top.jpg");
                            newPaths.push_back(basePath + "bottom.jpg");
                            newPaths.push_back(basePath + "front.jpg");
                            newPaths.push_back(basePath + "back.jpg");
                            loadSkybox(newPaths);
                        }
                        if (isSelected) {
                            ImGui::SetItemDefaultFocus();
                        }
                    }
                    ImGui::EndCombo();
                }

                // 刷新天空盒列表按钮
                if (ImGui::Button("Refresh Skybox List")) {
                    availableSkyboxes = getSkyboxList("textures/skybox");
                }

                ImGui::Text("Skybox Directory: textures/skybox");
                ImGui::Text("Required files for each skybox:");
                ImGui::BulletText("right.jpg, left.jpg");
                ImGui::BulletText("top.jpg, bottom.jpg");
                ImGui::BulletText("front.jpg, back.jpg");
            }
        }

        ImGui::Separator(); // 分隔符

        //------------------------------------------------------
        // 帮助信息
        //------------------------------------------------------
        if (!mouseCaptured) {
            ImGui::Separator();
            ImGui::Text("Controls Help");
            ImGui::Text("Press TAB to toggle mouse capture");
            ImGui::Text("Current Status: GUI Control Mode");
        }

        ImGui::End(); // end sidebar
    }
    //std::cout << "Rendering frame..." << std::endl;

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
            //std::cout << "SpotLight position and direction updated." << std::endl;
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
    lightingShader.setFloat("shadowMapResolution", Resolution);
    lightingShader.setFloat("far_plane", far_plane);

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

    // 绑定阴影贴图
    int shadowBaseUnit = 16;
    for (size_t i = 0; i < lightManager.getLightCount(); ++i)
    {
        const auto& light = lightManager.getLight(i);
        GLuint shadowTexture = shadowManager.getShadowTexture(i);

        if (light->getType() == LightType::Point)
        {
            if (shadowTexture != 0) // 确保有效的立方体贴图
            {
                glActiveTexture(GL_TEXTURE0 + shadowBaseUnit + i);
                glBindTexture(GL_TEXTURE_CUBE_MAP, shadowTexture);
                lightingShader.setInt("shadowCubeMaps[" + std::to_string(i) + "]", shadowBaseUnit + i);
                //std::cout << "shadowCubeMaps[" << i << "] bound to texture: " << i << std::endl;
            }
        }
        else // DirectionalLight 或 SpotLight
        {
            if (shadowTexture != 0) // 确保有效的 2D 阴影贴图
            {
                glActiveTexture(GL_TEXTURE0 + shadowBaseUnit + i);
                glBindTexture(GL_TEXTURE_2D, shadowTexture);
                lightingShader.setInt("shadowMaps[" + std::to_string(i) + "]", shadowBaseUnit + i);
                //std::cout << "shadowMaps[" << i << "] bound to texture: " << i << std::endl;
            }
        }
    }
    // 确保0号纹理单元清空绑定
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
    glBindTexture(GL_TEXTURE_2D, 0);

    lightingShader.setInt("lightCount", static_cast<int>(lightManager.getLightCount()));

    if (postProcessing.hasEnabledEffects()) {
        postProcessing.begin();
        scene.draw(lightingShader, selectedObject);

        // 渲染天空盒
        if (enableSkybox && skybox && skyboxShader) {
            glDepthFunc(GL_LEQUAL);
            skyboxShader->use();
            glm::mat4 skyboxView = glm::mat4(glm::mat3(view)); // 移除平移
            skyboxShader->setMat4("view", skyboxView);
            skyboxShader->setMat4("projection", projection);
            skybox->Draw(*skyboxShader, glm::mat4(1.0f), skyboxView, projection);
            glDepthFunc(GL_LESS);
        }

        postProcessing.endAndRender();
    }
    else {
        scene.draw(lightingShader, selectedObject); // 直接渲染到屏幕

        // 渲染天空盒
        if (enableSkybox && skybox && skyboxShader) {
            glDepthFunc(GL_LEQUAL);
            skyboxShader->use();
            glm::mat4 skyboxView = glm::mat4(glm::mat3(view)); // 移除平移
            skyboxShader->setMat4("view", skyboxView);
            skyboxShader->setMat4("projection", projection);
            skybox->Draw(*skyboxShader, glm::mat4(1.0f), skyboxView, projection);
            glDepthFunc(GL_LESS);
        }
    }

    //std::cout << "Rendering ImGui..." << std::endl;

    // 渲染ImGui
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    //std::cout << "Frame rendered." << std::endl;

    if (captureManager) {
        captureManager->recordFrame();
    }

    // 在所有物体渲染完成后，如果启用了包围球显示，则绘制包围球
    if (showBoundingSpheres) {
        for (const auto& obj : scene.getGameObjects()) {
            drawBoundingSphere(obj);
        }
    }
}

bool Renderer::exportToObj(const std::shared_ptr<GameObject>& obj, const std::string& filePath) {
    if (!obj) return false;

    std::ofstream file(filePath);
    if (!file.is_open()) {
        std::cerr << "Failed to open file for writing: " << filePath << std::endl;
        return false;
    }

    // 写入 OBJ 文件头部信息
    file << "# Exported from OpenGL Project\n";
    file << "# Object name: " << obj->getName() << "\n\n";

    const Model& model = obj->getModel();
    int vertexOffset = 1; // OBJ 文件的索引从 1 开始

    // 获取物体的变换矩阵
    glm::mat4 transform = obj->getModelMatrix();

    for (const auto& mesh : model.meshes) {
        // 写入顶点数据
        for (const auto& vertex : mesh.vertices) {
            // 应用变换矩阵到顶点位置
            glm::vec4 transformedPos = transform * glm::vec4(vertex.Position, 1.0f);
            file << "v " << transformedPos.x << " " << transformedPos.y << " " << transformedPos.z << "\n";

            // 写入纹理坐标
            file << "vt " << vertex.TexCoords.x << " " << vertex.TexCoords.y << "\n";

            // 写入法线
            glm::vec3 transformedNormal = glm::mat3(glm::transpose(glm::inverse(transform))) * vertex.Normal;
            transformedNormal = glm::normalize(transformedNormal);
            file << "vn " << transformedNormal.x << " " << transformedNormal.y << " " << transformedNormal.z << "\n";
        }

        // 写入面数据
        file << "\n# Faces\n";
        for (size_t i = 0; i < mesh.indices.size(); i += 3) {
            file << "f ";
            for (size_t j = 0; j < 3; ++j) {
                int idx = mesh.indices[i + j] + vertexOffset;
                file << idx << "/" << idx << "/" << idx;
                if (j < 2) file << " ";
            }
            file << "\n";
        }

        vertexOffset += mesh.vertices.size();
    }

    file.close();
    std::cout << "Successfully exported OBJ file to: " << filePath << std::endl;
    return true;
}

void Renderer::processInput()
{
    // 如果ImGui正在接收键盘输入，不处理键盘事件
    ImGuiIO& io = ImGui::GetIO();
    if (io.WantCaptureKeyboard) return;

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

    // 使用 B 键切换包围球显示
    static bool bPressed = false;
    if (glfwGetKey(window, GLFW_KEY_B) == GLFW_PRESS) {
        if (!bPressed) {
            bPressed = true;
            showBoundingSpheres = !showBoundingSpheres;
        }
    }
    else {
        bPressed = false;
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
        if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
            camera.ProcessKeyboard(UP, deltaTime);
        if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
            camera.ProcessKeyboard(DOWN, deltaTime);
    }

    // F 键截图
    static bool fPressed = false;
    if (glfwGetKey(window, GLFW_KEY_F) == GLFW_PRESS) {
        if (!fPressed) {
            fPressed = true;
            // 将文件保存到当前目录的 Capture 文件夹
            if (captureManager->captureScreen("./capture")) {
                std::cout << "Screenshot taken successfully." << std::endl;
            }
        }
    }
    else {
        fPressed = false;
    }

    // 动画控制
    static bool gPressed = false;
    static bool hPressed = false;
    static bool jPressed = false;

    if (glfwGetKey(window, GLFW_KEY_G) == GLFW_PRESS) {
        if (!gPressed) {  // G 键触发 Running 动画
            gPressed = true;
            if (Character) {
                Character->playAnimation("Running");
                std::cout << "Playing animation: Running" << std::endl;
            }
            else {
                std::cerr << "Character not set in Renderer." << std::endl;
            }
        }
    }
    else {
        gPressed = false;  // 松开 G 键
    }

    if (glfwGetKey(window, GLFW_KEY_H) == GLFW_PRESS) {
        if (!hPressed) {  // H 键触发 Idle 动画
            hPressed = true;
            if (Character) {
                Character->playAnimation("Idle");
                std::cout << "Playing animation: Idle" << std::endl;
            }
            else {
                std::cerr << "Character not set in Renderer." << std::endl;
            }
        }
    }
    else {
        hPressed = false;  // 松开 H 键
    }

    if (glfwGetKey(window, GLFW_KEY_J) == GLFW_PRESS) {
        if (!jPressed) {  // J 键触发 Jump 动画
            jPressed = true;
            if (Character) {
                Character->playAnimation("Jump");
                std::cout << "Playing animation: Jump" << std::endl;
            }
            else {
                std::cerr << "Character not set in Renderer." << std::endl;
            }
        }
    }
    else {
        jPressed = false;  // 松开 J 键
    }

    // 动画停止逻辑
    if (glfwGetKey(window, GLFW_KEY_T) == GLFW_PRESS) {
        if (Character) {
            Character->stopAnimation();
            std::cout << "Stopped animation." << std::endl;
        }
        else {
            std::cerr << "Character not set in Renderer." << std::endl;
        }
    }

    // R 键启动/停止录屏
    static bool rPressed = false;
    if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS) {
        if (!rPressed) {
            rPressed = true;
            // 启动或停止录屏
            if (!captureManager->startRecording("./capture", 30)) {
                captureManager->stopRecording();
                std::cout << "Recording stopped." << std::endl;
            }
        }
    }
    else {
        rPressed = false;
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

    // 设置阴影贴图分辨率
    shadowManager.updateShadowResolution(Resolution);

    // 生成阴影贴图
    shadowManager.generateShadowMaps(lights, scene, shadowShader, pointshadowShader);
}

void Renderer::drawBoundingSphere(const std::shared_ptr<GameObject>& obj) {
    if (!obj) return;

    // 获取物体的位置和缩放
    glm::vec3 objPos = obj->getPosition();
    glm::vec3 objScale = obj->getScale();
    float radius = glm::length(objScale) * 0.5f;

    // 保存当前的着色器程序
    GLint currentProgram;
    glGetIntegerv(GL_CURRENT_PROGRAM, &currentProgram);

    // 使用简单的着色器绘制线框球体
    basicShader->use();
    basicShader->setMat4("projection", camera.GetProjectionMatrix(float(SCR_WIDTH) / float(SCR_HEIGHT)));
    basicShader->setMat4("view", camera.GetViewMatrix());

    // 创建球体变换矩阵
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, objPos);
    model = glm::scale(model, glm::vec3(radius));
    basicShader->setMat4("model", model);

    // 设置线框颜色（使用绿色）
    basicShader->setVec3("color", glm::vec3(0.0f, 1.0f, 0.0f));

    // 启用线框模式
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

    // 禁用深度写入但保持深度测试
    glDepthMask(GL_FALSE);

    // 绘制球体
    sphereMesh->Draw(*basicShader);

    // 恢复深度写入
    glDepthMask(GL_TRUE);

    // 恢复填充模式
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    // 恢复之前的着色器程序
    glUseProgram(currentProgram);
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
    if (!renderer) return;

    float xpos = static_cast<float>(xposIn);
    float ypos = static_cast<float>(yposIn);

    if (renderer->firstMouse)
    {
        renderer->lastX = xpos;
        renderer->lastY = ypos;
        renderer->firstMouse = false;
    }

    float xoffset = xpos - renderer->lastX;
    float yoffset = renderer->lastY - ypos;

    renderer->lastX = xpos;
    renderer->lastY = ypos;

    if (renderer->mouseCaptured) {
        // 第一人称模式：控制相机
        renderer->camera.ProcessMouseMovement(xoffset, yoffset);
    }
    else if (renderer->mouseLeftButtonDown && renderer->selectedObject) {
        // 第三人称模式：拖动物体
        glm::vec3 rayDir = renderer->screenToWorldRay(xpos, ypos);
        glm::vec3 rayOrigin = renderer->camera.Position;

        // 计算新的物体位置
        float distance = glm::length(renderer->dragOffset);
        glm::vec3 newPos = rayOrigin + rayDir * distance;
        renderer->selectedObject->setPosition(newPos);
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

// 静态回调函数 - 鼠标按钮
void Renderer::mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
    Renderer* renderer = static_cast<Renderer*>(glfwGetWindowUserPointer(window));
    if (!renderer || renderer->mouseCaptured) return;  // 第一人称模式下不处理

    // 如果鼠标在ImGui界面上，不处理点击事件
    ImGuiIO& io = ImGui::GetIO();
    if (io.WantCaptureMouse) return;

    if (button == GLFW_MOUSE_BUTTON_LEFT) {
        renderer->mouseLeftButtonDown = (action == GLFW_PRESS);

        if (action == GLFW_PRESS) {
            double xpos, ypos;
            glfwGetCursorPos(window, &xpos, &ypos);

            // 发射射线
            glm::vec3 rayDir = renderer->screenToWorldRay(static_cast<float>(xpos), static_cast<float>(ypos));
            glm::vec3 rayOrigin = renderer->camera.Position;

            // 选择物体
            auto newSelectedObject = renderer->pickObject(rayOrigin, rayDir);
            if (newSelectedObject) {
                // 如果之前有选中的物体，取消其选中状态
                if (renderer->selectedObject) {
                    renderer->selectedObject->setIsSelected(false);
                }
                renderer->selectedObject = newSelectedObject;
                renderer->selectedObject->setIsSelected(true);
                // 计算拖动偏移量
                renderer->dragOffset = renderer->selectedObject->getPosition() - rayOrigin;
            }
            else {
                // 只有在点击空白处时才取消选择
                if (renderer->selectedObject) {
                    renderer->selectedObject->setIsSelected(false);
                    renderer->selectedObject = nullptr;
                }
            }
        }
    }
}

// 屏幕坐标转世界射线
glm::vec3 Renderer::screenToWorldRay(float mouseX, float mouseY)
{
    // 将屏幕坐标转换为标准化设备坐标 (-1 到 1)
    float x = (2.0f * mouseX) / SCR_WIDTH - 1.0f;
    float y = 1.0f - (2.0f * mouseY) / SCR_HEIGHT;

    // 构建裁剪空间坐标
    glm::vec4 clipCoords(x, y, -1.0f, 1.0f);

    // 转换到视图空间
    float aspectRatio = static_cast<float>(SCR_WIDTH) / static_cast<float>(SCR_HEIGHT);
    glm::mat4 projInverse = glm::inverse(camera.GetProjectionMatrix(aspectRatio));
    glm::vec4 viewCoords = projInverse * clipCoords;
    viewCoords.z = -1.0f;
    viewCoords.w = 0.0f;

    // 转换到世界空间
    glm::mat4 viewInverse = glm::inverse(camera.GetViewMatrix());
    glm::vec4 worldCoords = viewInverse * viewCoords;

    // 获取射线方向并归一化
    glm::vec3 rayDir = glm::normalize(glm::vec3(worldCoords));
    return rayDir;
}

// 射线拾取物体
std::shared_ptr<GameObject> Renderer::pickObject(const glm::vec3& rayOrigin, const glm::vec3& rayDir)
{
    float closestDist = std::numeric_limits<float>::max();
    std::shared_ptr<GameObject> closestObj = nullptr;

    for (auto& obj : scene.getGameObjects()) {
        // 获取物体的包围盒
        glm::vec3 objPos = obj->getPosition();
        glm::vec3 objScale = obj->getScale();

        // 使用更大的包围盒半径来确保能选中物体
        float radius = glm::length(objScale) * 0.5;

        // 计算射线到物体中心的最近点
        glm::vec3 toCenter = objPos - rayOrigin;
        float tCenter = glm::dot(toCenter, rayDir);

        if (tCenter < 0) continue;  // 物体在射线后方

        glm::vec3 closest = rayOrigin + rayDir * tCenter;
        float dist = glm::length(closest - objPos);

        // 增大选择容差
        if (dist < radius && tCenter < closestDist) {
            closestDist = tCenter;
            closestObj = obj;
        }
    }

    return closestObj;
}

// 创建球体网格的辅助函数
std::shared_ptr<Mesh> Renderer::createSphereMesh(float radius, int segments, int rings) {
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    std::vector<Texture> textures;  // 空纹理列表，因为我们只需要绘制线框

    // 生成顶点
    for (int ring = 0; ring <= rings; ++ring) {
        float phi = glm::pi<float>() * float(ring) / float(rings);
        for (int segment = 0; segment <= segments; ++segment) {
            float theta = 2.0f * glm::pi<float>() * float(segment) / float(segments);

            float x = radius * sin(phi) * cos(theta);
            float y = radius * cos(phi);
            float z = radius * sin(phi) * sin(theta);

            glm::vec3 position(x, y, z);
            glm::vec3 normal = glm::normalize(position);
            glm::vec2 texCoords(float(segment) / segments, float(ring) / rings);

            Vertex vertex;
            vertex.Position = position;
            vertex.Normal = normal;
            vertex.TexCoords = texCoords;
            vertex.Tangent = glm::vec3(0.0f);
            vertex.Bitangent = glm::vec3(0.0f);
            // 初始化骨骼数据
            for (int i = 0; i < MAX_BONE_INFLUENCE; i++) {
                vertex.boneIDs[i] = -1;
                vertex.weights[i] = 0.0f;
            }
            vertices.push_back(vertex);
        }
    }

    // 生成索引
    for (int ring = 0; ring < rings; ++ring) {
        for (int segment = 0; segment < segments; ++segment) {
            unsigned int current = ring * (segments + 1) + segment;
            unsigned int next = current + segments + 1;

            indices.push_back(current);
            indices.push_back(next);
            indices.push_back(current + 1);

            indices.push_back(next);
            indices.push_back(next + 1);
            indices.push_back(current + 1);
        }
    }

    return std::make_shared<Mesh>(vertices, indices, textures);
}

void Renderer::loadSkybox(const std::vector<std::string>& paths)
{
    if (paths.size() != 6) {
        std::cerr << "Error: Skybox requires exactly 6 texture paths" << std::endl;
        return;
    }

    try {
        skybox = std::make_unique<Skybox>(paths);
        skyboxPaths = paths;
    }
    catch (const std::exception& e) {
        std::cerr << "Failed to load skybox: " << e.what() << std::endl;
    }
}

std::vector<std::string> Renderer::getDefaultSkyboxPaths() const
{
    // 默认天空盒贴图路径
    return {
        "textures/skybox/right.jpg",
        "textures/skybox/left.jpg",
        "textures/skybox/top.jpg",
        "textures/skybox/bottom.jpg",
        "textures/skybox/front.jpg",
        "textures/skybox/back.jpg"
    };
}

std::vector<std::string> Renderer::getSkyboxList(const std::string& directory) const
{
    std::vector<std::string> skyboxes;
    WIN32_FIND_DATAA findData;
    HANDLE hFind = FindFirstFileA((directory + "/*").c_str(), &findData);

    if (hFind != INVALID_HANDLE_VALUE) {
        do {
            if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                std::string dirName = findData.cFileName;
                if (dirName != "." && dirName != "..") {
                    // 检查是否包含所需的六个文件
                    bool isValidSkybox = true;
                    std::vector<std::string> requiredFiles = { "right.jpg", "left.jpg", "top.jpg", "bottom.jpg", "front.jpg", "back.jpg" };
                    for (const auto& file : requiredFiles) {
                        std::string fullPath = directory + "/" + dirName + "/" + file;
                        WIN32_FIND_DATAA fileData;
                        HANDLE hFileFind = FindFirstFileA(fullPath.c_str(), &fileData);
                        if (hFileFind == INVALID_HANDLE_VALUE) {
                            isValidSkybox = false;
                            break;
                        }
                        FindClose(hFileFind);
                    }
                    if (isValidSkybox) {
                        skyboxes.push_back(dirName);
                    }
                }
            }
        } while (FindNextFileA(hFind, &findData));
        FindClose(hFind);
    }

    return skyboxes;
}

void Renderer::loadSkyboxFromPanorama(const std::string& panoramaPath)
{
    try {
        skybox = std::make_unique<Skybox>(panoramaPath);
    }
    catch (const std::runtime_error& e) {
        std::cerr << "Failed to load panorama skybox: " << e.what() << std::endl;
    }
}

std::vector<std::string> Renderer::getPanoramaList(const std::string& directory) const
{
    std::vector<std::string> panoramas;
    WIN32_FIND_DATAA findData;

    // 搜索 HDR 文件
    std::string searchPath = directory + "/*.hdr";
    HANDLE hFind = FindFirstFileA(searchPath.c_str(), &findData);

    if (hFind != INVALID_HANDLE_VALUE) {
        do {
            if (!(findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
                panoramas.push_back(findData.cFileName);
            }
        } while (FindNextFileA(hFind, &findData));
        FindClose(hFind);
    }

    // 搜索 JPG 文件
    searchPath = directory + "/*.jpg";
    hFind = FindFirstFileA(searchPath.c_str(), &findData);

    if (hFind != INVALID_HANDLE_VALUE) {
        do {
            if (!(findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
                panoramas.push_back(findData.cFileName);
            }
        } while (FindNextFileA(hFind, &findData));
        FindClose(hFind);
    }

    // 搜索 PNG 文件
    searchPath = directory + "/*.png";
    hFind = FindFirstFileA(searchPath.c_str(), &findData);

    if (hFind != INVALID_HANDLE_VALUE) {
        do {
            if (!(findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
                panoramas.push_back(findData.cFileName);
            }
        } while (FindNextFileA(hFind, &findData));
        FindClose(hFind);
    }

    return panoramas;
}
