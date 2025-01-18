#ifndef POST_PROCESSING_H
#define POST_PROCESSING_H

#include <glad/glad.h>
#include <map>
#include <string>
#include <functional>
#include "Shader.h"

class PostProcessing {
public:
    PostProcessing(unsigned int width, unsigned int height);
    ~PostProcessing();

    // ��ʼ����Դ
    bool initialize();

    // ע���µĺ���Ч��
    void registerEffect(const std::string& name, const Shader& shader, std::function<void(Shader&)> configure = nullptr);

    // ����/�ر�ĳ��Ч��
    void enableEffect(const std::string& name, bool enable);
    bool isEffectEnabled(const std::string& name) const;

    // ����ĳ��Ч���Ĳ���
    void setEffectConfig(const std::string& name, std::function<void(Shader&)> configure);

    // ��ȡ��ע���Ч���б�
    std::map<std::string, bool> getEffectsState() const;

    // ��ʼ������ FBO��
    void begin();

    // ��������Ӧ���������õ�Ч��
    void endAndRender();

    bool hasEnabledEffects() const;

private:
    unsigned int width, height;
    unsigned int fbo, rbo, texture;       // ֡������������
    unsigned int quadVAO, quadVBO;       // ��Ļ�ı���
    unsigned int intermediateFBO;
    unsigned int intermediateTex;
    std::map<std::string, Shader> effects; // �洢����Ч��
    std::map<std::string, bool> effectStates; // �洢Ч��������״̬
    std::map<std::string, std::function<void(Shader&)>> effectConfigurations; // �Զ�������

    // ��ʼ����Ļ�ı���
    void initQuad();
};

#endif
