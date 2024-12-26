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

#include "stb_image.h"

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <map>
#include <vector>

// ���ļ���������ĺ�������
unsigned int TextureFromFile(const char* path, const std::string& directory, bool gamma = false);

class Model
{
public:
    // ģ������
    std::vector<Texture> textures_loaded;    // �洢Ŀǰ���ص����������Ż���ȷ�������ᱻ��μ��ء�
    std::vector<Mesh>    meshes;             // �洢ģ�͵���������
    std::string directory;                   // ģ���ļ����ڵ�Ŀ¼·��
    bool gammaCorrection;                    // �Ƿ�����٤��У��
    BoundingBox boundingBox;                 // ��Χ��
    std::string path;                // ģ���ļ�·��

    // ���캯������������3Dģ���ļ�·��
    Model(const std::string& path, bool gamma = false);

    // ��ȡģ��·��
    const std::string& getPath() const;

    // ����ģ���Լ�������������
    void Draw(GLint shader) const;

private:
    // ʹ��ASSIMP֧�ֵ��ļ���ʽ����ģ�ͣ��������ɵ�����洢��meshes������
    void loadModel(const std::string& path);

    // �ݹ鴦��ڵ㡣����ÿ���ڵ�����񣬲������ӽڵ��ظ��˹��̣�����У�
    void processNode(aiNode* node, const aiScene* scene);

    // ����һ��ASSIMP��������ȡ�����ݲ�����һ��Mesh����
    Mesh processMesh(aiMesh* mesh, const aiScene* scene);

    // ���������͵����в�����������δ����ʱ��������
    // ���ذ���������Ϣ��Texture�ṹ��
    std::vector<Texture> loadMaterialTextures(aiMaterial* mat, aiTextureType type, const std::string& typeName);
};

#endif // MODEL_H
