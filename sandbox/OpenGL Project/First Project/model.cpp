// Model.cpp
#include "Model.h"

// 构造函数
Model::Model(const std::string& path, bool gamma)
    : path(path),gammaCorrection(gamma)
{
    loadModel(path); // 加载模型
}

// 获取模型路径
const std::string& Model::getPath() const {
    return path;
}

// 从文件加载纹理
unsigned int TextureFromFile(const char* path, const std::string& directory, bool gamma)
{
    std::string filename = std::string(path);
    filename = directory + '/' + filename;

    unsigned int textureID;
    glGenTextures(1, &textureID);

    int width, height, nrComponents;
    // 使用 stbi_load 加载图像
    unsigned char* data = stbi_load(filename.c_str(), &width, &height, &nrComponents, 0);
    if (data)
    {
        GLenum format;
        if (nrComponents == 1)
            format = GL_RED;
        else if (nrComponents == 3)
            format = GL_RGB;
        else if (nrComponents == 4)
            format = GL_RGBA;

        // 根据需要进行伽马校正
        GLenum internalFormat = format;
        if (gamma)
        {
            if (format == GL_RGB)
                internalFormat = GL_SRGB;
            else if (format == GL_RGBA)
                internalFormat = GL_SRGB_ALPHA;
        }

        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        // 设置纹理参数
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        stbi_image_free(data);
    }
    else
    {
        std::cout << "Texture failed to load at path: " << path << std::endl;
        stbi_image_free(data);
    }

    return textureID;
}

// 绘制模型以及它的所有网格
void Model::Draw(GLint shader) const
{
    for (unsigned int i = 0; i < meshes.size(); i++)
        meshes[i].Draw(shader);
}

// 加载模型
void Model::loadModel(const std::string& path)
{
    // 使用ASSIMP读取文件
    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_FlipUVs | aiProcess_CalcTangentSpace);
    // 检查是否出错
    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) // 如果出错
    {
        std::cout << "ERROR::ASSIMP:: " << importer.GetErrorString() << std::endl;
        return;
    }
    // 获取文件路径的目录部分
    directory = path.substr(0, path.find_last_of('/'));

    // 递归处理ASSIMP的根节点
    processNode(scene->mRootNode, scene);
}

// 递归处理节点
void Model::processNode(aiNode* node, const aiScene* scene)
{
    // 处理当前节点的每个网格
    for (unsigned int i = 0; i < node->mNumMeshes; i++)
    {
        // 节点对象仅包含索引，用于索引场景中的实际对象。
        // 场景包含所有数据，节点仅用于保持组织（例如节点之间的关系）。
        aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
        meshes.emplace_back(processMesh(mesh, scene));
    }
    // 在处理完所有网格后（如果有），递归处理每个子节点
    for (unsigned int i = 0; i < node->mNumChildren; i++)
    {
        processNode(node->mChildren[i], scene);
    }
}

