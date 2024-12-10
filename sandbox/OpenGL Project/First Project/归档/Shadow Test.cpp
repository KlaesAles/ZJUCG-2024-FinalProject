#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <shader.h>
#include <camera.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include <Shadow/ShadowManager.h>
#include <Light.h>
#include <LightManager.h>

#include <iostream>
#include <vector>

// Function declarations
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow* window);
unsigned int loadTexture(char const* path);

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

// lighting
glm::vec3 lightPos(1.2f, 1.0f, 2.0f);

// set up vertex data (and buffer(s)) and configure vertex attributes
// ------------------------------------------------------------------
float planeVertices[] = {
    // Positions          // Normals         // Texture Coords
        25.0f, -2.0f, 25.0f, 0.0f, 1.0f, 0.0f, 25.0f, 0.0f,
        -25.0f, -2.0f, -25.0f, 0.0f, 1.0f, 0.0f, 0.0f, 25.0f,
        -25.0f, -2.0f, 25.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f,

        25.0f, -2.0f, 25.0f, 0.0f, 1.0f, 0.0f, 25.0f, 0.0f,
        25.0f, -2.0f, -25.0f, 0.0f, 1.0f, 0.0f, 25.0f, 25.0f,
        -25.0f, -2.0f, -25.0f, 0.0f, 1.0f, 0.0f, 0.0f, 25.0f
};

