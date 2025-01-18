#include "Scene.h"

void Scene::draw(Shader& shader) const {
    draw(shader, nullptr);
}

void Scene::draw(Shader& shader, const std::shared_ptr<GameObject>& selectedObject) const {
    for (const auto& obj : gameObjects) {
        shader.setMat4("model", obj->getModelMatrix());

        // 如果是选中的物体，增加高亮效果
        if (selectedObject && obj == selectedObject) {
            // 保存所有网格的原始材质
            std::vector<PBRMaterial> originalMaterials;
            for (size_t i = 0; i < obj->getModel().meshes.size(); ++i) {
                originalMaterials.push_back(obj->getPBRMaterial(i));
                PBRMaterial& material = obj->getPBRMaterial(i);

                // 大幅增强高亮效果
                material.albedo = material.albedo * 1.5f;  // 提高亮度
                material.metallic = std::min(material.metallic * 0.5f + 0.5f, 1.0f);  // 增加金属感
                material.roughness = std::max(material.roughness * 0.5f, 0.1f);  // 降低粗糙度，使表面更光滑
                material.ao = 1.0f;  // 最大环境光遮蔽
            }

            // 渲染物体
            obj->uploadBoneUniforms(shader);
            obj->getModel().Draw(shader);

            // 恢复原始材质
            for (size_t i = 0; i < obj->getModel().meshes.size(); ++i) {
                obj->getPBRMaterial(i) = originalMaterials[i];
            }
        }
        else {
            // 正常渲染未选中的物体
            obj->uploadBoneUniforms(shader);
            obj->getModel().Draw(shader);
        }
    }
}
