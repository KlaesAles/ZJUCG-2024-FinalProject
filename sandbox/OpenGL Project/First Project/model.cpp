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

        // 伽马校正
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
    std::cout << "Loading model: " << path << std::endl;
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

    // 解析动画数据
    if (scene->HasAnimations()) {
        for (unsigned int animIndex = 0; animIndex < scene->mNumAnimations; ++animIndex) {
            aiAnimation* aiAnim = scene->mAnimations[animIndex];
            float duration = static_cast<float>(aiAnim->mDuration);
            float ticksPerSecond = static_cast<float>((aiAnim->mTicksPerSecond != 0) ? aiAnim->mTicksPerSecond : 25.0f);
            Animation animation(aiAnim->mName.C_Str(), duration, ticksPerSecond);

            for (unsigned int channelIndex = 0; channelIndex < aiAnim->mNumChannels; ++channelIndex) {
                aiNodeAnim* channel = aiAnim->mChannels[channelIndex];
                BoneChannel boneChannel;
                boneChannel.boneName = channel->mNodeName.C_Str();

                unsigned int numKeys = channel->mNumPositionKeys;
                for (unsigned int k = 0; k < numKeys; ++k) {
                    BoneKeyframe key;
                    key.time = static_cast<float>(channel->mPositionKeys[k].mTime);
                    key.position = glm::vec3(
                        channel->mPositionKeys[k].mValue.x,
                        channel->mPositionKeys[k].mValue.y,
                        channel->mPositionKeys[k].mValue.z
                    );
                    key.rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
                    key.scale = glm::vec3(1.0f);
                    boneChannel.keyframes.push_back(key);
                }

                unsigned int numRotKeys = channel->mNumRotationKeys;
                for (unsigned int r = 0; r < numRotKeys && r < boneChannel.keyframes.size(); ++r) {
                    boneChannel.keyframes[r].rotation = glm::quat(
                        channel->mRotationKeys[r].mValue.w,
                        channel->mRotationKeys[r].mValue.x,
                        channel->mRotationKeys[r].mValue.y,
                        channel->mRotationKeys[r].mValue.z
                    );
                }

                unsigned int numScaleKeys = channel->mNumScalingKeys;
                for (unsigned int s = 0; s < numScaleKeys && s < boneChannel.keyframes.size(); ++s) {
                    boneChannel.keyframes[s].scale = glm::vec3(
                        channel->mScalingKeys[s].mValue.x,
                        channel->mScalingKeys[s].mValue.y,
                        channel->mScalingKeys[s].mValue.z
                    );
                }
                animation.addBoneChannel(boneChannel);
            }
            // 将解析后的动画存储到 Model 的 animations 容器中
            animations.push_back(animation);
        }
    }
    // 读取骨骼层次关系
    readHierarchy(scene->mRootNode, scene, "");
    printBoneHierarchy();
    std::cout << "Finished processing nodes." << std::endl;
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
        if (mesh->mTextureCoords[0])
        {
            glm::vec2 vec;
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

        // 初始化顶点的骨骼数据
        for (int j = 0; j < MAX_BONE_INFLUENCE; ++j) {
            vertex.boneIDs[j] = 0;
            vertex.weights[j] = 0.0f;
        }

        vertices.emplace_back(vertex);

        // 创建包围盒
        glm::vec3 point;
        point.x = mesh->mVertices[i].x;
        point.y = mesh->mVertices[i].y;
        point.z = mesh->mVertices[i].z;
        points.emplace_back(point);
    }

    // 处理骨骼数据，此时确保所有顶点均已初始化
    if (mesh->HasBones()) {
        for (unsigned int b = 0; b < mesh->mNumBones; b++) {
            aiBone* bone = mesh->mBones[b];
            int boneIndex = 0;

            if (boneMapping.find(bone->mName.C_Str()) == boneMapping.end()) {
                boneIndex = numBones++;
                BoneInfo boneInfo;
                boneInfo.offsetMatrix = glm::transpose(glm::mat4(
                    bone->mOffsetMatrix.a1, bone->mOffsetMatrix.a2, bone->mOffsetMatrix.a3, bone->mOffsetMatrix.a4,
                    bone->mOffsetMatrix.b1, bone->mOffsetMatrix.b2, bone->mOffsetMatrix.b3, bone->mOffsetMatrix.b4,
                    bone->mOffsetMatrix.c1, bone->mOffsetMatrix.c2, bone->mOffsetMatrix.c3, bone->mOffsetMatrix.c4,
                    bone->mOffsetMatrix.d1, bone->mOffsetMatrix.d2, bone->mOffsetMatrix.d3, bone->mOffsetMatrix.d4
                ));
                boneInfoMap[bone->mName.C_Str()] = boneInfo;
                boneMapping[bone->mName.C_Str()] = boneIndex;
            }
            else {
                boneIndex = boneMapping[bone->mName.C_Str()];
            }

            for (unsigned int j = 0; j < bone->mNumWeights; j++) {
                unsigned int vertexID = bone->mWeights[j].mVertexId;
                float weight = bone->mWeights[j].mWeight;
                // 确保vertexID在vertices范围内
                if (vertexID < vertices.size()) {
                    addBoneData(vertices[vertexID], boneIndex, weight);
                }
                else {
                    std::cerr << "Warning: vertexID " << vertexID << " out of bounds (size: " << vertices.size() << ")" << std::endl;
                }
            }
        }
    }

    // 初始化或更新包围盒
    for (const auto& point : points) {
        boundingBox.update(point);
    }

    /*
    // 输出每个顶点的骨骼绑定数据
    for (unsigned int i = 0; i < vertices.size(); ++i) {
        std::cout << "Vertex " << i << ":\n";
        for (int j = 0; j < MAX_BONE_INFLUENCE; ++j) {
            if (vertices[i].weights[j] > 0.0f) {
                std::string boneName = "Unknown";
                for (const auto& [name, index] : boneMapping) {
                    if (index == vertices[i].boneIDs[j]) {
                        boneName = name;
                        break;
                    }
                }
                std::cout << "  Bone " << j << ": " << boneName
                    << " (ID: " << vertices[i].boneIDs[j]
                    << ", Weight: " << vertices[i].weights[j] << ")\n";
            }
        }
    }
    */

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

    std::vector<Texture> diffuseMaps = loadMaterialTextures(material, aiTextureType_DIFFUSE, "texture_diffuse");
    textures.insert(textures.end(), diffuseMaps.begin(), diffuseMaps.end());

    std::vector<Texture> specularMaps = loadMaterialTextures(material, aiTextureType_SPECULAR, "texture_specular");
    textures.insert(textures.end(), specularMaps.begin(), specularMaps.end());

    std::vector<Texture> normalMaps = loadMaterialTextures(material, aiTextureType_HEIGHT, "texture_normal");
    textures.insert(textures.end(), normalMaps.begin(), normalMaps.end());

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

