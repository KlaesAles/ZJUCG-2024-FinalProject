#ifndef ANIMATION_H
#define ANIMATION_H

#include <string>
#include <vector>
#include <map>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include "BoneInfo.h"

// 单个骨骼关键帧数据
struct BoneKeyframe {
    float time;
    glm::vec3 position;
    glm::quat rotation;
    glm::vec3 scale;
};

// 骨骼动画通道，包含一个骨骼的所有关键帧
struct BoneChannel {
    std::string boneName;
    std::vector<BoneKeyframe> keyframes;
};

class Animation {
private:
    std::string name;
    float duration;
    float ticksPerSecond;
    std::map<std::string, BoneChannel> boneChannels;

public:
    Animation() : name(""), duration(0.0f), ticksPerSecond(0.0f) {}

    Animation(const std::string& name, float duration, float ticksPerSecond)
        : name(name), duration(duration), ticksPerSecond(ticksPerSecond) {}

    const std::string& getName() const { return name; }
    float getDuration() const { return duration; }
    float getTicksPerSecond() const { return ticksPerSecond; }

    void addBoneChannel(const BoneChannel& channel) {
        boneChannels[channel.boneName] = channel;
    }

    const std::map<std::string, BoneChannel>& getBoneChannels() const {
        return boneChannels;
    }

    // 在指定时间评估骨骼变换
    std::map<std::string, glm::mat4> evaluate(float time, const std::map<std::string, BoneInfo>& boneInfoMap) const {
        std::map<std::string, glm::mat4> boneTransforms;

        for (const auto& [boneName, boneInfo] : boneInfoMap) {
            // 检查该骨骼是否存在动画通道
            if (boneChannels.find(boneName) != boneChannels.end()) {
                const BoneChannel& channel = boneChannels.at(boneName);

                if (!channel.keyframes.empty()) {
                    // 处理动画时间：循环时间
                    float animTime = fmod(time, duration);

                    // 查找当前时间点相邻的关键帧
                    const BoneKeyframe* prevKey = &channel.keyframes.front();
                    const BoneKeyframe* nextKey = &channel.keyframes.back();

                    for (size_t i = 0; i < channel.keyframes.size() - 1; ++i) {
                        if (channel.keyframes[i].time <= animTime && channel.keyframes[i + 1].time >= animTime) {
                            prevKey = &channel.keyframes[i];
                            nextKey = &channel.keyframes[i + 1];
                            break;
                        }
                    }

                    // 插值因子
                    float deltaTime = nextKey->time - prevKey->time;
                    float factor = (deltaTime == 0.0f) ? 0.0f : (animTime - prevKey->time) / deltaTime;

                    // 插值位置、旋转和缩放
                    glm::vec3 interpPos = glm::mix(prevKey->position, nextKey->position, factor);
                    glm::quat interpRot = glm::slerp(prevKey->rotation, nextKey->rotation, factor);
                    glm::vec3 interpScale = glm::mix(prevKey->scale, nextKey->scale, factor);

                    // 构造局部变换矩阵
                    glm::mat4 translationMat = glm::translate(glm::mat4(1.0f), interpPos);
                    glm::mat4 rotationMat = glm::mat4_cast(interpRot);
                    glm::mat4 scaleMat = glm::scale(glm::mat4(1.0f), interpScale);
                    glm::mat4 localTransform = translationMat * rotationMat * scaleMat;

                    boneTransforms[boneName] = localTransform;
                }
            }
            else {
                boneTransforms[boneName] = glm::mat4(1.0f);
            }
        }

        return boneTransforms;
    }

};

#endif // ANIMATION_H
