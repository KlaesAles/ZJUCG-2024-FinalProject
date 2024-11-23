#ifndef LIGHT_H
#define LIGHT_H

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <string>

enum class LightType {
    Directional,
    Point,
    Spot
};

class Light {
public:
    virtual ~Light() = default;

    // 获取光源类型
    virtual LightType getType() const = 0;

    // 获取光源的投影矩阵
    virtual glm::mat4 getProjectionMatrix() const = 0;

    // 获取光源的视图矩阵
    virtual glm::mat4 getViewMatrix() const = 0;

    // 获取光源位置
    virtual glm::vec3 getPosition() const = 0;

    // 获取光源方向
    virtual glm::vec3 getDirection() const = 0;

    // 获取光源颜色
    virtual glm::vec3 getColor() const { return glm::vec3(1.0f); }

    // 获取光源强度
    virtual float getIntensity() const { return 1.0f; }
};

class DirectionalLight : public Light {
private:
    glm::vec3 direction;  // 光源方向
    glm::vec3 color;      // 光源颜色
    float intensity;      // 光源强度
    float orthoSize;      // 正交投影范围大小
    float nearPlane, farPlane; // 投影的裁剪面

public:
    DirectionalLight(glm::vec3 dir, glm::vec3 col = glm::vec3(1.0f), float intensity = 1.0f,
                     float size = 10.0f, float nearP = 0.1f, float farP = 100.0f)
        : direction(glm::normalize(dir)), color(col), intensity(intensity),
          orthoSize(size), nearPlane(nearP), farPlane(farP) {}

    LightType getType() const override { return LightType::Directional; }

    glm::mat4 getProjectionMatrix() const override {
        return glm::ortho(-orthoSize, orthoSize, -orthoSize, orthoSize, nearPlane, farPlane);
    }

    glm::mat4 getViewMatrix() const override {
        glm::vec3 lightPos = -direction * 10.0f;
        return glm::lookAt(lightPos, glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    }

    glm::vec3 getPosition() const override { return -direction * 10.0f; }

    glm::vec3 getDirection() const override { return direction; }

    glm::vec3 getColor() const override { return color; }

    float getIntensity() const override { return intensity; }
};

class PointLight : public Light {
private:
    glm::vec3 position;  // 光源位置
    glm::vec3 color;     // 光源颜色
    float intensity;     // 光源强度
    float nearPlane, farPlane; // 投影的裁剪面

public:
    PointLight(glm::vec3 pos, glm::vec3 col = glm::vec3(1.0f), float intensity = 1.0f,
               float nearP = 0.1f, float farP = 100.0f)
        : position(pos), color(col), intensity(intensity),
          nearPlane(nearP), farPlane(farP) {}

    LightType getType() const override { return LightType::Point; }

    glm::mat4 getProjectionMatrix() const override {
        return glm::perspective(glm::radians(90.0f), 1.0f, nearPlane, farPlane);
    }

    glm::mat4 getViewMatrix() const override {
        throw std::runtime_error("PointLight does not have a single view matrix!");
    }

    glm::vec3 getPosition() const override { return position; }

    glm::vec3 getDirection() const override {
        throw std::runtime_error("PointLight does not have a single direction!");
    }

    glm::vec3 getColor() const override { return color; }

    float getIntensity() const override { return intensity; }
};

class SpotLight : public Light {
private:
    glm::vec3 position;  // 光源位置
    glm::vec3 direction; // 光源方向
    glm::vec3 color;     // 光源颜色
    float intensity;     // 光源强度
    float cutoffAngle;   // 聚光锥的角度（FOV）
    float nearPlane, farPlane; // 投影的裁剪面

public:
    SpotLight(glm::vec3 pos, glm::vec3 dir, glm::vec3 col = glm::vec3(1.0f), float intensity = 1.0f,
              float cutoff = 45.0f, float nearP = 0.1f, float farP = 100.0f)
        : position(pos), direction(glm::normalize(dir)), color(col), intensity(intensity),
          cutoffAngle(cutoff), nearPlane(nearP), farPlane(farP) {}

    LightType getType() const override { return LightType::Spot; }

    glm::mat4 getProjectionMatrix() const override {
        return glm::perspective(glm::radians(cutoffAngle * 2.0f), 1.0f, nearPlane, farPlane);
    }

    glm::mat4 getViewMatrix() const override {
        return glm::lookAt(position, position + direction, glm::vec3(0.0f, 1.0f, 0.0f));
    }

    glm::vec3 getPosition() const override { return position; }

    glm::vec3 getDirection() const override { return direction; }

    glm::vec3 getColor() const override { return color; }

    float getIntensity() const override { return intensity; }
};

#endif // LIGHT_H