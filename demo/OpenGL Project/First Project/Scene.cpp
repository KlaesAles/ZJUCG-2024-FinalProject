#include "Scene.h"

void Scene::draw(Shader& shader) const {
    draw(shader, nullptr);
}

void Scene::draw(Shader& shader, const std::shared_ptr<GameObject>& selectedObject) const {
    for (const auto& obj : gameObjects) {
        shader.setMat4("model", obj->getModelMatrix());

        // �����ѡ�е����壬���Ӹ���Ч��
        if (selectedObject && obj == selectedObject) {
            // �������������ԭʼ����
            std::vector<PBRMaterial> originalMaterials;
            for (size_t i = 0; i < obj->getModel().meshes.size(); ++i) {
                originalMaterials.push_back(obj->getPBRMaterial(i));
                PBRMaterial& material = obj->getPBRMaterial(i);

                // �����ǿ����Ч��
                material.albedo = material.albedo * 1.5f;  // �������
                material.metallic = std::min(material.metallic * 0.5f + 0.5f, 1.0f);  // ���ӽ�����
                material.roughness = std::max(material.roughness * 0.5f, 0.1f);  // ���ʹֲڶȣ�ʹ������⻬
                material.ao = 1.0f;  // ��󻷾����ڱ�
            }

            // ��Ⱦ����
            obj->uploadBoneUniforms(shader);
            obj->getModel().Draw(shader);

            // �ָ�ԭʼ����
            for (size_t i = 0; i < obj->getModel().meshes.size(); ++i) {
                obj->getPBRMaterial(i) = originalMaterials[i];
            }
        }
        else {
            // ������Ⱦδѡ�е�����
            obj->uploadBoneUniforms(shader);
            obj->getModel().Draw(shader);
        }
    }
}
