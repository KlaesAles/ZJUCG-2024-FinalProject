#ifndef GAME_OBJECT_H
#define GAME_OBJECT_H

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "Model.h"
#include "BoundingBox.h"
#include "Animator.h"

class GameObject {
private:
	std::string name;		   // ����
    glm::mat4 modelMatrix;     // ģ�;�����������ı任
    Model model;               // ģ������
    BoundingBox boundingBox;   // ��Χ��

    glm::vec3 position;        // λ��
    glm::vec3 scale;           // ����
    glm::vec3 rotation;        // ��ת�Ƕȣ��Զ�Ϊ��λ��

    Animator animator;

    // ����ģ�;���
    void updateModelMatrix() {
        modelMatrix = glm::mat4(1.0f);
        modelMatrix = glm::translate(modelMatrix, position);
        modelMatrix = glm::scale(modelMatrix, scale);
        modelMatrix = glm::rotate(modelMatrix, glm::radians(rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
        modelMatrix = glm::rotate(modelMatrix, glm::radians(rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
        modelMatrix = glm::rotate(modelMatrix, glm::radians(rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));
        updateBoundingBox();
    }

    // ���°�Χ��
    void updateBoundingBox() {
        glm::vec3 modelMin = model.boundingBox.min;
        glm::vec3 modelMax = model.boundingBox.max;

        glm::vec3 scaledMin = modelMin * scale + position;
        glm::vec3 scaledMax = modelMax * scale + position;

        boundingBox.min = glm::min(scaledMin, scaledMax);
        boundingBox.max = glm::max(scaledMin, scaledMax);
    }

public:
    // ���캯��
    GameObject(const std::string& name,
        const std::string& modelPath,
        const glm::vec3& position = glm::vec3(0.0f),
        const glm::vec3& scale = glm::vec3(1.0f),
        const glm::vec3& rotation = glm::vec3(0.0f),
        bool gamma = false)
        : name(name), model(modelPath, gamma), animator(&model), position(position), scale(scale), rotation(rotation) {
        updateModelMatrix();
        // �� Model �н����������ж�����ӵ� Animator
        for (const auto& anim : model.animations) {
            animator.addAnimation(anim);
        }
    }

    // ��ӻ�ȡ������ PBR ���ʵĺ���
    PBRMaterial& getPBRMaterial(unsigned int meshIndex = 0) {
        // ����ÿ�� GameObject ֻ��һ�� Mesh������ж�� Mesh����Ҫ���� meshIndex ��ȡ
        return model.meshes[meshIndex].material;
    }

    // ��ȡ����
    const std::string& getName() const {
        return name;
    }

    // ��ȡģ��
    const Model& getModel() const {
        return model;
    }

    // ��ȡģ�;���
    const glm::mat4& getModelMatrix() const {
        return modelMatrix;
    }

    // ��ȡ��Χ��
    const BoundingBox& getBoundingBox() const {
        return boundingBox;
    }

    // ����λ��
    void setPosition(const glm::vec3& newPosition) {
        position = newPosition;
        updateModelMatrix();
    }

    // ��������
    void setScale(const glm::vec3& newScale) {
        scale = newScale;
        updateModelMatrix();
    }

    // ������ת
    void setRotation(const glm::vec3& newRotation) {
        rotation = newRotation;
        updateModelMatrix();
    }

    // ��Ӷ���
    void addAnimation(const Animation& animation) {
        animator.addAnimation(animation);
    }

    // ���Ŷ���
    void playAnimation(const std::string& name) {
        animator.playAnimation(name);
    }

    // ֹͣ����
    void stopAnimation() {
        animator.stopAnimation();
    }

    // ���������߼�������������
    void update(float deltaTime, Shader& shader) {

        animator.update(deltaTime, shader.ID); // ���¶���
    }

    // �ϴ�����������ص� uniform
    void uploadBoneUniforms(Shader& shader) {
        bool useBones = (model.numBones > 0);
        shader.use(); // ������ɫ��
        shader.setInt("useBones", useBones ? 1 : 0);
        if (useBones) {
            animator.update(0.0f, shader.ID); // ֱ�Ӵ��� Shader ����
        }
    }


    // ��ȡ����
    const glm::vec3& getPosition() const { return position; }
    const glm::vec3& getScale() const { return scale; }
    const glm::vec3& getRotation() const { return rotation; }
};

#endif // GAME_OBJECT_H
