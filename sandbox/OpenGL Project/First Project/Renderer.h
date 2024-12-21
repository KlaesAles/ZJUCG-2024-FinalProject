// Renderer.h
#ifndef RENDERER_H
#define RENDERER_H

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "Shader.h"
#include "Camera.h"
#include "LightManager.h"
#include "ShadowManager.h"
#include "GameObject.h"

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
        LightManager& lightManager, ShadowManager& shadowManager,
        std::vector<GameObject>& sceneObjects);

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
    std::vector<GameObject>& sceneObjects;

    // ��ɫ��
    Shader lightingShader;
    Shader shadowShader;

    // ʱ�����
    float deltaTime;
    float lastFrame;

    // ������
    bool mouseCaptured;
    float lastX;
    float lastY;
    bool firstMouse;

    // ������ͼ
    bool debugLightView;
    int debugLightIndex;
    bool debugMaterialView;
    int debugMaterialIndex;

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

    // ��̬�ص����� - ���ڴ�С�仯
    static void framebuffer_size_callback(GLFWwindow* window, int width, int height);

    // ��̬�ص����� - ����ƶ�
    static void mouse_callback(GLFWwindow* window, double xposIn, double yposIn);

    // ��̬�ص����� - ������
    static void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
};

#endif // RENDERER_H
