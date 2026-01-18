#include "Rendering/Light.h"
#include <glad/glad.h>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>

namespace Kaya {

// ========== ShadowMap ==========

ShadowMap::ShadowMap(unsigned int width, unsigned int height)
    : m_Width(width), m_Height(height) {
    
    // Create framebuffer
    glGenFramebuffers(1, &m_FBO);

    // Create depth texture
    glGenTextures(1, &m_DepthMap);
    glBindTexture(GL_TEXTURE_2D, m_DepthMap);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, width, height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

    // Attach depth texture to framebuffer
    glBindFramebuffer(GL_FRAMEBUFFER, m_FBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, m_DepthMap, 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        std::cerr << "Shadow framebuffer is not complete!" << std::endl;
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    std::cout << "Shadow map created: " << width << "x" << height << std::endl;
}

ShadowMap::~ShadowMap() {
    if (m_DepthMap) glDeleteTextures(1, &m_DepthMap);
    if (m_FBO) glDeleteFramebuffers(1, &m_FBO);
}

void ShadowMap::Bind() {
    glBindFramebuffer(GL_FRAMEBUFFER, m_FBO);
    glViewport(0, 0, m_Width, m_Height);
    glClear(GL_DEPTH_BUFFER_BIT);
}

void ShadowMap::Unbind() {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

// ========== DirectionalLight ==========

DirectionalLight::DirectionalLight(const glm::vec3& direction, const glm::vec3& color, float intensity)
    : Direction(glm::normalize(direction)), Color(color), Intensity(intensity) {
}

glm::mat4 DirectionalLight::GetLightSpaceMatrix() const {
    glm::mat4 lightProjection = glm::ortho(-ShadowDistance, ShadowDistance, 
                                          -ShadowDistance, ShadowDistance, 
                                          ShadowNearPlane, ShadowFarPlane);
    
    glm::vec3 lightPos = -Direction * (ShadowDistance * 0.5f);
    glm::mat4 lightView = glm::lookAt(lightPos, glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    
    return lightProjection * lightView;
}

} // namespace Kaya
