#ifndef GAME_OBJECT_H
#define GAME_OBJECT_H

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glad/glad.h>

class GameObject {
private:
    glm::mat4 modelMatrix; // ģ�;�����������ı任
    GLuint VAO;            // ����������� ID������Ҫ���Ƶļ�����
    int vertexCount;       // �������������� glDrawArrays ���������Ʒ���
    GLenum drawMode;       // ����ģʽ��Ĭ��Ϊ GL_TRIANGLES

    glm::vec3 position;    // λ��
    glm::vec3 scale;       // ����
    glm::vec3 rotation;    // ��ת�Ƕȣ��Զ�Ϊ��λ��

    void updateModelMatrix() {
        modelMatrix = glm::mat4(1.0f);
        modelMatrix = glm::translate(modelMatrix, position);
        modelMatrix = glm::scale(modelMatrix, scale);
        modelMatrix = glm::rotate(modelMatrix, glm::radians(rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
        modelMatrix = glm::rotate(modelMatrix, glm::radians(rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
        modelMatrix = glm::rotate(modelMatrix, glm::radians(rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));
    }

public:
    // ���캯������ʼ��ģ�;��󲢰� VAO
    GameObject(GLuint vao, int vertexCount, glm::vec3 position = glm::vec3(0.0f),
               glm::vec3 scale = glm::vec3(1.0f), glm::vec3 rotation = glm::vec3(0.0f), GLenum drawMode = GL_TRIANGLES)
        : VAO(vao), vertexCount(vertexCount), drawMode(drawMode),
          position(position), scale(scale), rotation(rotation) {
        updateModelMatrix();
    }

    // ���Ʒ���
    void draw(GLuint shaderProgram) const {
        // ʹ�ô������ɫ������
        glUseProgram(shaderProgram);

        // ����ģ�;�����ɫ��
        GLuint modelLoc = glGetUniformLocation(shaderProgram, "model");
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, &modelMatrix[0][0]);

        // ���Ƽ�����
        glBindVertexArray(VAO);
        glDrawArrays(drawMode, 0, vertexCount);
        glBindVertexArray(0);
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

    // ��ȡģ�;���
    const glm::mat4& getModelMatrix() const {
        return modelMatrix;
    }

    // ��ȡ�󶨵� VAO
    GLuint getVAO() const {
        return VAO;
    }

    // ���û���ģʽ
    void setDrawMode(GLenum mode) {
        drawMode = mode;
    }
};

#endif // GAME_OBJECT_H
