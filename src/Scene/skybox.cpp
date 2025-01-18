#include "skybox.h"

// 顶点数据
float skyboxVertices[] = {
    // positions          
    -1.0f,  1.0f, -1.0f,
    -1.0f, -1.0f, -1.0f,
     1.0f, -1.0f, -1.0f,
     1.0f, -1.0f, -1.0f,
     1.0f,  1.0f, -1.0f,
    -1.0f,  1.0f, -1.0f,

    -1.0f, -1.0f,  1.0f,
    -1.0f, -1.0f, -1.0f,
    -1.0f,  1.0f, -1.0f,
    -1.0f,  1.0f, -1.0f,
    -1.0f,  1.0f,  1.0f,
    -1.0f, -1.0f,  1.0f,

     1.0f, -1.0f, -1.0f,
     1.0f, -1.0f,  1.0f,
     1.0f,  1.0f,  1.0f,
     1.0f,  1.0f,  1.0f,
     1.0f,  1.0f, -1.0f,
     1.0f, -1.0f, -1.0f,

    -1.0f, -1.0f,  1.0f,
    -1.0f,  1.0f,  1.0f,
     1.0f,  1.0f,  1.0f,
     1.0f,  1.0f,  1.0f,
     1.0f, -1.0f,  1.0f,
    -1.0f, -1.0f,  1.0f,

    -1.0f,  1.0f, -1.0f,
     1.0f,  1.0f, -1.0f,
     1.0f,  1.0f,  1.0f,
     1.0f,  1.0f,  1.0f,
    -1.0f,  1.0f,  1.0f,
    -1.0f,  1.0f, -1.0f,

    -1.0f, -1.0f, -1.0f,
    -1.0f, -1.0f,  1.0f,
     1.0f, -1.0f, -1.0f,
     1.0f, -1.0f, -1.0f,
    -1.0f, -1.0f,  1.0f,
     1.0f, -1.0f,  1.0f
};

Skybox::Skybox(const std::vector<std::string>& paths)
{
    if (paths.size() != 6) {
        throw std::runtime_error("Skybox constructor requires exactly 6 texture paths");
    }
    texture = GenCubeMap(paths);

    glGenBuffers(1, &VBO); 
    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), skyboxVertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
}

Skybox::Skybox(const std::string& panoramaPath)
{
    if (panoramaPath.empty()) {
        throw std::runtime_error("Panorama path cannot be empty");
    }
    texture = GenCubeMapFromPanorama(panoramaPath);

    glGenBuffers(1, &VBO); 
    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), skyboxVertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
}

Skybox::~Skybox()
{
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteTextures(1, &texture);
}

void Skybox::Draw(Shader& shader, glm::mat4 model, glm::mat4 view, glm::mat4 projection)
{
    glm::mat4 skyboxView = glm::mat4(glm::mat3(view));
    shader.use();
    shader.setMat4("model", model);
    shader.setMat4("view", view);
    shader.setMat4("projection", projection);
    shader.setInt("skybox", 0);
    glDepthMask(GL_FALSE);
    glBindVertexArray(VAO);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, texture);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glDepthMask(GL_TRUE);
}

unsigned int Skybox::GenCubeMap(const std::vector<std::string>& facePaths)
{
    if (facePaths.size() != 6) {
        throw std::runtime_error("GenCubeMap requires exactly 6 texture paths");
    }

    unsigned int tid;
    glGenTextures(1, &tid);
    glBindTexture(GL_TEXTURE_CUBE_MAP, tid);

    int width, height, nrChannels;
    stbi_set_flip_vertically_on_load(false);
    
    bool loadError = false;
    std::string errorPath;
    
    for (unsigned int i = 0; i < facePaths.size(); ++i)
    {
        unsigned char* image = stbi_load(facePaths[i].c_str(), &width, &height, &nrChannels, 0);
        if (image)
        {
            GLenum format = (nrChannels == 4) ? GL_RGBA : GL_RGB;
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, format, width, height, 0,
                format, GL_UNSIGNED_BYTE, image);
            stbi_image_free(image);
        }
        else
        {
            loadError = true;
            errorPath = facePaths[i];
            break;
        }
    }

    if (loadError) {
        glDeleteTextures(1, &tid);
        throw std::runtime_error("Failed to load cubemap texture at path: " + errorPath);
    }

    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    return tid;
}

