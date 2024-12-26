// Model.cpp
#include "Model.h"

// ���캯��
Model::Model(const std::string& path, bool gamma)
    : path(path),gammaCorrection(gamma)
{
    loadModel(path); // ����ģ��
}

// ��ȡģ��·��
const std::string& Model::getPath() const {
    return path;
}

// ���ļ���������
unsigned int TextureFromFile(const char* path, const std::string& directory, bool gamma)
{
    std::string filename = std::string(path);
    filename = directory + '/' + filename;

    unsigned int textureID;
    glGenTextures(1, &textureID);

    int width, height, nrComponents;
    // ʹ�� stbi_load ����ͼ��
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

        // ������Ҫ����٤��У��
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

        // �����������
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

// ����ģ���Լ�������������
void Model::Draw(GLint shader) const
{
    for (unsigned int i = 0; i < meshes.size(); i++)
        meshes[i].Draw(shader);
}

// ����ģ��
void Model::loadModel(const std::string& path)
{
    // ʹ��ASSIMP��ȡ�ļ�
    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_FlipUVs | aiProcess_CalcTangentSpace);
    // ����Ƿ����
    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) // �������
    {
        std::cout << "ERROR::ASSIMP:: " << importer.GetErrorString() << std::endl;
        return;
    }
    // ��ȡ�ļ�·����Ŀ¼����
    directory = path.substr(0, path.find_last_of('/'));

    // �ݹ鴦��ASSIMP�ĸ��ڵ�
    processNode(scene->mRootNode, scene);
}

// �ݹ鴦��ڵ�
void Model::processNode(aiNode* node, const aiScene* scene)
{
    // ����ǰ�ڵ��ÿ������
    for (unsigned int i = 0; i < node->mNumMeshes; i++)
    {
        // �ڵ����������������������������е�ʵ�ʶ���
        // ���������������ݣ��ڵ�����ڱ�����֯������ڵ�֮��Ĺ�ϵ����
        aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
        meshes.emplace_back(processMesh(mesh, scene));
    }
    // �ڴ������������������У����ݹ鴦��ÿ���ӽڵ�
    for (unsigned int i = 0; i < node->mNumChildren; i++)
    {
        processNode(node->mChildren[i], scene);
    }
}

// ����һ��ASSIMP��������ȡ�����ݲ�����һ��Mesh����
Mesh Model::processMesh(aiMesh* mesh, const aiScene* scene)
{
    // ���ڴ洢�������ݵ�����
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    std::vector<Texture> textures;
    std::vector<glm::vec3> points;

    // ���������ÿ������
    for (unsigned int i = 0; i < mesh->mNumVertices; i++)
    {
        Vertex vertex;
        glm::vec3 vector; // ��ʱ����

        // ����λ��
        vector.x = mesh->mVertices[i].x;
        vector.y = mesh->mVertices[i].y;
        vector.z = mesh->mVertices[i].z;
        vertex.Position = vector;

        // ���㷨��
        if (mesh->HasNormals())
        {
            vector.x = mesh->mNormals[i].x;
            vector.y = mesh->mNormals[i].y;
            vector.z = mesh->mNormals[i].z;
            vertex.Normal = vector;
        }

        // ��������
        if (mesh->mTextureCoords[0]) // �����Ƿ������������
        {
            glm::vec2 vec;
            // һ��������԰������8�ֲ�ͬ���������ꡣ���Ǽ��費��ʹ�ö������������������ģ�ͣ����ʼ��ʹ�õ�һ�����ϣ�0����
            vec.x = mesh->mTextureCoords[0][i].x;
            vec.y = mesh->mTextureCoords[0][i].y;
            vertex.TexCoords = vec;

            // ����
            vector.x = mesh->mTangents[i].x;
            vector.y = mesh->mTangents[i].y;
            vector.z = mesh->mTangents[i].z;
            vertex.Tangent = vector;

            // ������
            vector.x = mesh->mBitangents[i].x;
            vector.y = mesh->mBitangents[i].y;
            vector.z = mesh->mBitangents[i].z;
            vertex.Bitangent = vector;
        }
        else
            vertex.TexCoords = glm::vec2(0.0f, 0.0f);

        vertices.emplace_back(vertex);

        // ������Χ��
        glm::vec3 point;
        point.x = mesh->mVertices[i].x;
        point.y = mesh->mVertices[i].y;
        point.z = mesh->mVertices[i].z;
        points.emplace_back(point);
    }

    // ��ʼ������°�Χ��
    for (const auto& point : points) {
        boundingBox.update(point);
    }

    // ���������ÿ���棨һ������һ�������Σ�������ȡ��Ӧ�Ķ�������
    for (unsigned int i = 0; i < mesh->mNumFaces; i++)
    {
        aiFace face = mesh->mFaces[i];
        // ��ȡ��������������洢��indices������
        for (unsigned int j = 0; j < face.mNumIndices; j++)
            indices.emplace_back(face.mIndices[j]);
    }

    // �������
    aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];

    // 1. ����diffuse��ͼ
    std::vector<Texture> diffuseMaps = loadMaterialTextures(material, aiTextureType_DIFFUSE, "texture_diffuse");
    textures.insert(textures.end(), diffuseMaps.begin(), diffuseMaps.end());

    // 2. ����specular��ͼ
    std::vector<Texture> specularMaps = loadMaterialTextures(material, aiTextureType_SPECULAR, "texture_specular");
    textures.insert(textures.end(), specularMaps.begin(), specularMaps.end());

    // 3. ����normal��ͼ
    std::vector<Texture> normalMaps = loadMaterialTextures(material, aiTextureType_HEIGHT, "texture_normal");
    textures.insert(textures.end(), normalMaps.begin(), normalMaps.end());

    // 4. ����height��ͼ
    std::vector<Texture> heightMaps = loadMaterialTextures(material, aiTextureType_AMBIENT, "texture_height");
    textures.insert(textures.end(), heightMaps.begin(), heightMaps.end());

    // ����һ������ȡ���������ݴ�����Mesh����
    return Mesh(vertices, indices, textures);
}

// ���������͵����в�����������δ����ʱ��������
// ���ذ���������Ϣ��Texture�ṹ��
std::vector<Texture> Model::loadMaterialTextures(aiMaterial* mat, aiTextureType type, const std::string& typeName)
{
    std::vector<Texture> textures;
    for (unsigned int i = 0; i < mat->GetTextureCount(type); i++)
    {
        aiString str;
        mat->GetTexture(type, i, &str);
        // ��������Ƿ��Ѽ��أ�����Ѽ��أ���������������
        bool skip = false;
        for (unsigned int j = 0; j < textures_loaded.size(); j++)
        {
            if (std::strcmp(textures_loaded[j].path.c_str(), str.C_Str()) == 0)
            {
                textures.emplace_back(textures_loaded[j]);
                skip = true; // ����Ѽ�����ͬ·���������������أ��Ż���
                break;
            }
        }
        if (!skip)
        {   // ���������δ���أ������
            Texture texture;
            texture.id = TextureFromFile(str.C_Str(), this->directory, gammaCorrection);
            texture.type = typeName;
            texture.path = str.C_Str();
            textures.emplace_back(texture);
            textures_loaded.emplace_back(texture);  // ����洢Ϊ����ģ���Ѽ��ص�������ȷ�������ظ�������ͬ����
        }
    }
    return textures;
}
