#ifndef GAME_OBJECT_H
#define GAME_OBJECT_H

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glad/glad.h>

#include "model.h"
#include "shader.h"
#include "BoundingBox.h"

class GameObject {
private:
    glm::mat4 modelMatrix;    // 模型矩阵，用于物体的变换
    Model model;              // Model 类的实例，用于管理和渲染模型
    BoundingBox boundingBox;  // 包围盒，表示物体的边界

    glm::vec3 position;       // 位置
    glm::vec3 scale;          // 缩放
    glm::vec3 rotation;       // 旋转角度（以度为单位）

    // 更新模型矩阵
    void updateModelMatrix() {
        modelMatrix = glm::mat4(1.0f);
        modelMatrix = glm::translate(modelMatrix, position);
        modelMatrix = glm::scale(modelMatrix, scale);
        modelMatrix = glm::rotate(modelMatrix, glm::radians(rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
        modelMatrix = glm::rotate(modelMatrix, glm::radians(rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
        modelMatrix = glm::rotate(modelMatrix, glm::radians(rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));
        updateBoundingBox(); // 同步更新包围盒
    }

    // 更新包围盒的位置和大小
    void updateBoundingBox() {
        glm::vec3 modelMin = model.boundingBox.min;
        glm::vec3 modelMax = model.boundingBox.max;

        glm::vec3 scaledMin = modelMin * scale + position;
        glm::vec3 scaledMax = modelMax * scale + position;

        boundingBox.min = glm::min(scaledMin, scaledMax);
        boundingBox.max = glm::max(scaledMin, scaledMax);
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

    // 获取包围盒
    const BoundingBox& getBoundingBox() const {
        return boundingBox;
    }

    // 获取包围盒
    const glm::vec3& getPosition() const {
        return position;
    }
};

#endif // GAME_OBJECT_H