unsigned int Skybox::GenCubeMapFromPanorama(const std::string& panoramaPath)
{
    std::cout << "Loading panorama from: " << panoramaPath << std::endl;

    // 加载全景图
    int width, height, nrChannels;
    stbi_set_flip_vertically_on_load(true);
    unsigned char* image = stbi_load(panoramaPath.c_str(), &width, &height, &nrChannels, 0);
    
    if (!image) {
        std::cerr << "Failed to load panorama: " << stbi_failure_reason() << std::endl;
        throw std::runtime_error("Failed to load panorama texture at path: " + panoramaPath);
    }

    std::cout << "Loaded panorama: " << width << "x" << height << " channels: " << nrChannels << std::endl;

    // 创建全景图纹理
    unsigned int panoramaTexture;
    glGenTextures(1, &panoramaTexture);
    glBindTexture(GL_TEXTURE_2D, panoramaTexture);
    GLenum format = (nrChannels == 4) ? GL_RGBA : GL_RGB;
    GLenum internalFormat = (nrChannels == 4) ? GL_RGBA16F : GL_RGB16F;
    glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, format, GL_UNSIGNED_BYTE, image);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    stbi_image_free(image);

    // 创建立方体贴图
    unsigned int cubemapTexture;
    glGenTextures(1, &cubemapTexture);
    glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture);
    int cubemapSize = 2048; // 增加分辨率
    for (unsigned int i = 0; i < 6; ++i) {
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F, 
                    cubemapSize, cubemapSize, 0, GL_RGB, GL_FLOAT, nullptr);
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // 创建并配置帧缓冲对象
    unsigned int captureFBO, captureRBO;
    glGenFramebuffers(1, &captureFBO);
    glGenRenderbuffers(1, &captureRBO);
    glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
    glBindRenderbuffer(GL_RENDERBUFFER, captureRBO);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, cubemapSize, cubemapSize);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, captureRBO);

    // 加载并配置全景图转换着色器
    Shader equirectangularToCubemapShader("./shaders/panorama_to_cubemap.vs", "./shaders/panorama_to_cubemap.fs");
    equirectangularToCubemapShader.use();
    equirectangularToCubemapShader.setInt("panorama", 0);

    // 设置投影矩阵
    glm::mat4 captureProjection = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);
    equirectangularToCubemapShader.setMat4("projection", captureProjection);

    // 设置6个面的观察矩阵
    glm::mat4 captureViews[] = {
        glm::lookAt(glm::vec3(0.0f), glm::vec3( 1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
        glm::lookAt(glm::vec3(0.0f), glm::vec3(-1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
        glm::lookAt(glm::vec3(0.0f), glm::vec3( 0.0f,  1.0f,  0.0f), glm::vec3(0.0f,  0.0f,  1.0f)),
        glm::lookAt(glm::vec3(0.0f), glm::vec3( 0.0f, -1.0f,  0.0f), glm::vec3(0.0f,  0.0f, -1.0f)),
        glm::lookAt(glm::vec3(0.0f), glm::vec3( 0.0f,  0.0f,  1.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
        glm::lookAt(glm::vec3(0.0f), glm::vec3( 0.0f,  0.0f, -1.0f), glm::vec3(0.0f, -1.0f,  0.0f))
    };

    // 转换全景图到立方体贴图
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, panoramaTexture);
    glViewport(0, 0, cubemapSize, cubemapSize);
    glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
    
    for (unsigned int i = 0; i < 6; ++i) {
        equirectangularToCubemapShader.setMat4("view", captureViews[i]);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, 
                              GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, cubemapTexture, 0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        renderCube(); // 渲染一个1x1的立方体
    }

    // 生成 mipmap
    glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture);
    glGenerateMipmap(GL_TEXTURE_CUBE_MAP);

    // 清理资源
    glDeleteTextures(1, &panoramaTexture);
    glDeleteFramebuffers(1, &captureFBO);
    glDeleteRenderbuffers(1, &captureRBO);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    return cubemapTexture;
}

// 渲染一个1x1的立方体
void Skybox::renderCube()
{
    static unsigned int cubeVAO = 0;
    static unsigned int cubeVBO = 0;
    
    if (cubeVAO == 0) {
        float vertices[] = {
            // back face
            -1.0f, -1.0f, -1.0f,  // bottom-left
             1.0f,  1.0f, -1.0f,  // top-right
             1.0f, -1.0f, -1.0f,  // bottom-right         
             1.0f,  1.0f, -1.0f,  // top-right
            -1.0f, -1.0f, -1.0f,  // bottom-left
            -1.0f,  1.0f, -1.0f,  // top-left
            // front face
            -1.0f, -1.0f,  1.0f,  // bottom-left
             1.0f, -1.0f,  1.0f,  // bottom-right
             1.0f,  1.0f,  1.0f,  // top-right
             1.0f,  1.0f,  1.0f,  // top-right
            -1.0f,  1.0f,  1.0f,  // top-left
            -1.0f, -1.0f,  1.0f,  // bottom-left
            // left face
            -1.0f,  1.0f,  1.0f,  // top-right
            -1.0f,  1.0f, -1.0f,  // top-left
            -1.0f, -1.0f, -1.0f,  // bottom-left
            -1.0f, -1.0f, -1.0f,  // bottom-left
            -1.0f, -1.0f,  1.0f,  // bottom-right
            -1.0f,  1.0f,  1.0f,  // top-right
            // right face
             1.0f,  1.0f,  1.0f,  // top-left
             1.0f, -1.0f, -1.0f,  // bottom-right
             1.0f,  1.0f, -1.0f,  // top-right         
             1.0f, -1.0f, -1.0f,  // bottom-right
             1.0f,  1.0f,  1.0f,  // top-left
             1.0f, -1.0f,  1.0f,  // bottom-left     
            // bottom face
            -1.0f, -1.0f, -1.0f,  // top-right
             1.0f, -1.0f, -1.0f,  // top-left
             1.0f, -1.0f,  1.0f,  // bottom-left
             1.0f, -1.0f,  1.0f,  // bottom-left
            -1.0f, -1.0f,  1.0f,  // bottom-right
            -1.0f, -1.0f, -1.0f,  // top-right
            // top face
            -1.0f,  1.0f, -1.0f,  // top-left
             1.0f,  1.0f , 1.0f,  // bottom-right
             1.0f,  1.0f, -1.0f,  // top-right     
             1.0f,  1.0f,  1.0f,  // bottom-right
            -1.0f,  1.0f, -1.0f,  // top-left
            -1.0f,  1.0f,  1.0f   // bottom-left        
        };

        glGenVertexArrays(1, &cubeVAO);
        glGenBuffers(1, &cubeVBO);
        glBindVertexArray(cubeVAO);
        glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    }

    glBindVertexArray(cubeVAO);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindVertexArray(0);
}
