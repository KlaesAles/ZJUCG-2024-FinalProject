// Renderer.h
#ifndef RENDERER_H
#define RENDERER_H

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "Shader.h"
#include "Camera.h"
#include "LightManager.h"
#include "ShadowManager.h"
#include "Scene.h"
#include "PostProcessing.h"
#include "CaptureManager.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include <vector>
#include <memory>
#include <functional>

class Renderer {
public:
    // ���캯��
    Renderer(GLFWwindow* window, unsigned int width, unsigned int height, Camera& camera,
        LightManager& lightManager, ShadowManager& shadowManager, Scene& scene);

    // ��ʼ����Ⱦ��
    bool initialize();

    // ������Ⱦѭ��
    void run();

    // ������Դ
    void cleanup();

    // ������Ϸ�߼��ص�
    void setGameLogicCallback(std::function<void()> callback) {
        gameLogicCallback = callback;
    }

    // ���� character
    void setCharacter(const std::shared_ptr<GameObject>& character) {
        Character = character;
    }

private:
    // ���ڳߴ�
    unsigned int SCR_WIDTH;
    unsigned int SCR_HEIGHT;

    // GLFW����
    GLFWwindow* window;

    // �����ⲿϵͳ
    Camera& camera;
    LightManager& lightManager;
    ShadowManager& shadowManager;
    Scene& scene;

    // ��ɫ��
    Shader lightingShader;
    Shader shadowShader;
    Shader pointshadowShader;

    // ���� character ������ָ��
    std::shared_ptr<GameObject> Character; 

    // ����
    PostProcessing postProcessing;

    // ��ӽ�ͼ/¼�ƹ�����
    std::unique_ptr<CaptureManager> captureManager;

    // ʱ�����
    float deltaTime;
    float lastFrame;

    // ������
    bool mouseCaptured;
    float lastX;
    float lastY;
    bool firstMouse;

    // imgui�����
    bool sidebar_open = true;
    float sidebar_width = 400.0f;

    // ������ͼ
    bool debugLightView;
    int debugLightIndex;
    bool debugMaterialView;
    int debugMaterialIndex;


    // ��ȡ�����б�
    std::vector<std::string> getSceneFiles(const std::string& directory);

    // ��Ϸ�߼��ص�
    std::function<void()> gameLogicCallback;

    // ��ʼ��ImGui
    void initImGui();

    // ����OpenGL״̬
    void configureOpenGL();

    // ���ûص�����
    void setCallbacks();

    // ��Ⱦһ֡
    void renderFrame();

    // ��������
    void processInput();

    // ������Ӱ��ͼ
    void updateShadowMaps();

    // ����ͼ��س���
    void saveScene(const std::string& filePath);
    void loadScene(const std::string& filePath);

    static char saveFileName[128]; // Ĭ���ļ���
    static std::vector<std::string> availableScenes; // ���ó����ļ��б�
    static int selectedSceneIndex; // ��ǰѡ�еĳ�������

    // ��̬�ص����� - ���ڴ�С�仯
    static void framebuffer_size_callback(GLFWwindow* window, int width, int height);

    // ��̬�ص����� - ����ƶ�
    static void mouse_callback(GLFWwindow* window, double xposIn, double yposIn);

    // ��̬�ص����� - ������
    static void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
};

#endif // RENDERER_H
