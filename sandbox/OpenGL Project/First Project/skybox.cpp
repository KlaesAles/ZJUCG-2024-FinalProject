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

    unsigned int tid;
    glGenTextures(1, &tid);
    glBindTexture(GL_TEXTURE_CUBE_MAP, tid);

    // 设置纹理参数
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    int width, height, nrChannels;
    stbi_set_flip_vertically_on_load(true);  // 翻转图像
    unsigned char* image = stbi_load(panoramaPath.c_str(), &width, &height, &nrChannels, 0);
    
    if (!image) {
        std::cerr << "Failed to load panorama: " << stbi_failure_reason() << std::endl;
        glDeleteTextures(1, &tid);
        throw std::runtime_error("Failed to load panorama texture at path: " + panoramaPath);
    }

    std::cout << "Loaded panorama: " << width << "x" << height << " channels: " << nrChannels << std::endl;

    // 计算合适的纹理尺寸（必须是2的幂）
    int maxSize;
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxSize);
    int texSize = 1024; // 默认尺寸
    while (texSize * 2 <= maxSize && texSize * 2 <= width && texSize * 2 <= height) {
        texSize *= 2;
    }
    std::cout << "Using texture size: " << texSize << "x" << texSize << std::endl;

    // 分配临时缓冲区用于调整大小
    unsigned char* resizedImage = new unsigned char[texSize * texSize * nrChannels];
    
    // 简单的最近邻采样来调整图像大小
    for (int y = 0; y < texSize; y++) {
        for (int x = 0; x < texSize; x++) {
            int srcX = x * width / texSize;
            int srcY = y * height / texSize;
            for (int c = 0; c < nrChannels; c++) {
                resizedImage[(y * texSize + x) * nrChannels + c] = 
                    image[(srcY * width + srcX) * nrChannels + c];
            }
        }
    }

    // 这里需要将全景图转换为立方体贴图
    // TODO: 实现全景图到立方体贴图的转换
    // 临时代码：将相同的图像应用到所有六个面
    GLenum format = (nrChannels == 4) ? GL_RGBA : GL_RGB;
    GLenum internalFormat = (nrChannels == 4) ? GL_RGBA8 : GL_RGB8;
    
    // 为每个面分配空间并上传纹理数据
    for (unsigned int i = 0; i < 6; ++i) {
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, internalFormat, texSize, texSize, 0,
            format, GL_UNSIGNED_BYTE, resizedImage);
        GLenum error = glGetError();
        if (error != GL_NO_ERROR) {
            std::cerr << "OpenGL error when loading face " << i << ": " << error << std::endl;
        }
    }
    
    // 生成 mipmap
    glGenerateMipmap(GL_TEXTURE_CUBE_MAP);
    
    delete[] resizedImage;
    stbi_image_free(image);
    
    std::cout << "Cubemap texture generated with ID: " << tid << std::endl;
    return tid;
}
