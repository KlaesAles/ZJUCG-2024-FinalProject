#ifndef SKYBOX_H
#define SKYBOX_H

// Windows 相关定义必须放在最前面
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#define RPC_NO_WINDOWS_H
#include <SDKDDKVer.h>
#include <Windows.h>

// 取消 Windows 宏定义
#undef near
#undef far

// OpenGL 相关头文件
#include <glad/glad.h>

// GLM Math Library
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

// 标准库
#include <iostream>
#include <vector>
#include <string>
#include <stdexcept>

// 项目相关头文件
#include "Shader.h"
#include "stb_image.h"

// 前向声明
class Skybox;

// 顶点数据
extern float skyboxVertices[];

class Skybox
{
public:
    // 用于立方体贴图的构造函数
    explicit Skybox(const std::vector<std::string>& paths);
    // 用于全景图的构造函数
    explicit Skybox(const std::string& panoramaPath);
    ~Skybox();

    // 删除拷贝构造函数和赋值运算符
    Skybox(const Skybox&) = delete;
    Skybox& operator=(const Skybox&) = delete;

    void Draw(Shader& shader, glm::mat4 model, glm::mat4 view, glm::mat4 projection);
    unsigned int getCubeMap() const { return texture; }

private:
    unsigned int VAO{}, VBO{}, texture{};
    unsigned int GenCubeMap(const std::vector<std::string>& facePaths);
    unsigned int GenCubeMapFromPanorama(const std::string& panoramaPath);
};

#endif // !SKYBOX_H
