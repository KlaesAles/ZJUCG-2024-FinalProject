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

class Mesh {
public:
    // 网格数据
    vector<Vertex> vertices;             // 顶点数组
    vector<unsigned int> indices;        // 索引数组
    vector<Texture> diffuseTextures;     // 按类型分组的diffuse纹理
    vector<Texture> specularTextures;    // 按类型分组的specular纹理
    vector<Texture> normalTextures;      // 按类型分组的normal纹理
    vector<Texture> heightTextures;      // 按类型分组的height纹理

    unsigned int VAO;

    // 构造函数
    Mesh(vector<Vertex> vertices, vector<unsigned int> indices, vector<Texture> textures)
    {
        this->vertices = vertices;
        this->indices = indices;

        // 根据类型分组纹理
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

        // 设置顶点缓冲区及其属性指针
        setupMesh();
    }

    void Draw(GLint shader)
    {
        // 绑定纹理数组到shader
        unsigned int textureUnit = 0;

        // 设置diffuse纹理
        for (unsigned int i = 0; i < diffuseTextures.size(); i++) {
            glActiveTexture(GL_TEXTURE0 + textureUnit); // 激活相应的纹理单元
            glUniform1i(glGetUniformLocation(shader, ("material.diffuse[" + to_string(i) + "]").c_str()), textureUnit);
            glBindTexture(GL_TEXTURE_2D, diffuseTextures[i].id); // 绑定纹理
            textureUnit++;
        }

        // 设置specular纹理
        for (unsigned int i = 0; i < specularTextures.size(); i++) {
            glActiveTexture(GL_TEXTURE0 + textureUnit);
            glUniform1i(glGetUniformLocation(shader, ("material.specular[" + to_string(i) + "]").c_str()), textureUnit);
            glBindTexture(GL_TEXTURE_2D, specularTextures[i].id);
            textureUnit++;
        }

        // 设置normal纹理
        for (unsigned int i = 0; i < normalTextures.size(); i++) {
            glActiveTexture(GL_TEXTURE0 + textureUnit);
            glUniform1i(glGetUniformLocation(shader, ("material.normal[" + to_string(i) + "]").c_str()), textureUnit);
            glBindTexture(GL_TEXTURE_2D, normalTextures[i].id);
            textureUnit++;
        }

        // 设置height纹理
        for (unsigned int i = 0; i < heightTextures.size(); i++) {
            glActiveTexture(GL_TEXTURE0 + textureUnit);
            glUniform1i(glGetUniformLocation(shader, ("height[" + to_string(i) + "]").c_str()), textureUnit);
            glBindTexture(GL_TEXTURE_2D, heightTextures[i].id);
            textureUnit++;
        }

        // 设置纹理数量到shader
        glUniform1i(glGetUniformLocation(shader, "material.diffuseCount"), diffuseTextures.size());
        glUniform1i(glGetUniformLocation(shader, "material.specularCount"), specularTextures.size());
        glUniform1i(glGetUniformLocation(shader, "material.normalCount"), normalTextures.size());
        glUniform1i(glGetUniformLocation(shader, "heightCount"), heightTextures.size());

        // 绘制网格
        glBindVertexArray(VAO); // 绑定VAO
        glDrawElements(GL_TRIANGLES, static_cast<unsigned int>(indices.size()), GL_UNSIGNED_INT, 0); // 绘制
        glBindVertexArray(0); // 解绑VAO

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
};

#endif
