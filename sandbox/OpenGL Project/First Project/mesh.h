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
    glm::vec3 Position;  // 顶点位置
    glm::vec3 Normal;    // 顶点法线
    glm::vec2 TexCoords; // 纹理坐标
    glm::vec3 Tangent;   // 切线
    glm::vec3 Bitangent; // 副切线
    int boneIDs[MAX_BONE_INFLUENCE];    // 骨骼ID数组
    float weights[MAX_BONE_INFLUENCE]; // 骨骼权重数组
};

struct Texture {
    unsigned int id;    // 纹理ID
    string type;        // 纹理类型
    string path;        // 纹理路径
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
    // 网格数据
    vector<Vertex> vertices;             // 顶点数组
    vector<unsigned int> indices;        // 索引数组
    std::vector<Texture> textures;       // 所有纹理

    unsigned int VAO;

    PBRMaterial material; // 新增 PBR 材质

    // 构造函数
    Mesh(vector<Vertex> vertices, vector<unsigned int> indices, vector<Texture> textures)
    {
        this->vertices = vertices;
        this->indices = indices;
        this->textures = textures;

        // 设置 PBR 材质参数
        setupPBRMaterial();

        // 设置顶点缓冲区及其属性指针
        setupMesh();
    }

	void Draw(Shader& shader) const
    {
        // 设置基本材质属性
        shader.setVec3("material.albedo", material.albedo);
        shader.setFloat("material.metallic", material.metallic);
        shader.setFloat("material.roughness", material.roughness);
        shader.setFloat("material.ao", material.ao);

        // 绑定 PBR 纹理
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

        // 绘制网格
        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, static_cast<unsigned int>(indices.size()), GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);

        // 重置激活的纹理单元
        glActiveTexture(GL_TEXTURE0);
    }

private:
    // 渲染数据
    unsigned int VBO, EBO;

    // 初始化所有缓冲区对象/数组
    void setupMesh()
    {
        // 创建缓冲区/数组
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
        glGenBuffers(1, &EBO);

        glBindVertexArray(VAO);

        // 加载数据到顶点缓冲区
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), &vertices[0], GL_STATIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &indices[0], GL_STATIC_DRAW);

        // 设置顶点属性指针
        // 顶点位置
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
        // 顶点法线
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Normal));
        // 顶点纹理坐标
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, TexCoords));
        // 顶点切线
        glEnableVertexAttribArray(3);
        glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Tangent));
        // 顶点副切线
        glEnableVertexAttribArray(4);
        glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Bitangent));
        // 顶点骨骼ID
        glEnableVertexAttribArray(5);
        glVertexAttribIPointer(5, 4, GL_INT, sizeof(Vertex), (void*)offsetof(Vertex, boneIDs));
        // 顶点权重
        glEnableVertexAttribArray(6);
        glVertexAttribPointer(6, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, weights));

        glBindVertexArray(0); // 解绑VAO
    }

    // 设置 PBR 材质参数
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
