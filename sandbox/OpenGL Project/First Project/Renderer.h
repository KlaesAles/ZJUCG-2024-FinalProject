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
#include "Mesh.h" // Include Mesh.h

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
    // 构造函数
    Renderer(GLFWwindow* window, unsigned int width, unsigned int height, Camera& camera,
        LightManager& lightManager, ShadowManager& shadowManager, Scene& scene);

    // 初始化渲染器
    bool initialize();

    // 运行渲染循环
    void run();

    // 清理资源
    void cleanup();

    // 设置游戏逻辑回调
    void setGameLogicCallback(std::function<void()> callback) {
        gameLogicCallback = callback;
    }

    // 设置 character
    void setCharacter(const std::shared_ptr<GameObject>& character) {
        Character = character;
    }

private:
    // 窗口尺寸
    unsigned int SCR_WIDTH;
    unsigned int SCR_HEIGHT;

    // GLFW窗口
    GLFWwindow* window;

    // 引用外部系统
    Camera& camera;
    LightManager& lightManager;
    ShadowManager& shadowManager;
    Scene& scene;

    // 着色器
    Shader lightingShader;
    Shader shadowShader;
    Shader pointshadowShader;

    // 持有 character 的智能指针
    std::shared_ptr<GameObject> Character; 

    // 后处理
    PostProcessing postProcessing;

    // 添加截图/录制管理器
    std::unique_ptr<CaptureManager> captureManager;

    // 时间管理
    float deltaTime;
    float lastFrame;

    // 鼠标相关变量
    bool mouseCaptured;
    float lastX, lastY;
    bool firstMouse;
    bool mouseLeftButtonDown;  // 左键按下状态
    std::shared_ptr<GameObject> selectedObject;  // 当前选中的物体
    glm::vec3 dragOffset;  // 拖动偏移量

    // imgui侧边栏
    bool sidebar_open = true;
    float sidebar_width = 400.0f;

    // 调试视图
    bool debugLightView;
    int debugLightIndex;
    bool debugMaterialView;
    int debugMaterialIndex;

    // 包围球显示相关
    bool showBoundingSpheres = false;  // 控制包围球显示
    void drawBoundingSphere(const std::shared_ptr<GameObject>& obj);  // 绘制包围球

    // 获取场景列表
    std::vector<std::string> getSceneFiles(const std::string& directory);

    // 游戏逻辑回调
    std::function<void()> gameLogicCallback;

    // 初始化ImGui
    void initImGui();

    // 配置OpenGL状态
    void configureOpenGL();

    // 设置回调函数
    void setCallbacks();

    // 渲染一帧
    void renderFrame();

    // 处理输入
    void processInput();

    // 更新阴影贴图
    void updateShadowMaps();

    // 保存和加载场景
    void saveScene(const std::string& filePath);
    void loadScene(const std::string& filePath);

    static char saveFileName[128]; // 默认文件名
    static std::vector<std::string> availableScenes; // 可用场景文件列表
    static int selectedSceneIndex; // 当前选中的场景索引

    // 静态回调函数 - 窗口大小变化
    static void framebuffer_size_callback(GLFWwindow* window, int width, int height);

    // 静态回调函数 - 鼠标移动
    static void mouse_callback(GLFWwindow* window, double xpos, double ypos);

    // 静态回调函数 - 鼠标滚轮
    static void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);

    // 静态回调函数 - 鼠标按钮
    static void mouse_button_callback(GLFWwindow* window, int button, int action, int mods);

    // 射线拾取相关函数
    glm::vec3 screenToWorldRay(float mouseX, float mouseY);
    std::shared_ptr<GameObject> pickObject(const glm::vec3& rayOrigin, const glm::vec3& rayDir);

    // 基础着色器和球体网格的成员变量
    std::unique_ptr<Shader> basicShader;  // 用于绘制基础图形的着色器
    std::shared_ptr<Mesh> sphereMesh;     // 用于绘制包围球的网格

    // 创建球体网格的辅助函数
    std::shared_ptr<Mesh> createSphereMesh(float radius, int segments, int rings);
};

#endif // RENDERER_H
