#ifndef ANIMATOR_H
#define ANIMATOR_H
#define GLM_ENABLE_EXPERIMENTAL

#include <map>
#include <string>
#include <stdexcept>
#include <vector>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/string_cast.hpp>
#include <glad/glad.h>
#include "Animation.h"
#include "Model.h"
#include "shader.h"

class Animator {
private:
    std::map<std::string, Animation> animations;
    Animation* currentAnimation = nullptr;
    float currentTime = 0.0f;
    bool isPlaying = false;
    Model* model = nullptr;

public:
    // 默认构造函数
    Animator() : model(nullptr) {}

    Animator(Model* modelPtr) : model(modelPtr) {}

    void addAnimation(const Animation& animation) {
        animations[animation.getName()] = animation;
    }

    void playAnimation(const std::string& name) {
        if (animations.find(name) != animations.end()) {
            currentAnimation = &animations[name];
            currentTime = 0.0f;
            isPlaying = true;
        }
        else {
            throw std::runtime_error("Animation not found: " + name);
        }
    }

    void stopAnimation() {
        currentAnimation = nullptr;
        currentTime = 0.0f;
        isPlaying = false;
    }

    void update(float deltaTime, GLuint shaderProgramID) {
        if (!model) return;

        // 骨骼矩阵数量
        size_t boneCount = model->numBones;

        // 如果没有活动动画，则初始化所有骨骼为单位矩阵并上传
        if (!isPlaying || !currentAnimation) {
            std::vector<glm::mat4> identityMatrices(boneCount, glm::mat4(1.0f));
            uploadBoneMatrices(shaderProgramID, identityMatrices);
            return;
        }

        // 更新当前时间
        currentTime += deltaTime * currentAnimation->getTicksPerSecond();
        if (currentTime > currentAnimation->getDuration()) {
            currentTime = fmod(currentTime, currentAnimation->getDuration());
        }

        // 计算局部骨骼变换
        auto boneLocalTransforms = currentAnimation->evaluate(currentTime, model->boneInfoMap);

        // 骨骼全局变换递归计算
        std::map<std::string, glm::mat4> globalTransforms;
        for (const auto& [boneName, boneIdx] : model->boneMapping) {
            computeGlobalTransform(boneName, globalTransforms, boneLocalTransforms);
        }

        // 更新骨骼最终变换矩阵
        std::vector<glm::mat4> finalBoneMatrices(boneCount, glm::mat4(1.0f));
        for (const auto& [boneName, boneIdx] : model->boneMapping) {
            finalBoneMatrices[boneIdx] = model->boneInfoMap[boneName].finalTransform;
        }

        /*
        for (const auto& [boneName, globalMatrix] : globalTransforms) {
            std::cout << "Bone: " << boneName << ", Global Transform: " << glm::to_string(globalMatrix) << std::endl;
        }
        */

        // 上传骨骼矩阵到着色器
        uploadBoneMatrices(shaderProgramID, finalBoneMatrices);
    }

    void computeGlobalTransform(
        const std::string& boneName,
        std::map<std::string, glm::mat4>& globalTransforms,
        const std::map<std::string, glm::mat4>& boneLocalTransforms
    ) {
        if (globalTransforms.find(boneName) != globalTransforms.end()) {
            return; // 已经计算过全局变换
        }

        glm::mat4 localTransform = glm::mat4(1.0f);
        if (boneLocalTransforms.find(boneName) != boneLocalTransforms.end()) {
            localTransform = boneLocalTransforms.at(boneName);
        }

        // 递归获取父级的全局变换
        glm::mat4 parentGlobalTransform = glm::mat4(1.0f);
        if (model->boneParentMap.find(boneName) != model->boneParentMap.end()) {
            const std::string& parentName = model->boneParentMap.at(boneName);
            computeGlobalTransform(parentName, globalTransforms, boneLocalTransforms);
            parentGlobalTransform = globalTransforms[parentName];
        }

        // 计算当前骨骼的全局变换
        glm::mat4 globalTransform = parentGlobalTransform * localTransform;
        globalTransforms[boneName] = globalTransform;

        // 应用 offsetMatrix
        model->boneInfoMap[boneName].finalTransform =
            globalTransform * model->boneInfoMap[boneName].offsetMatrix;
    }

    void uploadBoneMatrices(GLuint shaderProgramID, const std::vector<glm::mat4>& matrices) {
        glUseProgram(shaderProgramID);
        GLint boneLoc = glGetUniformLocation(shaderProgramID, "bones");
        if (boneLoc != -1) {
            glUniformMatrix4fv(boneLoc, static_cast<GLsizei>(matrices.size()), GL_FALSE, glm::value_ptr(matrices[0]));
        }
    }

    const Animation* getCurrentAnimation() const {
        return currentAnimation;
    }

    bool isAnimationPlaying() const {
        return isPlaying;
    }
};

#endif // ANIMATOR_H
