#ifndef MESH_H
#define MESH_H

#include <glad/glad.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <shader.h>

#include <string>
#include <vector>

using namespace std;

#define MAX_BONE_INFLUENCE 4

struct Vertex {
    glm::vec3 Position;  // ����λ��
    glm::vec3 Normal;    // ���㷨��
    glm::vec2 TexCoords; // ��������
    glm::vec3 Tangent;   // ����
    glm::vec3 Bitangent; // ������
    int boneIDs[MAX_BONE_INFLUENCE];    // ����ID����
    float weights[MAX_BONE_INFLUENCE]; // ����Ȩ������
};

struct Texture {
    unsigned int id;    // ����ID
    string type;        // ��������
    string path;        // ����·��
};

class Mesh {
public:
    // ��������
    vector<Vertex> vertices;             // ��������
    vector<unsigned int> indices;        // ��������
    vector<Texture> diffuseTextures;     // �����ͷ����diffuse����
    vector<Texture> specularTextures;    // �����ͷ����specular����
    vector<Texture> normalTextures;      // �����ͷ����normal����
    vector<Texture> heightTextures;      // �����ͷ����height����

    unsigned int VAO;

    // ���캯��
    Mesh(vector<Vertex> vertices, vector<unsigned int> indices, vector<Texture> textures)
    {
        this->vertices = vertices;
        this->indices = indices;

        // �������ͷ�������
        for (const auto& tex : textures) {
            if (tex.type == "texture_diffuse")
                diffuseTextures.push_back(tex);
            else if (tex.type == "texture_specular")
                specularTextures.push_back(tex);
            else if (tex.type == "texture_normal")
                normalTextures.push_back(tex);
            else if (tex.type == "texture_height")
                heightTextures.push_back(tex);
        }

        // ���ö��㻺������������ָ��
        setupMesh();
    }

    void Draw(GLint shader)
    {
        // ���������鵽shader
        unsigned int textureUnit = 0;

        // ����diffuse����
        for (unsigned int i = 0; i < diffuseTextures.size(); i++) {
            glActiveTexture(GL_TEXTURE0 + textureUnit); // ������Ӧ������Ԫ
            glUniform1i(glGetUniformLocation(shader, ("material.diffuse[" + to_string(i) + "]").c_str()), textureUnit);
            glBindTexture(GL_TEXTURE_2D, diffuseTextures[i].id); // ������
            textureUnit++;
        }

        // ����specular����
        for (unsigned int i = 0; i < specularTextures.size(); i++) {
            glActiveTexture(GL_TEXTURE0 + textureUnit);
            glUniform1i(glGetUniformLocation(shader, ("material.specular[" + to_string(i) + "]").c_str()), textureUnit);
            glBindTexture(GL_TEXTURE_2D, specularTextures[i].id);
            textureUnit++;
        }

        // ����normal����
        for (unsigned int i = 0; i < normalTextures.size(); i++) {
            glActiveTexture(GL_TEXTURE0 + textureUnit);
            glUniform1i(glGetUniformLocation(shader, ("material.normal[" + to_string(i) + "]").c_str()), textureUnit);
            glBindTexture(GL_TEXTURE_2D, normalTextures[i].id);
            textureUnit++;
        }

        // ����height����
        for (unsigned int i = 0; i < heightTextures.size(); i++) {
            glActiveTexture(GL_TEXTURE0 + textureUnit);
            glUniform1i(glGetUniformLocation(shader, ("height[" + to_string(i) + "]").c_str()), textureUnit);
            glBindTexture(GL_TEXTURE_2D, heightTextures[i].id);
            textureUnit++;
        }

        // ��������������shader
        glUniform1i(glGetUniformLocation(shader, "material.diffuseCount"), diffuseTextures.size());
        glUniform1i(glGetUniformLocation(shader, "material.specularCount"), specularTextures.size());
        glUniform1i(glGetUniformLocation(shader, "material.normalCount"), normalTextures.size());
        glUniform1i(glGetUniformLocation(shader, "heightCount"), heightTextures.size());

        // ��������
        glBindVertexArray(VAO); // ��VAO
        glDrawElements(GL_TRIANGLES, static_cast<unsigned int>(indices.size()), GL_UNSIGNED_INT, 0); // ����
        glBindVertexArray(0); // ���VAO

        // ���ü��������Ԫ
        glActiveTexture(GL_TEXTURE0);
    }

private:
    // ��Ⱦ����
    unsigned int VBO, EBO;

    // ��ʼ�����л���������/����
    void setupMesh()
    {
        // ����������/����
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
        glGenBuffers(1, &EBO);

        glBindVertexArray(VAO);

        // �������ݵ����㻺����
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), &vertices[0], GL_STATIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &indices[0], GL_STATIC_DRAW);

        // ���ö�������ָ��
        // ����λ��
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
        // ���㷨��
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Normal));
        // ������������
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, TexCoords));
        // ��������
        glEnableVertexAttribArray(3);
        glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Tangent));
        // ���㸱����
        glEnableVertexAttribArray(4);
        glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Bitangent));
        // �������ID
        glEnableVertexAttribArray(5);
        glVertexAttribIPointer(5, 4, GL_INT, sizeof(Vertex), (void*)offsetof(Vertex, boneIDs));
        // ����Ȩ��
        glEnableVertexAttribArray(6);
        glVertexAttribPointer(6, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, weights));

        glBindVertexArray(0); // ���VAO
    }
};

#endif
