// Renderer.cpp
#include "Renderer.h"
#include <iostream>

// ���캯��
Renderer::Renderer(GLFWwindow* win, unsigned int width, unsigned int height, Camera& cam,
    LightManager& lm, ShadowManager& sm,
    std::vector<GameObject>& objects)
    : window(win), SCR_WIDTH(width), SCR_HEIGHT(height),
    camera(cam), lightManager(lm), shadowManager(sm),
    sceneObjects(objects),
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
    // ����OpenGL״̬
    configureOpenGL();

    std::cout << "Setting callbacks..." << std::endl;

    // ���ûص�����
    setCallbacks();

    std::cout << "Initializing ImGui..." << std::endl;
    // ��ʼ�� ImGui
    initImGui();

    // �� Renderer �����ڵ��û�ָ��
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
    if (!ImGui_ImplOpenGL3_Init("#version 330")) { // ȷ�������� OpenGL �汾ƥ��
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

        std::cout << "Frame time: " << deltaTime << " seconds." << std::endl;

        // ��������
        processInput();

        // ������Ӱ��ͼ
        updateShadowMaps();

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

    // ���ƿ������
    ImGui::Begin("Light Controls");
    if (lightManager.getLightCount() > 0) {
        auto spotLightPtr = std::dynamic_pointer_cast<SpotLight>(lightManager.getLight(lightManager.getLightCount() - 1));
        if (spotLightPtr) {
            // ǿ�ȿ���
            float intensity = spotLightPtr->getIntensity();
            if (ImGui::SliderFloat("Spotlight Intensity", &intensity, 0.0f, 5.0f)) {
                spotLightPtr->setIntensity(intensity);
            }

            // �й�Ƕȿ���
            float cutoffAngle = spotLightPtr->getCutoffAngle();
            if (ImGui::SliderFloat("Spotlight Cutoff Angle", &cutoffAngle, 1.0f, 45.0f)) {
                spotLightPtr->setCutoffAngle(cutoffAngle);
            }

            // ��ɫ����
            glm::vec3 color = spotLightPtr->getColor();
            float colorArray[3] = { color.r, color.g, color.b };
            if (ImGui::ColorEdit3("Spotlight Color", colorArray)) {
                spotLightPtr->setColor(glm::vec3(colorArray[0], colorArray[1], colorArray[2]));
            }
        }
    }

    ImGui::Checkbox("Debug Light View", &debugLightView);
    if (debugLightView) {
        debugMaterialView = false;
        ImGui::SliderInt("Debug Light Index", &debugLightIndex, 0, static_cast<int>(lightManager.getLightCount()) - 1, "%d");
    }

    ImGui::Checkbox("Debug Material View", &debugMaterialView);
    if (debugMaterialView) {
        debugLightView = false;
        ImGui::SliderInt("Debug Material Index", &debugMaterialIndex, 0, 47, "%d");
    }

    ImGui::End();

    // ��ʾ��������
    if (!mouseCaptured) {
        ImGui::Begin("Controls Help");
        ImGui::Text("Press TAB to toggle mouse capture");
        ImGui::Text("Current Status: GUI Control Mode");
        ImGui::End();
    }

    std::cout << "Rendering frame..." << std::endl;

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
            std::cout << "SpotLight position and direction updated." << std::endl;
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

    // ��������Ӱ��ͼ
    for (size_t i = 0; i < lightManager.getLightCount(); ++i) {
        glActiveTexture(GL_TEXTURE2 + static_cast<GLenum>(i)); // ��GL_TEXTURE2��ʼ
        glBindTexture(GL_TEXTURE_2D, shadowManager.getShadowTexture(i));
        std::string shadowMapName = "shadowMaps[" + std::to_string(i) + "]";
        lightingShader.setInt(shadowMapName, 2 + static_cast<int>(i));
    }

    lightingShader.setInt("lightCount", static_cast<int>(lightManager.getLightCount()));

    // ��Ⱦ������Ϸ����
    for (auto& object : sceneObjects) {
        object.draw(lightingShader.ID);
    }

    std::cout << "Rendering ImGui..." << std::endl;

    // ��ȾImGui
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    std::cout << "Frame rendered." << std::endl;
}

void Renderer::processInput()
{
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
    std::vector<GameObject*> objects;

    // �ռ���������ָ��
    for (auto& obj : sceneObjects) {
        objects.push_back(&obj);
    }

    // �ռ���Դָ��
    for (size_t i = 0; i < lightManager.getLightCount(); ++i) {
        lights.push_back(lightManager.getLight(i).get());
    }

    // ������Ӱ��ͼ�ֱ��ʣ��ɸ�����Ҫ������
    shadowManager.updateShadowResolution(4096);

    // ������Ӱ��ͼ
    shadowManager.generateShadowMaps(lights, objects, shadowShader.ID);
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
        float yoffset = renderer->lastY - ypos; // y�����Ƿ����

        renderer->lastX = xpos;
        renderer->lastY = ypos;

        renderer->camera.ProcessMouseMovement(xoffset, yoffset);
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