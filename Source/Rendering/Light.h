#pragma once

#include <glm/glm.hpp>

namespace Kaya {

class ShadowMap {
public:
    ShadowMap(unsigned int width = 2048, unsigned int height = 2048);
    ~ShadowMap();

    void Bind();
    void Unbind();

    unsigned int GetDepthMapTexture() const { return m_DepthMap; }
    unsigned int GetWidth() const { return m_Width; }
    unsigned int GetHeight() const { return m_Height; }

private:
    unsigned int m_FBO = 0;
    unsigned int m_DepthMap = 0;
    unsigned int m_Width;
    unsigned int m_Height;
};

class DirectionalLight {
public:
    DirectionalLight(const glm::vec3& direction = glm::vec3(-0.3f, -1.0f, -0.3f),
                     const glm::vec3& color = glm::vec3(1.0f),
                     float intensity = 1.0f);

    glm::mat4 GetLightSpaceMatrix() const;
    
    glm::vec3 Direction;
    glm::vec3 Color;
    float Intensity;
    bool CastShadows = true;

    // Shadow parameters
    float ShadowDistance = 50.0f;
    float ShadowNearPlane = 1.0f;
    float ShadowFarPlane = 100.0f;
};

} // namespace Kaya
