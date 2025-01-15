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

    // 初始化资源
    bool initialize();

    // 注册新的后处理效果
    void registerEffect(const std::string& name, const Shader& shader, std::function<void(Shader&)> configure = nullptr);

    // 开启/关闭某个效果
    void enableEffect(const std::string& name, bool enable);
    bool isEffectEnabled(const std::string& name) const;

    // 设置某个效果的参数
    void setEffectConfig(const std::string& name, std::function<void(Shader&)> configure);

    // 获取已注册的效果列表
    std::map<std::string, bool> getEffectsState() const;

    // 开始后处理（绑定 FBO）
    void begin();

    // 结束后处理并应用所有启用的效果
    void endAndRender();

    bool hasEnabledEffects() const;

private:
    unsigned int width, height;
    unsigned int fbo, rbo, texture;       // 帧缓冲和相关纹理
    unsigned int quadVAO, quadVBO;       // 屏幕四边形
    unsigned int intermediateFBO;
    unsigned int intermediateTex;
    std::map<std::string, Shader> effects; // 存储后处理效果
    std::map<std::string, bool> effectStates; // 存储效果的启用状态
    std::map<std::string, std::function<void(Shader&)>> effectConfigurations; // 自定义配置

    // 初始化屏幕四边形
    void initQuad();
};

#endif
