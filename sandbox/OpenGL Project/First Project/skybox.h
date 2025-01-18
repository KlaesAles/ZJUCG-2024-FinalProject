#ifndef SKYBOX_H
#define SKYBOX_H

// Windows ��ض�����������ǰ��
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#define RPC_NO_WINDOWS_H
#include <SDKDDKVer.h>
#include <Windows.h>

// ȡ�� Windows �궨��
#undef near
#undef far

// OpenGL ���ͷ�ļ�
#include <glad/glad.h>

// GLM Math Library
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

// ��׼��
#include <iostream>
#include <vector>
#include <string>
#include <stdexcept>

// ��Ŀ���ͷ�ļ�
#include "Shader.h"
#include "stb_image.h"

// ǰ������
class Skybox;

// ��������
extern float skyboxVertices[];

class Skybox
{
public:
    // ������������ͼ�Ĺ��캯��
    explicit Skybox(const std::vector<std::string>& paths);
    // ����ȫ��ͼ�Ĺ��캯��
    explicit Skybox(const std::string& panoramaPath);
    ~Skybox();

    // ɾ���������캯���͸�ֵ�����
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
