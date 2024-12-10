#ifndef GAME_OBJECT_H
#define GAME_OBJECT_H

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glad/glad.h>

#include "model.h"
#include "shader.h"

class GameObject {
private:
    glm::mat4 modelMatrix; // 模型矩阵，用于物体的变换
    Model model;           // Model 类的实例，用于管理和渲染模型

    glm::vec3 position;    // 位置
    glm::vec3 scale;       // 缩放
    glm::vec3 rotation;    // 旋转角度（以度为单位）

    void updateModelMatrix() {
        modelMatrix = glm::mat4(1.0f);
        modelMatrix = glm::translate(modelMatrix, position);
        modelMatrix = glm::scale(modelMatrix, scale);
        modelMatrix = glm::rotate(modelMatrix, glm::radians(rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
        modelMatrix = glm::rotate(modelMatrix, glm::radians(rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
        modelMatrix = glm::rotate(modelMatrix, glm::radians(rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));
    }

public:
    // 构造函数：初始化模型矩阵并加载模型
    GameObject(const std::string& modelPath,
        glm::vec3 position = glm::vec3(0.0f),
        glm::vec3 scale = glm::vec3(1.0f),
        glm::vec3 rotation = glm::vec3(0.0f),
        bool gamma = false)
        : model(modelPath, gamma), position(position), scale(scale), rotation(rotation) {
        updateModelMatrix();
    }

    // 绘制方法
    void draw(GLuint shader) {
        // 使用传入的着色器程序
        glUseProgram(shader);

        // 将模型矩阵传递给着色器
        GLuint modelLoc = glGetUniformLocation(shader, "model");
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, &modelMatrix[0][0]);

        // 绘制模型
        model.Draw(shader);
    }

    // 设置位置
    void setPosition(const glm::vec3& newPosition) {
        position = newPosition;
        updateModelMatrix();
    }

    // 设置缩放
    void setScale(const glm::vec3& newScale) {
        scale = newScale;
        updateModelMatrix();
    }

    // 设置旋转
    void setRotation(const glm::vec3& newRotation) {
        rotation = newRotation;
        updateModelMatrix();
    }

    // 获取模型矩阵
    const glm::mat4& getModelMatrix() const {
        return modelMatrix;
    }
};

#endif // GAME_OBJECT_H
