#ifndef MESH_H
#define MESH_H

#include <glad/glad.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <shader.h>
#include "PBRMaterial.h"

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

enum TextureType {
    TEXTURE_ALBEDO,
    TEXTURE_METALLIC,
    TEXTURE_ROUGHNESS,
    TEXTURE_NORMAL,
    TEXTURE_AO,
};

class Mesh {
public:
    // ��������
    vector<Vertex> vertices;             // ��������
    vector<unsigned int> indices;        // ��������
    std::vector<Texture> textures;       // ��������

    unsigned int VAO;

    PBRMaterial material; // ���� PBR ����

    // ���캯��
    Mesh(vector<Vertex> vertices, vector<unsigned int> indices, vector<Texture> textures)
    {
        this->vertices = vertices;
        this->indices = indices;
        this->textures = textures;

        // ���� PBR ���ʲ���
        setupPBRMaterial();

        // ���ö��㻺������������ָ��
        setupMesh();
    }

	void Draw(Shader& shader) const
    {
        // ���û�����������
        shader.setVec3("material.albedo", material.albedo);
        shader.setFloat("material.metallic", material.metallic);
        shader.setFloat("material.roughness", material.roughness);
        shader.setFloat("material.ao", material.ao);

        // �� PBR ����
        unsigned int textureUnit = 1;

        if (material.useAlbedoMap != 0 && material.albedoMap != 0) {
            glActiveTexture(GL_TEXTURE0 + textureUnit);
            glBindTexture(GL_TEXTURE_2D, material.albedoMap);
            shader.setInt("material.albedoMap", textureUnit++);
            shader.setInt("material.useAlbedoMap", 1);
        }
        else {
            shader.setInt("material.useAlbedoMap", 0);
        }

        if (material.useMetallicMap && material.metallicMap != 0) {
            glActiveTexture(GL_TEXTURE0 + textureUnit);
            glBindTexture(GL_TEXTURE_2D, material.metallicMap);
            shader.setInt("material.metallicMap", textureUnit++);
            shader.setInt("material.useMetallicMap", 1);
        }
        else {
            shader.setInt("material.useMetallicMap", 0);
        }

        if (material.useRoughnessMap && material.roughnessMap != 0) {
            glActiveTexture(GL_TEXTURE0 + textureUnit);
            glBindTexture(GL_TEXTURE_2D, material.roughnessMap);
            shader.setInt("material.roughnessMap", textureUnit++);
            shader.setInt("material.useRoughnessMap", 1);
        }
        else {
            shader.setInt("material.useRoughnessMap", 0);
        }

        if (material.useNormalMap && material.normalMap != 0) {
            glActiveTexture(GL_TEXTURE0 + textureUnit);
            glBindTexture(GL_TEXTURE_2D, material.normalMap);
            shader.setInt("material.normalMap", textureUnit++);
            shader.setInt("material.useNormalMap", 1);
        }
        else {
            shader.setInt("material.useNormalMap", 0);
        }

        if (material.useAOMap && material.aoMap != 0) {
            glActiveTexture(GL_TEXTURE0 + textureUnit);
            glBindTexture(GL_TEXTURE_2D, material.aoMap);
            shader.setInt("material.aoMap", textureUnit++);
            shader.setInt("material.useAOMap", 1);
        }
        else {
            shader.setInt("material.useAOMap", 0);
        }

        // ��������
        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, static_cast<unsigned int>(indices.size()), GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);

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

    // ���� PBR ���ʲ���
    void setupPBRMaterial() {
        for (const auto& tex : textures) {
            if (tex.type == "texture_albedo") {
                material.albedoMap = tex.id;
            }
            else if (tex.type == "texture_metallic") {
                material.metallicMap = tex.id;
            }
            else if (tex.type == "texture_roughness") {
                material.roughnessMap = tex.id;
            }
            else if (tex.type == "texture_normal") {
                material.normalMap = tex.id;
            }
            else if (tex.type == "texture_ao") {
                material.aoMap = tex.id;
            }
        }

        material.updateUsageFlags();

        if (!material.useAlbedoMap) {
            material.albedo = glm::vec3(1.0f);
        }
        if (!material.useMetallicMap) {
            material.metallic = 0.0f;
        }
        if (!material.useRoughnessMap) {
            material.roughness = 0.5f;
        }
        if (!material.useAOMap) {
            material.ao = 1.0f;
        }
    }

};

#endif
