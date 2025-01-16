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
    // Ĭ�Ϲ��캯��
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

        // ������������
        size_t boneCount = model->numBones;

        // ���û�л���������ʼ�����й���Ϊ��λ�����ϴ�
        if (!isPlaying || !currentAnimation) {
            std::vector<glm::mat4> identityMatrices(boneCount, glm::mat4(1.0f));
            uploadBoneMatrices(shaderProgramID, identityMatrices);
            return;
        }

        // ���µ�ǰʱ��
        currentTime += deltaTime * currentAnimation->getTicksPerSecond();
        if (currentTime > currentAnimation->getDuration()) {
            currentTime = fmod(currentTime, currentAnimation->getDuration());
        }

        // ����ֲ������任
        auto boneLocalTransforms = currentAnimation->evaluate(currentTime, model->boneInfoMap);

        // ����ȫ�ֱ任�ݹ����
        std::map<std::string, glm::mat4> globalTransforms;
        for (const auto& [boneName, boneIdx] : model->boneMapping) {
            computeGlobalTransform(boneName, globalTransforms, boneLocalTransforms);
        }

        // ���¹������ձ任����
        std::vector<glm::mat4> finalBoneMatrices(boneCount, glm::mat4(1.0f));
        for (const auto& [boneName, boneIdx] : model->boneMapping) {
            finalBoneMatrices[boneIdx] = model->boneInfoMap[boneName].finalTransform;
        }

        /*
        for (const auto& [boneName, globalMatrix] : globalTransforms) {
            std::cout << "Bone: " << boneName << ", Global Transform: " << glm::to_string(globalMatrix) << std::endl;
        }
        */

        // �ϴ�����������ɫ��
        uploadBoneMatrices(shaderProgramID, finalBoneMatrices);
    }

    void computeGlobalTransform(
        const std::string& boneName,
        std::map<std::string, glm::mat4>& globalTransforms,
        const std::map<std::string, glm::mat4>& boneLocalTransforms
    ) {
        if (globalTransforms.find(boneName) != globalTransforms.end()) {
            return; // �Ѿ������ȫ�ֱ任
        }

        glm::mat4 localTransform = glm::mat4(1.0f);
        if (boneLocalTransforms.find(boneName) != boneLocalTransforms.end()) {
            localTransform = boneLocalTransforms.at(boneName);
        }

        // �ݹ��ȡ������ȫ�ֱ任
        glm::mat4 parentGlobalTransform = glm::mat4(1.0f);
        if (model->boneParentMap.find(boneName) != model->boneParentMap.end()) {
            const std::string& parentName = model->boneParentMap.at(boneName);
            computeGlobalTransform(parentName, globalTransforms, boneLocalTransforms);
            parentGlobalTransform = globalTransforms[parentName];
        }

        // ���㵱ǰ������ȫ�ֱ任
        glm::mat4 globalTransform = parentGlobalTransform * localTransform;
        globalTransforms[boneName] = globalTransform;

        // Ӧ�� offsetMatrix
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
