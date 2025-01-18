// Renderer.h
#ifndef RENDERER_H
#define RENDERER_H

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#define RPC_NO_WINDOWS_H
#include <SDKDDKVer.h>
#include <Windows.h>

// ȷ���ڰ�������ͷ�ļ�֮ǰȡ����Щ�궨��
#undef byte
#undef FAR
#undef near

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "Shader.h"
#include "Camera.h"
#include "LightManager.h"
#include "ShadowManager.h"
#include "Scene.h"
#include "PostProcessing.h"
#include "CaptureManager.h"
#include "skybox.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include <vector>
#include <memory>
#include <functional>
#include <string>

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

    // �����ر���
    bool mouseCaptured;
    float lastX, lastY;
    bool firstMouse;
    bool mouseLeftButtonDown;  // �������״̬
    std::shared_ptr<GameObject> selectedObject;  // ��ǰѡ�е�����
    glm::vec3 dragOffset;  // �϶�ƫ����

    // imgui�����
    bool sidebar_open = true;
    float sidebar_width = 400.0f;

    // ������ͼ
    bool debugLightView;
    int debugLightIndex;
    bool debugMaterialView;
    int debugMaterialIndex;

    // ��Χ����ʾ���
    bool showBoundingSpheres = false;  // ���ư�Χ����ʾ
    void drawBoundingSphere(const std::shared_ptr<GameObject>& obj);  // ���ư�Χ��

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

    // ����Ϊ OBJ �ļ�
    bool exportToObj(const std::shared_ptr<GameObject>& obj, const std::string& filePath);

    static char saveFileName[128]; // Ĭ���ļ���
    static std::vector<std::string> availableScenes; // ���ó����ļ��б�
    static int selectedSceneIndex; // ��ǰѡ�еĳ�������

    // ��̬�ص����� - ���ڴ�С�仯
    static void framebuffer_size_callback(GLFWwindow* window, int width, int height);

    // ��̬�ص����� - ����ƶ�
    static void mouse_callback(GLFWwindow* window, double xposIn, double yposIn);

    // ��̬�ص����� - ������
    static void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);

    // ��̬�ص����� - ��갴ť
    static void mouse_button_callback(GLFWwindow* window, int button, int action, int mods);

    // ����ʰȡ��غ���
    glm::vec3 screenToWorldRay(float mouseX, float mouseY);
    std::shared_ptr<GameObject> pickObject(const glm::vec3& rayOrigin, const glm::vec3& rayDir);

    // ������ɫ������������ĳ�Ա����
    std::unique_ptr<Shader> basicShader;  // ���ڻ��ƻ���ͼ�ε���ɫ��
    std::shared_ptr<Mesh> sphereMesh;     // ���ڻ��ư�Χ�������

    // ������������ĸ�������
    std::shared_ptr<Mesh> createSphereMesh(float radius, int segments, int rings);

    // ��պ����
    std::unique_ptr<Shader> skyboxShader;  // ��պ���ɫ��
    std::unique_ptr<Skybox> skybox;        // ��պж���
    bool enableSkybox = true;              // �Ƿ�������պ�
    std::vector<std::string> skyboxPaths;  // ��պ���ͼ·��
    bool isPanorama = false;               // �Ƿ�ʹ��ȫ��ͼ

    // ������պ�
    void loadSkybox(const std::vector<std::string>& paths);
    // ��ȫ��ͼ������պ�
    void loadSkyboxFromPanorama(const std::string& panoramaPath);
    // ��ȡĬ����պ�·��
    std::vector<std::string> getDefaultSkyboxPaths() const;
    // ��ȡ���õ���պ��б�
    std::vector<std::string> getSkyboxList(const std::string& directory) const;
    // ��ȡ���õ�ȫ��ͼ�б�
    std::vector<std::string> getPanoramaList(const std::string& directory) const;
};

#endif // RENDERER_H
