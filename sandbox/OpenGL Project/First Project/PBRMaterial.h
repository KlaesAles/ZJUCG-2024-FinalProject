// PBRMaterial.h
#ifndef PBR_MATERIAL_H
#define PBR_MATERIAL_H

#include <string>
#include <vector>
#include "Mesh.h" // 确保包含 Texture 结构体

struct PBRMaterial {
    // 基本颜色
    glm::vec3 albedo;
    float metallic;
    float roughness;
    float ao; // 环境光遮蔽

    // 纹理
    unsigned int albedoMap;
    unsigned int metallicMap;
    unsigned int roughnessMap;
    unsigned int normalMap;
    unsigned int aoMap;

    // 是否使用纹理
    bool useAlbedoMap;
    bool useMetallicMap;
    bool useRoughnessMap;
    bool useNormalMap;
    bool useAOMap;

    PBRMaterial()
        : albedo(glm::vec3(1.0f)),
        metallic(0.0f),
        roughness(0.5f),
        ao(1.0f),
        albedoMap(0),
        metallicMap(0),
        roughnessMap(0),
        normalMap(0),
        aoMap(0),
        useAlbedoMap(false),
        useMetallicMap(false),
        useRoughnessMap(false),
        useNormalMap(false),
        useAOMap(false)
    {
    }
};

#endif // PBR_MATERIAL_H