float cubeVertices[] = {
    // positions          // normals           // texture coords
    -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f,  0.0f,
     0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f,  0.0f,
     0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f,  1.0f,
     0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f,  1.0f,
    -0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f,  1.0f,
    -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f,  0.0f,

    -0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f,  0.0f,
     0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f,  0.0f,
     0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f,  1.0f,
     0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f,  1.0f,
    -0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f,  1.0f,
    -0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f,  0.0f,

    -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  1.0f,  0.0f,
    -0.5f,  0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  1.0f,  1.0f,
    -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  0.0f,  1.0f,
    -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  0.0f,  1.0f,
    -0.5f, -0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  0.0f,  0.0f,
    -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  1.0f,  0.0f,

     0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  1.0f,  0.0f,
     0.5f,  0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  1.0f,  1.0f,
     0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  0.0f,  1.0f,
     0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  0.0f,  1.0f,
     0.5f, -0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  0.0f,  0.0f,
     0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  1.0f,  0.0f,

    -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  0.0f,  1.0f,
     0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  1.0f,  1.0f,
     0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  1.0f,  0.0f,
     0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  1.0f,  0.0f,
    -0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  0.0f,  0.0f,
    -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  0.0f,  1.0f,

    -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  0.0f,  1.0f,
     0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  1.0f,  1.0f,
     0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  1.0f,  0.0f,
     0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  1.0f,  0.0f,
    -0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  0.0f,  0.0f,
    -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  0.0f,  1.0f
};
// positions all containers
glm::vec3 cubePositions[] = {
    glm::vec3(4.0f, -3.5f, 0.0),
    glm::vec3(2.0f, 3.0f, 1.0),
    glm::vec3(-3.0f, -1.0f, 0.0),
    glm::vec3(-1.5f, 1.0f, 1.5),
    glm::vec3(-1.5f, 2.0f, -3.0),
};
glm::vec3 cubeScale[] = {
    glm::vec3(1.0f),
    glm::vec3(1.5f),
    glm::vec3(1.0f),
    glm::vec3(1.0f),
    glm::vec3(1.5f),
};
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
    Shader lightingShader("./shader/Shadow Test.vs", "./shader/Shadow Test.fs");
    Shader lightCubeShader("./shader/light_cube.vs", "./shader/light_cube.fs");
    Shader ShadowShader("./Shadow/shadow.vs", "./Shadow/shadow.fs");

    // first of first, bind the plane's VAO and VBO
    unsigned int planeVBO, planeVAO;
    glGenVertexArrays(1, &planeVAO);
    glGenBuffers(1, &planeVBO);

    glBindBuffer(GL_ARRAY_BUFFER, planeVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(planeVertices), planeVertices, GL_STATIC_DRAW);

    glBindVertexArray(planeVAO);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    // first, configure the cube's VAO (and VBO)
    unsigned int cubeVBO, cubeVAO;
    glGenVertexArrays(1, &cubeVAO);
    glGenBuffers(1, &cubeVBO);

    glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cubeVertices), cubeVertices, GL_STATIC_DRAW);

    glBindVertexArray(cubeVAO);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    // second, configure the light's VAO (VBO stays the same; the vertices are the same for the light object which is also a 3D cube)
    unsigned int lightCubeVAO;
    glGenVertexArrays(1, &lightCubeVAO);
    glBindVertexArray(lightCubeVAO);

    glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
    // note that we update the lamp's position attribute's stride to reflect the updated buffer data
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // load textures (we now use a utility function to keep the code more organized)
    // -----------------------------------------------------------------------------
    unsigned int diffuseMap = loadTexture("./assets/container2.png");
    unsigned int specularMap = loadTexture("./assets/container2_specular.png");

    // shader configuration
    // --------------------
    lightingShader.use();
    lightingShader.setInt("material.diffuse", 0);
    lightingShader.setInt("material.specular", 1);

	// Bind GameObjects
	// ---------------
    // 创建场景物体容器
    std::vector<GameObject> sceneObjects;

    // 初始化场景中的地面
    sceneObjects.emplace_back(planeVAO, 6, glm::vec3(0.0f), glm::vec3(1.0f), glm::vec3(0.0f));

    // 初始化场景中的立方体
    for (int i = 0; i < sizeof(cubePositions) / sizeof(cubePositions[1]); ++i ) {
        sceneObjects.emplace_back(cubeVAO, 36, cubePositions[i], cubeScale[i], glm::vec3(0.0f, 0.0, 0.0f));
    }

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
	auto dirLight = std::make_shared<DirectionalLight>(glm::vec3(2.0f, -4.0f, 1.0f), glm::vec3(1.0f));
    lightManager.addLight(dirLight);

    // 初始化聚光灯
    auto spotLight = std::make_shared<SpotLight>(camera.Position, camera.Front, glm::vec3(1.0f), 1.0f, 12.5f);
    lightManager.addLight(spotLight);

    // 创建阴影管理器
    ShadowManager shadowManager;

    // 将 shadowManager 传递给窗口
    glfwSetWindowUserPointer(window, &shadowManager);

    // shader configuration
    // --------------------
    lightingShader.use();
    lightingShader.setInt("material.diffuse", 0);
    lightingShader.setInt("material.specular", 1);

	bool debugView = 0;
    int debugLightIndex = 0;

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

        ImGui::Checkbox("Debug View", &debugView);
        if (debugView) {
            ImGui::SliderInt("debug Light Index", &debugLightIndex, 0, lightManager.getLightCount() - 1, "%d");
        }

        ImGui::End();

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
        lightingShader.setInt("debugView", debugView);
        lightingShader.setInt("debugLightIndex", debugLightIndex);

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

        // bind diffuse map
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, diffuseMap);
        // bind specular map
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, specularMap);


        // Render all GameObjects
        for (const auto& object : sceneObjects) {
            object.draw(lightingShader.ID); // 使用 GameObject 的 draw 方法
        }

        // Render light cubes
        lightCubeShader.use();
        lightCubeShader.setMat4("projection", projection);
        lightCubeShader.setMat4("view", view);
        for (unsigned int i = 0; i < sizeof(pointLightPositions) / sizeof(pointLightPositions[1]); i++) {
            GameObject lightCube(lightCubeVAO, 36, pointLightPositions[i], glm::vec3(0.2f));
            lightCube.draw(lightCubeShader.ID);
        }

        // Render ImGui
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
        // -------------------------------------------------------------------------------
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // Cleanup
    glDeleteVertexArrays(1, &cubeVAO);
    glDeleteVertexArrays(1, &lightCubeVAO);
    glDeleteVertexArrays(1, &planeVAO);
    glDeleteBuffers(1, &cubeVBO);
    glDeleteBuffers(1, &planeVBO);

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
// utility function for loading a 2D texture from file
// ---------------------------------------------------
unsigned int loadTexture(char const* path)
{
	unsigned int textureID;
	glGenTextures(1, &textureID);

	int width, height, nrComponents;
	unsigned char* data = stbi_load(path, &width, &height, &nrComponents, 0);
	if (data)
	{
		GLenum format;
		if (nrComponents == 1)
			format = GL_RED;
		else if (nrComponents == 3)
			format = GL_RGB;
		else if (nrComponents == 4)
			format = GL_RGBA;

		glBindTexture(GL_TEXTURE_2D, textureID);
		glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
		glGenerateMipmap(GL_TEXTURE_2D);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		stbi_image_free(data);
	}
	else
	{
		std::cout << "Texture failed to load at path: " << path << std::endl;
		stbi_image_free(data);
	}

	return textureID;
}