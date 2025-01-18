#include "PostProcessing.h"
#include <iostream>

PostProcessing::PostProcessing(unsigned int width, unsigned int height)
    : width(width), height(height), fbo(0), rbo(0), texture(0), quadVAO(0), quadVBO(0) {
}

PostProcessing::~PostProcessing() {
    glDeleteFramebuffers(1, &fbo);
    glDeleteRenderbuffers(1, &rbo);
    glDeleteTextures(1, &texture);
    glDeleteVertexArrays(1, &quadVAO);
    glDeleteBuffers(1, &quadVBO);
}

bool PostProcessing::initialize() {
    // 主 FBO
    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);

    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // 颜色附件
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture, 0);

    // 创建并绑定渲染缓冲
    glGenRenderbuffers(1, &rbo);
    glBindRenderbuffer(GL_RENDERBUFFER, rbo);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rbo);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        std::cerr << "Main Framebuffer (fbo) is not complete!" << std::endl;
        return false;
    }
    // 解绑定
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

	// 中间 FBO
    glGenFramebuffers(1, &intermediateFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, intermediateFBO);

    glGenTextures(1, &intermediateTex);
    glBindTexture(GL_TEXTURE_2D, intermediateTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, intermediateTex, 0);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        std::cerr << "Intermediate Framebuffer is not complete!" << std::endl;
        return false;
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    initQuad();

    return true;
}

void PostProcessing::registerEffect(const std::string& name, const Shader& shader, std::function<void(Shader&)> configure) {
    effects[name] = shader;
    effectStates[name] = false; // 默认关闭
    effectConfigurations[name] = configure;
}

void PostProcessing::enableEffect(const std::string& name, bool enable) {
    if (effectStates.find(name) != effectStates.end()) {
        effectStates[name] = enable;
    }
}

bool PostProcessing::isEffectEnabled(const std::string& name) const {
    auto it = effectStates.find(name);
    return it != effectStates.end() ? it->second : false;
}

void PostProcessing::setEffectConfig(const std::string& name, std::function<void(Shader&)> configure) {
    if (effectConfigurations.find(name) != effectConfigurations.end()) {
        effectConfigurations[name] = configure;
    }
}

std::map<std::string, bool> PostProcessing::getEffectsState() const {
    return effectStates;
}

void PostProcessing::begin() {
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void PostProcessing::endAndRender()
{
    unsigned int currentTexture = texture;

    glBindVertexArray(quadVAO);

    // 遍历所有效果
    bool firstEnabled = true;
    unsigned int enabledCount = 0;
    for (auto& kv : effectStates) {
        if (kv.second) enabledCount++;
    }

    for (auto& kv : effects) {
        const std::string& name = kv.first;
        Shader& shader = kv.second;

        if (effectStates[name]) {
            bool isLastPass = (--enabledCount == 0);

            if (!isLastPass) {
                // 绑定中间FBO，以将结果输出到 intermediateTex
                glBindFramebuffer(GL_FRAMEBUFFER, intermediateFBO);
                glClear(GL_COLOR_BUFFER_BIT);
            }
            else {
                // 最后一Pass，直接输出到屏幕
                glBindFramebuffer(GL_FRAMEBUFFER, 0);
                glClear(GL_COLOR_BUFFER_BIT);
            }

            shader.use();
            glBindTexture(GL_TEXTURE_2D, currentTexture);

            // 配置着色器
            if (effectConfigurations[name]) {
                effectConfigurations[name](shader);
            }

            glDrawArrays(GL_TRIANGLES, 0, 6);

            // 如果不是最后一Pass，把中间结果再作为下一Pass的输入
            if (!isLastPass) {
                currentTexture = intermediateTex;
            }
        }
    }

    glBindVertexArray(0);
}


bool PostProcessing::hasEnabledEffects() const {
    for (const auto& effect : effectStates) {
        if (effect.second) {
            return true;
        }
    }
    return false;
}


void PostProcessing::initQuad() {
    float quadVertices[] = {
        // Positions   // TexCoords
        -1.0f,  1.0f,  0.0f, 1.0f,
        -1.0f, -1.0f,  0.0f, 0.0f,
         1.0f, -1.0f,  1.0f, 0.0f,

        -1.0f,  1.0f,  0.0f, 1.0f,
         1.0f, -1.0f,  1.0f, 0.0f,
         1.0f,  1.0f,  1.0f, 1.0f
    };

    glGenVertexArrays(1, &quadVAO);
    glGenBuffers(1, &quadVBO);
    glBindVertexArray(quadVAO);
    glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glBindVertexArray(0);
}