// 处理一个ASSIMP的网格，提取其数据并生成一个Mesh对象
Mesh Model::processMesh(aiMesh* mesh, const aiScene* scene)
{
    // 用于存储网格数据的容器
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    std::vector<Texture> textures;
    std::vector<glm::vec3> points;

    // 遍历网格的每个顶点
    for (unsigned int i = 0; i < mesh->mNumVertices; i++)
    {
        Vertex vertex;
        glm::vec3 vector; // 临时向量

        // 顶点位置
        vector.x = mesh->mVertices[i].x;
        vector.y = mesh->mVertices[i].y;
        vector.z = mesh->mVertices[i].z;
        vertex.Position = vector;

        // 顶点法线
        if (mesh->HasNormals())
        {
            vector.x = mesh->mNormals[i].x;
            vector.y = mesh->mNormals[i].y;
            vector.z = mesh->mNormals[i].z;
            vertex.Normal = vector;
        }

        // 纹理坐标
        if (mesh->mTextureCoords[0]) // 网格是否包含纹理坐标
        {
            glm::vec2 vec;
            // 一个顶点可以包含最多8种不同的纹理坐标。我们假设不会使用顶点包含多个纹理坐标的模型，因此始终使用第一个集合（0）。
            vec.x = mesh->mTextureCoords[0][i].x;
            vec.y = mesh->mTextureCoords[0][i].y;
            vertex.TexCoords = vec;

            // 切线
            vector.x = mesh->mTangents[i].x;
            vector.y = mesh->mTangents[i].y;
            vector.z = mesh->mTangents[i].z;
            vertex.Tangent = vector;

            // 副切线
            vector.x = mesh->mBitangents[i].x;
            vector.y = mesh->mBitangents[i].y;
            vector.z = mesh->mBitangents[i].z;
            vertex.Bitangent = vector;
        }
        else
            vertex.TexCoords = glm::vec2(0.0f, 0.0f);

        vertices.emplace_back(vertex);

        // 创建包围盒
        glm::vec3 point;
        point.x = mesh->mVertices[i].x;
        point.y = mesh->mVertices[i].y;
        point.z = mesh->mVertices[i].z;
        points.emplace_back(point);
    }

    // 初始化或更新包围盒
    for (const auto& point : points) {
        boundingBox.update(point);
    }

    // 遍历网格的每个面（一个面是一个三角形），并获取对应的顶点索引
    for (unsigned int i = 0; i < mesh->mNumFaces; i++)
    {
        aiFace face = mesh->mFaces[i];
        // 获取面的所有索引并存储到indices向量中
        for (unsigned int j = 0; j < face.mNumIndices; j++)
            indices.emplace_back(face.mIndices[j]);
    }

    // 处理材质
    aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];

    // 1. 处理diffuse贴图
    std::vector<Texture> diffuseMaps = loadMaterialTextures(material, aiTextureType_DIFFUSE, "texture_diffuse");
    textures.insert(textures.end(), diffuseMaps.begin(), diffuseMaps.end());

    // 2. 处理specular贴图
    std::vector<Texture> specularMaps = loadMaterialTextures(material, aiTextureType_SPECULAR, "texture_specular");
    textures.insert(textures.end(), specularMaps.begin(), specularMaps.end());

    // 3. 处理normal贴图
    std::vector<Texture> normalMaps = loadMaterialTextures(material, aiTextureType_HEIGHT, "texture_normal");
    textures.insert(textures.end(), normalMaps.begin(), normalMaps.end());

    // 4. 处理height贴图
    std::vector<Texture> heightMaps = loadMaterialTextures(material, aiTextureType_AMBIENT, "texture_height");
    textures.insert(textures.end(), heightMaps.begin(), heightMaps.end());

    // 返回一个由提取的网格数据创建的Mesh对象
    return Mesh(vertices, indices, textures);
}

// 检查给定类型的所有材质纹理，并在未加载时加载纹理。
// 返回包含所需信息的Texture结构体
std::vector<Texture> Model::loadMaterialTextures(aiMaterial* mat, aiTextureType type, const std::string& typeName)
{
    std::vector<Texture> textures;
    for (unsigned int i = 0; i < mat->GetTextureCount(type); i++)
    {
        aiString str;
        mat->GetTexture(type, i, &str);
        // 检查纹理是否已加载，如果已加载，跳过加载新纹理
        bool skip = false;
        for (unsigned int j = 0; j < textures_loaded.size(); j++)
        {
            if (std::strcmp(textures_loaded[j].path.c_str(), str.C_Str()) == 0)
            {
                textures.emplace_back(textures_loaded[j]);
                skip = true; // 如果已加载相同路径的纹理，跳过加载（优化）
                break;
            }
        }
        if (!skip)
        {   // 如果纹理尚未加载，则加载
            Texture texture;
            texture.id = TextureFromFile(str.C_Str(), this->directory, gammaCorrection);
            texture.type = typeName;
            texture.path = str.C_Str();
            textures.emplace_back(texture);
            textures_loaded.emplace_back(texture);  // 将其存储为整个模型已加载的纹理，以确保不会重复加载相同纹理。
        }
    }
    return textures;
}