void Model::addBoneData(Vertex& vertex, int boneID, float weight) {
    for (int i = 0; i < MAX_BONE_INFLUENCE; ++i) {
        if (vertex.weights[i] == 0.0f) {
            vertex.boneIDs[i] = boneID;
            vertex.weights[i] = weight;
            return;
        }
    }
    std::cerr << "Warning: vertex bone influences exceeded MAX_BONE_INFLUENCE." << std::endl;
}

void Model::readHierarchy(aiNode* node, const aiScene* scene, const std::string& parentName) {
    std::string nodeName(node->mName.C_Str());

    // 过滤辅助节点 (如 "_$AssimpFbx$_PreRotation" 等)
    if (nodeName.find("_$AssimpFbx$_") != std::string::npos) {
        for (unsigned int i = 0; i < node->mNumChildren; ++i) {
            readHierarchy(node->mChildren[i], scene, parentName);
        }
        return;
    }

    // 如果节点已经存在于 boneParentMap，则跳过
    if (boneParentMap.find(nodeName) != boneParentMap.end()) {
        return;
    }

    // 如果当前节点是骨骼，记录父子关系
    if (boneMapping.find(nodeName) != boneMapping.end()) {
        boneParentMap[nodeName] = parentName;
    }

    // 遍历子节点并递归调用
    for (unsigned int i = 0; i < node->mNumChildren; ++i) {
        readHierarchy(node->mChildren[i], scene, nodeName);
    }
}

// 输出骨骼父子关系
void Model::printBoneHierarchy() const {
    for (const auto& [boneName, parentName] : boneParentMap) {
        std::cout << "Bone: " << boneName
            << " Parent: " << (parentName.empty() ? "None (Root)" : parentName)
            << std::endl;
    }
}

