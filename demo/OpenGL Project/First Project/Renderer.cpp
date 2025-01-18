#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#define RPC_NO_WINDOWS_H
#include <SDKDDKVer.h>
#include <Windows.h>
#include <commdlg.h>

// ȷ���ڰ�������ͷ�ļ�֮ǰȡ����Щ�궨��
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

char Renderer::saveFileName[128] = "scene"; // Ĭ�ϱ����ļ���
std::vector<std::string> Renderer::availableScenes = {}; // ��ʼ��Ϊ��
int Renderer::selectedSceneIndex = 0; // Ĭ��ѡ�еĳ�������
float Resolution = 5000.0f;
float far_plane = 100.0f;

// ���캯��
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
    showBoundingSpheres(false), // ��������Χ����ʾ��־
    enableSkybox(true) // Ĭ��������պ�
{
    // ��ʼ����պ�
    loadSkybox(getDefaultSkyboxPaths());
}

bool Renderer::initialize()
{
    if (window == nullptr) {
        std::cerr << "Renderer received a null window pointer." << std::endl;
        return false;
    }

    std::cout << "Configuring OpenGL..." << std::endl;
    // ����OpenGL״̬
    configureOpenGL();

    std::cout << "Setting callbacks..." << std::endl;

    // ���ûص�����
    setCallbacks();

    std::cout << "Initializing ImGui..." << std::endl;
    // ��ʼ�� ImGui
    initImGui();

    // ��ʼ������
    if (!postProcessing.initialize()) {
        std::cerr << "Failed to initialize post-processing!" << std::endl;
        return false;
    }

    // ��ʼ����պ���ɫ��
    skyboxShader = std::make_unique<Shader>("./shaders/skybox.vs", "./shaders/skybox.fs");
    if (!skyboxShader) {
        std::cerr << "Failed to create skybox shader!" << std::endl;
        return false;
    }

    // 1) ����Shader
    Shader GrayShader("./effects/post_process.vs", "./effects/post_process_Grayscale.fs");
    Shader InvertShader("./effects/post_process.vs", "./effects/post_process_Invert.fs");
    Shader SepiaShader("./effects/post_process.vs", "./effects/post_process_Sepia.fs");
    Shader VignetteShader("./effects/post_process.vs", "./effects/post_process_Vignette.fs");
    Shader RgbShiftShader("./effects/post_process.vs", "./effects/post_process_rgb_shift.fs");

    // 2) ע��Shader������ϵͳ
    // �����޲������ڵ�Ч��������ֱ��д nullptr ��д���ûص�
    postProcessing.registerEffect("Grayscale", GrayShader, nullptr);
    postProcessing.registerEffect("Invert", InvertShader, nullptr);
    postProcessing.registerEffect("Sepia", SepiaShader, nullptr);

    // ������Ĭ�����õ�Ч�����ɴ����ʼ lambda
    postProcessing.registerEffect("Vignette", VignetteShader, [](Shader& shader) {
        shader.setFloat("radius", 0.5f);  // ��ʼ���ǰ뾶
        shader.setFloat("softness", 0.2f);  // ��ʼ������Ͷ�
    });

    postProcessing.registerEffect("RGB Shift", RgbShiftShader, [](Shader& shader) {
        shader.setFloat("strength", 0.01f); // ��ʼɫ��ǿ��
    });

    // ��ʼ����ͼ/¼�ƹ�����
    captureManager = std::make_unique<CaptureManager>(SCR_WIDTH, SCR_HEIGHT);

    // �� Renderer �����ڵ��û�ָ��
    glfwSetWindowUserPointer(window, this);

    // ��ʼ��������ɫ��
    basicShader = std::make_unique<Shader>("./shaders/basic.vs", "./shaders/basic.fs");
    if (!basicShader) {
        std::cerr << "Failed to create basic shader!" << std::endl;
        return false;
    }

    // ������������
    sphereMesh = createSphereMesh(1.0f, 16, 16);  // �뾶Ϊ1��16x16��ϸ��
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
    if (!ImGui_ImplOpenGL3_Init("#version 330")) { // ȷ�������� OpenGL �汾ƥ��
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
    glfwSetMouseButtonCallback(window, mouse_button_callback);  // ��갴ť�ص�

    // ������겶��
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
}

void Renderer::run()
{
    std::cout << "Starting render loop..." << std::endl;
    while (!glfwWindowShouldClose(window))
    {
        // ����ʱ���
        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        //std::cout << "Frame time: " << deltaTime << " seconds." << std::endl;

        // ��������
        processInput();

        // ������Ӱ��ͼ
        updateShadowMaps();

        // ���³���
        scene.update(deltaTime, lightingShader);

        // ��Ⱦ
        renderFrame();

        // ��������������ѯ�¼�
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    std::cout << "Exiting render loop." << std::endl;

    // ������Դ
    cleanup();
}

void Renderer::renderFrame()
{

    // ����ImGui֡
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
        // ����Ч������
        //------------------------------------------------------
        {
            ImGui::Text("Post-Processing Effects");
            auto effectsState = postProcessing.getEffectsState();
            for (const auto& effect : effectsState) {
                bool enabled = effect.second;
                if (ImGui::Checkbox(effect.first.c_str(), &enabled)) {
                    postProcessing.enableEffect(effect.first, enabled);
                }
                // �������Ч�����������ǿ�ȵȲ���
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
        // ��������س���
        //------------------------------------------------------
        {
            // ���泡��
            ImGui::Text("Save Scene");
            ImGui::InputText("File Name", saveFileName, IM_ARRAYSIZE(saveFileName));
            if (ImGui::Button("Save")) {
                std::string filePath = "scenes/" + std::string(saveFileName) + ".json";
                saveScene(filePath);
                availableScenes = getSceneFiles("scenes");
            }
            ImGui::Separator();

            // ���س���
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
        // ����ѡ��
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
        // ���ʲ����༭
        //------------------------------------------------------
        {
            ImGui::Text("Material Parameters");
            static int selectedObjectM = 0;
            static int selectedMesh = 0;

            auto& gameObjects = scene.getGameObjects();
            if (!gameObjects.empty()) {
                // չʾ���� GameObject ���� Mesh
                ImGui::BeginChild("GameObjects", ImVec2(0, 200), true);

                for (int objIndex = 0; objIndex < (int)gameObjects.size(); ++objIndex) {
                    const auto& obj = gameObjects[objIndex];
                    std::string nodeLabel = obj->getName() + "###GameObject_" + std::to_string(objIndex);

                    // ��̬�����ȣ�ȷ�����Ʋ��ᱻ�ض�
                    ImVec2 textSize = ImGui::CalcTextSize(nodeLabel.c_str());
                    ImGui::PushItemWidth(std::max(textSize.x + 20, 200.0f)); // ��С��� 200����̬����

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

                            // ��̬���� Selectable ���
                            ImVec2 meshTextSize = ImGui::CalcTextSize(meshLabel.c_str());
                            ImGui::PushItemWidth(std::max(meshTextSize.x + 20, 200.0f));

                            bool meshSelected = (selectedObjectM == objIndex && selectedMesh == m);
                            if (ImGui::Selectable(meshLabel.c_str(), meshSelected)) {
                                selectedObjectM = objIndex;
                                selectedMesh = m;
                            }

                            ImGui::PopItemWidth(); // �ָ�Ĭ�Ͽ��
                        }
                    }
                    ImGui::PopItemWidth(); // �ָ�Ĭ�Ͽ��
                }
                ImGui::EndChild();

                // ��ȡѡ���Ĳ���
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
        // ����ı任��ɾ��
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

                // ��ʾ���޸�λ�á���ת������
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

                // ɾ��������
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
        // �����ģ��
        //------------------------------------------------------
        static int selectedModelIndex = 0;
        static glm::vec3 initialPosition = glm::vec3(0.0f);
        static glm::vec3 initialRotation = glm::vec3(0.0f);
        static glm::vec3 initialScale = glm::vec3(1.0f);

        // �����ļ��в��ݹ���� .obj �ļ�
        std::vector<std::string> availableModels;
        std::vector<std::string> modelPaths;
        for (const auto& entry : std::filesystem::recursive_directory_iterator("./resources/objects")) {
            if (entry.is_regular_file() && entry.path().extension() == ".obj") {
                // ʹ�� std::filesystem::path ����ͳһ��·��
                std::string fullPath = entry.path().string();
                std::replace(fullPath.begin(), fullPath.end(), '\\', '/'); // �滻 \ Ϊ /

                modelPaths.push_back(fullPath); // ��������·��
                availableModels.push_back(entry.path().filename().string()); // �����ļ���
            }
        }

        // �Զ�����Ψһ����
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

        // ѡ��ģ���ļ�
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

            // ����ģ�ͳ�ʼ����
            ImGui::Text("Initial Transform");
            ImGui::DragFloat3("addPosition", &initialPosition.x, 0.1f);
            ImGui::DragFloat3("addRotation", &initialRotation.x, 1.0f, 0.0f, 360.0f);
            ImGui::DragFloat3("addScale", &initialScale.x, 0.1f, 0.1f, 10.0f);

            // ���ģ�Ͱ�ť
            if (ImGui::Button("Add Model")) {
                std::string modelPath = modelPaths[selectedModelIndex];
                std::string baseName = std::string(newObjectName);
                std::string objectName = generateUniqueName(baseName, scene);

                try {
                    // �����ͼ�����߼��� Model �����Ƿ�����
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

        // ��������
        if (selectedObject != nullptr) {
            std::string objName = selectedObject->getName();

            // ���� OBJ ��ť
            if (ImGui::Button("Export as OBJ...")) {
                char filename[128] = "";
                strcpy_s(filename, sizeof(filename), (objName + ".obj").c_str());

                // ���ļ��Ի���
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
            // ���� selectedObject Ϊ null �����
            ImGui::Text("No object selected.");
        }

        ImGui::Separator(); // �ָ���

        //------------------------------------------------------
        // ��Դ����ӡ��༭��ɾ��
        //------------------------------------------------------
        {
            ImGui::Text("Scene Lights");
            auto& lights = lightManager.getLights();

            // ��ӹ�Դ
            static int selectedLightType = 0;
            const char* lightTypes[] = {
                "Directional Light", "Point Light", "Spot Light"
            };
            ImGui::Combo("Light Type", &selectedLightType,
                lightTypes, IM_ARRAYSIZE(lightTypes));
            if (ImGui::Button("Add Light")) {
                if (selectedLightType == 0) {
                    // �����
                    auto dirLight = std::make_shared<DirectionalLight>(
                        glm::vec3(-1.0f, -1.0f, -1.0f),
                        glm::vec3(1.0f));
                    lightManager.addLight(dirLight);
                    shadowManager.syncShadowDataWithLights(lightManager.getRawLights()); // ͬ�� ShadowManager
                }
                else if (selectedLightType == 1) {
                    // ���Դ
                    auto pointLight = std::make_shared<PointLight>(
                        glm::vec3(0.0f, 5.0f, 0.0f),
                        glm::vec3(1.0f));
                    lightManager.addLight(pointLight);
                    shadowManager.syncShadowDataWithLights(lightManager.getRawLights()); // ͬ�� ShadowManager
                }
                else if (selectedLightType == 2) {
                    // �۹��
                    auto spotLight = std::make_shared<SpotLight>(
                        glm::vec3(0.0f, 5.0f, 0.0f),
                        glm::vec3(0.0f, -1.0f, 0.0f),
                        glm::vec3(1.0f), 1.0f, 45.0f);
                    lightManager.addLight(spotLight);
                    shadowManager.syncShadowDataWithLights(lightManager.getRawLights()); // ͬ�� ShadowManager
                }
            }

            ImGui::Separator();

            // �༭��ɾ����Դ
            static int lightSelectedIndex = 0;
            if (!lights.empty()) {
                // �б�
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

                // ͨ����ɫ��ǿ��
                if (ImGui::ColorEdit3("LightColor", &color.x)) {
                    selectedLight->setColor(color);
                }
                if (ImGui::SliderFloat("LightIntensity", &intensity, 0.0f, 10.0f)) {
                    selectedLight->setIntensity(intensity);
                }

                // ����������ʾ��ͬ����
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
                    // �۹�Ƶ�Cutoff
                    float cutoffAngle = dynamic_cast<SpotLight*>(selectedLight.get())->getCutoffAngle();
                    if (ImGui::SliderFloat("Cutoff Angle", &cutoffAngle, 1.0f, 90.0f)) {
                        dynamic_cast<SpotLight*>(selectedLight.get())->setCutoffAngle(cutoffAngle);
                    }
                }

                // ɾ���ù�Դ
                if (ImGui::Button("Delete This Light")) {
                    if (lightSelectedIndex >= 0 && lightSelectedIndex < lightManager.getLightCount()) {
                        lightManager.removeLight(lightSelectedIndex);
                        shadowManager.syncShadowDataWithLights(lightManager.getRawLights());
                        lightSelectedIndex = std::max(0, lightSelectedIndex - 1); // ���¹�Դ����
                    }
                }
            }
            else {
                ImGui::Text("No lights in the scene.");
            }
        }

        //------------------------------------------------------
        // ��պ�
        //------------------------------------------------------

        ImGui::Separator(); // �ָ���

        // ��պп���
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
                            // ����ȫ��ͼ
                            std::string panoramaPath = "textures/panorama/" + availablePanoramas[i];
                            loadSkyboxFromPanorama(panoramaPath);
                        }
                        if (isSelected) {
                            ImGui::SetItemDefaultFocus();
                        }
                    }
                    ImGui::EndCombo();
                }

                // ˢ��ȫ��ͼ�б�ť
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
                            // �����µ���պ�·��
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

                // ˢ����պ��б�ť
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

        ImGui::Separator(); // �ָ���

        //------------------------------------------------------
        // ������Ϣ
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

    // ���������
    glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // ���¾۹�Ƶ�λ�úͷ���
    if (lightManager.getLightCount() > 0) {
        auto spotLightPtr = std::dynamic_pointer_cast<SpotLight>(lightManager.getLight(lightManager.getLightCount() - 1));
        if (spotLightPtr) {
            spotLightPtr->setPosition(camera.Position);
            spotLightPtr->setDirection(camera.Front);
            //std::cout << "SpotLight position and direction updated." << std::endl;
        }
    }

    // ���¹���UBO
    lightManager.updateUBO();

    // ��UBO����ɫ��
    lightingShader.use();
    lightManager.bindUBOToShader(lightingShader, 0); // ���� binding point Ϊ 0

    // ��ͼ/ͶӰ����
    glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom),
        static_cast<float>(SCR_WIDTH) / static_cast<float>(SCR_HEIGHT), 0.1f, 100.0f);
    glm::mat4 view = camera.GetViewMatrix();

    // ������ɫ��ͳһ����
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

    // �������й�Դ�Ĺ�ռ����
    std::vector<glm::mat4> lightSpaceMatrices;
	std::vector<Light*> lights;
    for (size_t i = 0; i < lightManager.getLightCount(); ++i) {
        lights.push_back(lightManager.getLight(i).get());
        lightSpaceMatrices.push_back(shadowManager.getLightSpaceMatrix(lights, i));
    }

    // ���ݹ�ռ������ɫ��
    for (size_t i = 0; i < lightSpaceMatrices.size(); ++i) {
        std::string matrixName = "lightSpaceMatrices[" + std::to_string(i) + "]";
        lightingShader.setMat4(matrixName, lightSpaceMatrices[i]);
    }

    // ����Ӱ��ͼ
    int shadowBaseUnit = 16;
    for (size_t i = 0; i < lightManager.getLightCount(); ++i)
    {
        const auto& light = lightManager.getLight(i);
        GLuint shadowTexture = shadowManager.getShadowTexture(i);

        if (light->getType() == LightType::Point)
        {
            if (shadowTexture != 0) // ȷ����Ч����������ͼ
            {
                glActiveTexture(GL_TEXTURE0 + shadowBaseUnit + i);
                glBindTexture(GL_TEXTURE_CUBE_MAP, shadowTexture);
                lightingShader.setInt("shadowCubeMaps[" + std::to_string(i) + "]", shadowBaseUnit + i);
                //std::cout << "shadowCubeMaps[" << i << "] bound to texture: " << i << std::endl;
            }
        }
        else // DirectionalLight �� SpotLight
        {
            if (shadowTexture != 0) // ȷ����Ч�� 2D ��Ӱ��ͼ
            {
                glActiveTexture(GL_TEXTURE0 + shadowBaseUnit + i);
                glBindTexture(GL_TEXTURE_2D, shadowTexture);
                lightingShader.setInt("shadowMaps[" + std::to_string(i) + "]", shadowBaseUnit + i);
                //std::cout << "shadowMaps[" << i << "] bound to texture: " << i << std::endl;
            }
        }
    }
    // ȷ��0������Ԫ��հ�
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
    glBindTexture(GL_TEXTURE_2D, 0);

    lightingShader.setInt("lightCount", static_cast<int>(lightManager.getLightCount()));

    if (postProcessing.hasEnabledEffects()) {
        postProcessing.begin();
        scene.draw(lightingShader, selectedObject);

        // ��Ⱦ��պ�
        if (enableSkybox && skybox && skyboxShader) {
            glDepthFunc(GL_LEQUAL);
            skyboxShader->use();
            glm::mat4 skyboxView = glm::mat4(glm::mat3(view)); // �Ƴ�ƽ��
            skyboxShader->setMat4("view", skyboxView);
            skyboxShader->setMat4("projection", projection);
            skybox->Draw(*skyboxShader, glm::mat4(1.0f), skyboxView, projection);
            glDepthFunc(GL_LESS);
        }

        postProcessing.endAndRender();
    }
    else {
        scene.draw(lightingShader, selectedObject); // ֱ����Ⱦ����Ļ

        // ��Ⱦ��պ�
        if (enableSkybox && skybox && skyboxShader) {
            glDepthFunc(GL_LEQUAL);
            skyboxShader->use();
            glm::mat4 skyboxView = glm::mat4(glm::mat3(view)); // �Ƴ�ƽ��
            skyboxShader->setMat4("view", skyboxView);
            skyboxShader->setMat4("projection", projection);
            skybox->Draw(*skyboxShader, glm::mat4(1.0f), skyboxView, projection);
            glDepthFunc(GL_LESS);
        }
    }

    //std::cout << "Rendering ImGui..." << std::endl;

    // ��ȾImGui
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    //std::cout << "Frame rendered." << std::endl;

    if (captureManager) {
        captureManager->recordFrame();
    }

    // ������������Ⱦ��ɺ���������˰�Χ����ʾ������ư�Χ��
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

    // д�� OBJ �ļ�ͷ����Ϣ
    file << "# Exported from OpenGL Project\n";
    file << "# Object name: " << obj->getName() << "\n\n";

    const Model& model = obj->getModel();
    int vertexOffset = 1; // OBJ �ļ��������� 1 ��ʼ

    // ��ȡ����ı任����
    glm::mat4 transform = obj->getModelMatrix();

    for (const auto& mesh : model.meshes) {
        // д�붥������
        for (const auto& vertex : mesh.vertices) {
            // Ӧ�ñ任���󵽶���λ��
            glm::vec4 transformedPos = transform * glm::vec4(vertex.Position, 1.0f);
            file << "v " << transformedPos.x << " " << transformedPos.y << " " << transformedPos.z << "\n";

            // д����������
            file << "vt " << vertex.TexCoords.x << " " << vertex.TexCoords.y << "\n";

            // д�뷨��
            glm::vec3 transformedNormal = glm::mat3(glm::transpose(glm::inverse(transform))) * vertex.Normal;
            transformedNormal = glm::normalize(transformedNormal);
            file << "vn " << transformedNormal.x << " " << transformedNormal.y << " " << transformedNormal.z << "\n";
        }

        // д��������
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
    // ���ImGui���ڽ��ռ������룬����������¼�
    ImGuiIO& io = ImGui::GetIO();
    if (io.WantCaptureKeyboard) return;

    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    // ʹ�� Tab ���л���겶��״̬
    static bool tabPressed = false;
    if (glfwGetKey(window, GLFW_KEY_TAB) == GLFW_PRESS) {
        if (!tabPressed) {  // ȷ��ֻ�ڰ��µ�˲���л�һ��
            tabPressed = true;
            mouseCaptured = !mouseCaptured;
            if (mouseCaptured) {
                glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
                firstMouse = true;  // �������״̬����ֹ�ӽ���ת
            }
            else {
                glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
            }
        }
    }
    else {
        tabPressed = false;
    }

    // ʹ�� B ���л���Χ����ʾ
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

    // ֻ����걻����ʱ��������ƶ�
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

    // F ����ͼ
    static bool fPressed = false;
    if (glfwGetKey(window, GLFW_KEY_F) == GLFW_PRESS) {
        if (!fPressed) {
            fPressed = true;
            // ���ļ����浽��ǰĿ¼�� Capture �ļ���
            if (captureManager->captureScreen("./capture")) {
                std::cout << "Screenshot taken successfully." << std::endl;
            }
        }
    }
    else {
        fPressed = false;
    }

    // ��������
    static bool gPressed = false;
    static bool hPressed = false;
    static bool jPressed = false;

    if (glfwGetKey(window, GLFW_KEY_G) == GLFW_PRESS) {
        if (!gPressed) {  // G ������ Running ����
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
        gPressed = false;  // �ɿ� G ��
    }

    if (glfwGetKey(window, GLFW_KEY_H) == GLFW_PRESS) {
        if (!hPressed) {  // H ������ Idle ����
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
        hPressed = false;  // �ɿ� H ��
    }

    if (glfwGetKey(window, GLFW_KEY_J) == GLFW_PRESS) {
        if (!jPressed) {  // J ������ Jump ����
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
        jPressed = false;  // �ɿ� J ��
    }

    // ����ֹͣ�߼�
    if (glfwGetKey(window, GLFW_KEY_T) == GLFW_PRESS) {
        if (Character) {
            Character->stopAnimation();
            std::cout << "Stopped animation." << std::endl;
        }
        else {
            std::cerr << "Character not set in Renderer." << std::endl;
        }
    }

    // R ������/ֹͣ¼��
    static bool rPressed = false;
    if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS) {
        if (!rPressed) {
            rPressed = true;
            // ������ֹͣ¼��
            if (!captureManager->startRecording("./capture", 30)) {
                captureManager->stopRecording();
                std::cout << "Recording stopped." << std::endl;
            }
        }
    }
    else {
        rPressed = false;
    }


    // �������Ϸ�߼��ص���ִ����
    if (gameLogicCallback) {
        gameLogicCallback();
    }
}

void Renderer::updateShadowMaps()
{
    // ��̬������Ӱ��ͼ
    std::vector<Light*> lights;
    const auto& gameObjects = scene.getGameObjects(); // Scene �ṩ�����б�

    // �ռ���Դָ��
    for (size_t i = 0; i < lightManager.getLightCount(); ++i) {
        lights.push_back(lightManager.getLight(i).get());
    }

    // ������Ӱ��ͼ�ֱ���
    shadowManager.updateShadowResolution(Resolution);

    // ������Ӱ��ͼ
    shadowManager.generateShadowMaps(lights, scene, shadowShader, pointshadowShader);
}

void Renderer::drawBoundingSphere(const std::shared_ptr<GameObject>& obj) {
    if (!obj) return;

    // ��ȡ�����λ�ú�����
    glm::vec3 objPos = obj->getPosition();
    glm::vec3 objScale = obj->getScale();
    float radius = glm::length(objScale) * 0.5f;

    // ���浱ǰ����ɫ������
    GLint currentProgram;
    glGetIntegerv(GL_CURRENT_PROGRAM, &currentProgram);

    // ʹ�ü򵥵���ɫ�������߿�����
    basicShader->use();
    basicShader->setMat4("projection", camera.GetProjectionMatrix(float(SCR_WIDTH) / float(SCR_HEIGHT)));
    basicShader->setMat4("view", camera.GetViewMatrix());

    // ��������任����
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, objPos);
    model = glm::scale(model, glm::vec3(radius));
    basicShader->setMat4("model", model);

    // �����߿���ɫ��ʹ����ɫ��
    basicShader->setVec3("color", glm::vec3(0.0f, 1.0f, 0.0f));

    // �����߿�ģʽ
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

    // �������д�뵫������Ȳ���
    glDepthMask(GL_FALSE);

    // ��������
    sphereMesh->Draw(*basicShader);

    // �ָ����д��
    glDepthMask(GL_TRUE);

    // �ָ����ģʽ
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    // �ָ�֮ǰ����ɫ������
    glUseProgram(currentProgram);
}

void Renderer::saveScene(const std::string& filePath) {
    try {
        nlohmann::json sceneJson = scene.serialize();
        std::ofstream file(filePath);
        if (file.is_open()) {
            file << sceneJson.dump(4); // ��ʽ�����
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
    // ����ImGui
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    std::cout << "ImGui cleaned up successfully." << std::endl;
}

// ��̬�ص����� - ���ڴ�С�仯
void Renderer::framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    // ��ȡ Renderer ʵ��
    Renderer* renderer = static_cast<Renderer*>(glfwGetWindowUserPointer(window));
    if (renderer) {
        renderer->SCR_WIDTH = width;
        renderer->SCR_HEIGHT = height;
        glViewport(0, 0, width, height);

        // ������Ӱ��ͼ�ֱ���
        int newResolution = std::max(width, height);
        renderer->shadowManager.updateShadowResolution(newResolution);
    }
}

// ��̬�ص����� - ����ƶ�
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
        // ��һ�˳�ģʽ���������
        renderer->camera.ProcessMouseMovement(xoffset, yoffset);
    }
    else if (renderer->mouseLeftButtonDown && renderer->selectedObject) {
        // �����˳�ģʽ���϶�����
        glm::vec3 rayDir = renderer->screenToWorldRay(xpos, ypos);
        glm::vec3 rayOrigin = renderer->camera.Position;

        // �����µ�����λ��
        float distance = glm::length(renderer->dragOffset);
        glm::vec3 newPos = rayOrigin + rayDir * distance;
        renderer->selectedObject->setPosition(newPos);
    }
}

// ��̬�ص����� - ������
void Renderer::scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    Renderer* renderer = static_cast<Renderer*>(glfwGetWindowUserPointer(window));
    if (renderer && renderer->mouseCaptured)
    {
        renderer->camera.ProcessMouseScroll(static_cast<float>(yoffset));
    }
}

// ��̬�ص����� - ��갴ť
void Renderer::mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
    Renderer* renderer = static_cast<Renderer*>(glfwGetWindowUserPointer(window));
    if (!renderer || renderer->mouseCaptured) return;  // ��һ�˳�ģʽ�²�����

    // ��������ImGui�����ϣ����������¼�
    ImGuiIO& io = ImGui::GetIO();
    if (io.WantCaptureMouse) return;

    if (button == GLFW_MOUSE_BUTTON_LEFT) {
        renderer->mouseLeftButtonDown = (action == GLFW_PRESS);

        if (action == GLFW_PRESS) {
            double xpos, ypos;
            glfwGetCursorPos(window, &xpos, &ypos);

            // ��������
            glm::vec3 rayDir = renderer->screenToWorldRay(static_cast<float>(xpos), static_cast<float>(ypos));
            glm::vec3 rayOrigin = renderer->camera.Position;

            // ѡ������
            auto newSelectedObject = renderer->pickObject(rayOrigin, rayDir);
            if (newSelectedObject) {
                // ���֮ǰ��ѡ�е����壬ȡ����ѡ��״̬
                if (renderer->selectedObject) {
                    renderer->selectedObject->setIsSelected(false);
                }
                renderer->selectedObject = newSelectedObject;
                renderer->selectedObject->setIsSelected(true);
                // �����϶�ƫ����
                renderer->dragOffset = renderer->selectedObject->getPosition() - rayOrigin;
            }
            else {
                // ֻ���ڵ���հ״�ʱ��ȡ��ѡ��
                if (renderer->selectedObject) {
                    renderer->selectedObject->setIsSelected(false);
                    renderer->selectedObject = nullptr;
                }
            }
        }
    }
}

// ��Ļ����ת��������
glm::vec3 Renderer::screenToWorldRay(float mouseX, float mouseY)
{
    // ����Ļ����ת��Ϊ��׼���豸���� (-1 �� 1)
    float x = (2.0f * mouseX) / SCR_WIDTH - 1.0f;
    float y = 1.0f - (2.0f * mouseY) / SCR_HEIGHT;

    // �����ü��ռ�����
    glm::vec4 clipCoords(x, y, -1.0f, 1.0f);

    // ת������ͼ�ռ�
    float aspectRatio = static_cast<float>(SCR_WIDTH) / static_cast<float>(SCR_HEIGHT);
    glm::mat4 projInverse = glm::inverse(camera.GetProjectionMatrix(aspectRatio));
    glm::vec4 viewCoords = projInverse * clipCoords;
    viewCoords.z = -1.0f;
    viewCoords.w = 0.0f;

    // ת��������ռ�
    glm::mat4 viewInverse = glm::inverse(camera.GetViewMatrix());
    glm::vec4 worldCoords = viewInverse * viewCoords;

    // ��ȡ���߷��򲢹�һ��
    glm::vec3 rayDir = glm::normalize(glm::vec3(worldCoords));
    return rayDir;
}

// ����ʰȡ����
std::shared_ptr<GameObject> Renderer::pickObject(const glm::vec3& rayOrigin, const glm::vec3& rayDir)
{
    float closestDist = std::numeric_limits<float>::max();
    std::shared_ptr<GameObject> closestObj = nullptr;

    for (auto& obj : scene.getGameObjects()) {
        // ��ȡ����İ�Χ��
        glm::vec3 objPos = obj->getPosition();
        glm::vec3 objScale = obj->getScale();

        // ʹ�ø���İ�Χ�а뾶��ȷ����ѡ������
        float radius = glm::length(objScale) * 0.5;

        // �������ߵ��������ĵ������
        glm::vec3 toCenter = objPos - rayOrigin;
        float tCenter = glm::dot(toCenter, rayDir);

        if (tCenter < 0) continue;  // ���������ߺ�

        glm::vec3 closest = rayOrigin + rayDir * tCenter;
        float dist = glm::length(closest - objPos);

        // ����ѡ���ݲ�
        if (dist < radius && tCenter < closestDist) {
            closestDist = tCenter;
            closestObj = obj;
        }
    }

    return closestObj;
}

// ������������ĸ�������
std::shared_ptr<Mesh> Renderer::createSphereMesh(float radius, int segments, int rings) {
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    std::vector<Texture> textures;  // �������б���Ϊ����ֻ��Ҫ�����߿�

    // ���ɶ���
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
            // ��ʼ����������
            for (int i = 0; i < MAX_BONE_INFLUENCE; i++) {
                vertex.boneIDs[i] = -1;
                vertex.weights[i] = 0.0f;
            }
            vertices.push_back(vertex);
        }
    }

    // ��������
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
    // Ĭ����պ���ͼ·��
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
                    // ����Ƿ��������������ļ�
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

    // ���� HDR �ļ�
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

    // ���� JPG �ļ�
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

    // ���� PNG �ļ�
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
