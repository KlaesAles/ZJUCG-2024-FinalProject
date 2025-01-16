// Model.h
#ifndef MODEL_H
#define MODEL_H

#include <glad/glad.h> 

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include "Mesh.h"
#include "Shader.h"
#include "BoundingBox.h"
#include "Animation.h"
#include "BoneInfo.h"

#include "stb_image.h"

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <map>
#include <vector>

// 从文件加载纹理的函数声明
unsigned int TextureFromFile(const char* path, const std::string& directory, bool gamma = false);

class Model
{
public:
    // 模型数据
    std::vector<Texture> textures_loaded;    // 存储目前加载的所有纹理，优化以确保纹理不会被多次加载。
    std::vector<Mesh>    meshes;             // 存储模型的所有网格
    std::string directory;                   // 模型文件所在的目录路径
    bool gammaCorrection;                    // 是否启用伽马校正
    BoundingBox boundingBox;                 // 包围盒
    std::string path;                // 模型文件路径

    std::map<std::string, int> boneMapping; // 骨骼名称到索引的映射
    std::map<std::string, BoneInfo> boneInfoMap; // 骨骼偏移矩阵
    int numBones = 0; // 骨骼总数
    std::map<std::string, std::string> boneParentMap;  // 骨骼父子关系映射

    std::vector<Animation> animations;  // 存储解析后的动画列表

    Model(const std::string& path, bool gamma = false);

    // 获取模型路径
    const std::string& getPath() const;

    // 绘制模型以及它的所有网格
    void Draw(GLint shader) const;

private:
    // 使用ASSIMP支持的文件格式加载模型，并将生成的网格存储到meshes向量中
    void loadModel(const std::string& path);

    // 递归处理节点。处理每个节点的网格，并对其子节点重复此过程（如果有）
    void processNode(aiNode* node, const aiScene* scene);

    // 处理一个ASSIMP的网格，提取其数据并生成一个Mesh对象
    Mesh processMesh(aiMesh* mesh, const aiScene* scene);

    // 检查给定类型的所有材质纹理，并在未加载时加载纹理。
    // 返回包含所需信息的Texture结构体
    std::vector<Texture> loadMaterialTextures(aiMaterial* mat, aiTextureType type, const std::string& typeName);

    // 读取骨骼层次关系
    void readHierarchy(aiNode* node, const aiScene* scene, const std::string& parentName);
    void addBoneData(Vertex& vertex, int boneID, float weight);
    void printBoneHierarchy() const;
};

#endif // MODEL_H
