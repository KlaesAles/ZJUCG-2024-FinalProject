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

    // 设置光源属性
    virtual void setPosition(const glm::vec3& position) { throw std::runtime_error("This light type does not support setting position!"); }
    
    virtual void setDirection(const glm::vec3& direction) { throw std::runtime_error("This light type does not support setting direction!"); }
    
    virtual void setColor(const glm::vec3& color) { throw std::runtime_error("This light type does not support setting color!"); }
    
    virtual void setIntensity(float intensity) { throw std::runtime_error("This light type does not support setting intensity!"); }
};

class DirectionalLight : public Light {
private:
    glm::vec3 direction;  // 光源方向
    glm::vec3 color;      // 光源颜色
    float intensity;      // 光源强度
    float orthoSize;      // 正交投影范围大小
    float nearPlane, farPlane; // 投影的裁剪面
    float shadowBias = 0.005f;

public:
    DirectionalLight(glm::vec3 dir, glm::vec3 col = glm::vec3(1.0f), float intensity = 1.0f,
                     float size = 20.0f, float nearP = 0.1f, float farP = 100.0f)
        : direction(glm::normalize(dir)), color(col), intensity(intensity),
          orthoSize(size), nearPlane(nearP), farPlane(farP) {}

    LightType getType() const override { return LightType::Directional; }

    glm::mat4 getProjectionMatrix() const override {
        return glm::ortho(-orthoSize, orthoSize, -orthoSize, orthoSize, nearPlane, farPlane);
    }

    glm::mat4 getViewMatrix() const override{
        glm::vec3 lightTarget = glm::vec3(0.0f, 0.0f, 0.0f);
        glm::vec3 lightPos = getPosition();
        glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);
        if (abs(direction.y) > 0.99f)
            up = glm::vec3(1.0f, 0.0f, 0.0f); // 避免与光源平行
        return glm::lookAt(lightPos, lightTarget, up);
    }

    glm::vec3 getPosition() const override { return -direction; }

    glm::vec3 getDirection() const override { return direction; }

    glm::vec3 getColor() const override { return color; }

    float getIntensity() const override { return intensity; }

    void setDirection(const glm::vec3& newDirection) override {
        direction = glm::normalize(newDirection);
    }

    void setColor(const glm::vec3& newColor) override {
        color = newColor;
    }

    void setIntensity(float newIntensity) override {
        intensity = newIntensity;
    }

    void setOrthoSize(float size) {
        orthoSize = size;
    }
    
    void setShadowBias(float bias) {
        shadowBias = bias;
    }
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

    std::vector<glm::mat4> getViewMatrices() const{
        std::vector<glm::mat4> views;
        views.push_back(glm::lookAt(position, position + glm::vec3(1, 0, 0), glm::vec3(0, -1, 0)));  // +X
        views.push_back(glm::lookAt(position, position + glm::vec3(-1, 0, 0), glm::vec3(0, -1, 0))); // -X
        views.push_back(glm::lookAt(position, position + glm::vec3(0, 1, 0), glm::vec3(0, 0, 1)));   // +Y
        views.push_back(glm::lookAt(position, position + glm::vec3(0, -1, 0), glm::vec3(0, 0, -1))); // -Y
        views.push_back(glm::lookAt(position, position + glm::vec3(0, 0, 1), glm::vec3(0, -1, 0)));  // +Z
        views.push_back(glm::lookAt(position, position + glm::vec3(0, 0, -1), glm::vec3(0, -1, 0))); // -Z
        return views;
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

    void setPosition(const glm::vec3& newPosition) override {
        position = newPosition;
    }

    void setColor(const glm::vec3& newColor) override {
        color = newColor;
    }

    void setIntensity(float newIntensity) override {
        intensity = newIntensity;
    }
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

    float getCutoffAngle() const { return cutoffAngle; }

    void setPosition(const glm::vec3& newPosition) override {
        position = newPosition;
    }

    void setDirection(const glm::vec3& newDirection) override {
        direction = glm::normalize(newDirection);
    }

    void setColor(const glm::vec3& newColor) override {
        color = newColor;
    }

    void setIntensity(float newIntensity) override {
        intensity = newIntensity;
    }

    void setCutoffAngle(float newcutoffAngle) {
        cutoffAngle = newcutoffAngle;
    }
};

#endif // LIGHT_H