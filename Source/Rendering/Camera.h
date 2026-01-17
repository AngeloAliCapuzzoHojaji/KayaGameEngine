#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace Kaya {

class Camera {
public:
    Camera(float fov = 45.0f, float aspectRatio = 16.0f/9.0f, float nearClip = 0.1f, float farClip = 1000.0f);

    void SetPosition(const glm::vec3& position) { m_Position = position; RecalculateViewMatrix(); }
    void SetRotation(const glm::vec3& rotation) { m_Rotation = rotation; RecalculateViewMatrix(); }
    
    const glm::vec3& GetPosition() const { return m_Position; }
    const glm::vec3& GetRotation() const { return m_Rotation; }

    // Yaw and Pitch accessors (in degrees)
    void SetYaw(float yaw) { m_Rotation.y = yaw; RecalculateViewMatrix(); }
    void SetPitch(float pitch) { 
        pitch = glm::clamp(pitch, -89.0f, 89.0f); 
        m_Rotation.x = pitch; 
        RecalculateViewMatrix(); 
    }
    float GetYaw() const { return m_Rotation.y; }
    float GetPitch() const { return m_Rotation.x; }
    
    void SetAspectRatio(float aspectRatio) { 
        m_AspectRatio = aspectRatio; 
        SetPerspective(m_FOV, m_AspectRatio, m_NearClip, m_FarClip); 
    }

    const glm::mat4& GetProjectionMatrix() const { return m_ProjectionMatrix; }
    const glm::mat4& GetViewMatrix() const { return m_ViewMatrix; }
    const glm::mat4& GetViewProjectionMatrix() const { return m_ViewProjectionMatrix; }

    void SetPerspective(float fov, float aspectRatio, float nearClip, float farClip);
    
    glm::vec3 GetForward() const;
    glm::vec3 GetRight() const;
    glm::vec3 GetUp() const;

private:
    void RecalculateViewMatrix();

private:
    glm::mat4 m_ProjectionMatrix;
    glm::mat4 m_ViewMatrix;
    glm::mat4 m_ViewProjectionMatrix;

    glm::vec3 m_Position = { 0.0f, 0.0f, 0.0f };
    glm::vec3 m_Rotation = { 0.0f, 0.0f, 0.0f }; // Pitch, Yaw, Roll

    float m_FOV;
    float m_AspectRatio;
    float m_NearClip;
    float m_FarClip;
};

} // namespace Kaya
