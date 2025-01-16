#ifndef GAME_OBJECT_H
#define GAME_OBJECT_H

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "Model.h"
#include "BoundingBox.h"
#include "Animator.h"

class GameObject {
private:
	std::string name;		   // 名称
    glm::mat4 modelMatrix;     // 模型矩阵，用于物体的变换
    Model model;               // 模型数据
    BoundingBox boundingBox;   // 包围盒

    glm::vec3 position;        // 位置
    glm::vec3 scale;           // 缩放
    glm::vec3 rotation;        // 旋转角度（以度为单位）

    Animator animator;

    // 更新模型矩阵
    void updateModelMatrix() {
        modelMatrix = glm::mat4(1.0f);
        modelMatrix = glm::translate(modelMatrix, position);
        modelMatrix = glm::scale(modelMatrix, scale);
        modelMatrix = glm::rotate(modelMatrix, glm::radians(rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
        modelMatrix = glm::rotate(modelMatrix, glm::radians(rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
        modelMatrix = glm::rotate(modelMatrix, glm::radians(rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));
        updateBoundingBox();
    }

    // 更新包围盒
    void updateBoundingBox() {
        glm::vec3 modelMin = model.boundingBox.min;
        glm::vec3 modelMax = model.boundingBox.max;

        glm::vec3 scaledMin = modelMin * scale + position;
        glm::vec3 scaledMax = modelMax * scale + position;

        boundingBox.min = glm::min(scaledMin, scaledMax);
        boundingBox.max = glm::max(scaledMin, scaledMax);
    }

public:
    // 构造函数
    GameObject(const std::string& name,
        const std::string& modelPath,
        const glm::vec3& position = glm::vec3(0.0f),
        const glm::vec3& scale = glm::vec3(1.0f),
        const glm::vec3& rotation = glm::vec3(0.0f),
        bool gamma = false)
        : name(name), model(modelPath, gamma), animator(&model), position(position), scale(scale), rotation(rotation) {
        updateModelMatrix();
        // 将 Model 中解析出的所有动画添加到 Animator
        for (const auto& anim : model.animations) {
            animator.addAnimation(anim);
        }
    }

    // 添加获取和设置 PBR 材质的函数
    PBRMaterial& getPBRMaterial(unsigned int meshIndex = 0) {
        // 假设每个 GameObject 只有一个 Mesh，如果有多个 Mesh，需要根据 meshIndex 获取
        return model.meshes[meshIndex].material;
    }

    // 获取名称
    const std::string& getName() const {
        return name;
    }

    // 获取模型
    const Model& getModel() const {
        return model;
    }

    // 获取模型矩阵
    const glm::mat4& getModelMatrix() const {
        return modelMatrix;
    }

    // 获取包围盒
    const BoundingBox& getBoundingBox() const {
        return boundingBox;
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

    // 添加动画
    void addAnimation(const Animation& animation) {
        animator.addAnimation(animation);
    }

    // 播放动画
    void playAnimation(const std::string& name) {
        animator.playAnimation(name);
    }

    // 停止动画
    void stopAnimation() {
        animator.stopAnimation();
    }

    // 更新物体逻辑（包括动画）
    void update(float deltaTime, Shader& shader) {

        animator.update(deltaTime, shader.ID); // 更新动画
    }

    // 上传骨骼动画相关的 uniform
    void uploadBoneUniforms(Shader& shader) {
        bool useBones = (model.numBones > 0);
        shader.use(); // 激活着色器
        shader.setInt("useBones", useBones ? 1 : 0);
        if (useBones) {
            animator.update(0.0f, shader.ID); // 直接传入 Shader 对象
        }
    }


    // 获取属性
    const glm::vec3& getPosition() const { return position; }
    const glm::vec3& getScale() const { return scale; }
    const glm::vec3& getRotation() const { return rotation; }
};

#endif // GAME_OBJECT_H
