#include "Rendering/Camera.h"
#include <glm/gtc/matrix_transform.hpp>

namespace Kaya {

Camera::Camera(float fov, float aspectRatio, float nearClip, float farClip)
    : m_FOV(fov), m_AspectRatio(aspectRatio), m_NearClip(nearClip), m_FarClip(farClip) {
    m_ProjectionMatrix = glm::perspective(glm::radians(m_FOV), m_AspectRatio, m_NearClip, m_FarClip);
    RecalculateViewMatrix();
}

Camera::Camera(const glm::vec3& position, const glm::vec3& up, float yaw, float pitch)
    : m_Position(position), m_FOV(45.0f), m_AspectRatio(16.0f/9.0f), m_NearClip(0.1f), m_FarClip(1000.0f) {
    m_Rotation.x = pitch;  // Pitch
    m_Rotation.y = yaw;    // Yaw
    m_Rotation.z = 0.0f;   // Roll
    m_ProjectionMatrix = glm::perspective(glm::radians(m_FOV), m_AspectRatio, m_NearClip, m_FarClip);
    RecalculateViewMatrix();
}

void Camera::SetPerspective(float fov, float aspectRatio, float nearClip, float farClip) {
    m_FOV = fov;
    m_AspectRatio = aspectRatio;
    m_NearClip = nearClip;
    m_FarClip = farClip;
    m_ProjectionMatrix = glm::perspective(glm::radians(m_FOV), m_AspectRatio, m_NearClip, m_FarClip);
    m_ViewProjectionMatrix = m_ProjectionMatrix * m_ViewMatrix;
}

void Camera::RecalculateViewMatrix() {
    glm::mat4 transform = glm::translate(glm::mat4(1.0f), m_Position);
    transform = glm::rotate(transform, glm::radians(m_Rotation.x), glm::vec3(1, 0, 0)); // Pitch
    transform = glm::rotate(transform, glm::radians(m_Rotation.y), glm::vec3(0, 1, 0)); // Yaw
    transform = glm::rotate(transform, glm::radians(m_Rotation.z), glm::vec3(0, 0, 1)); // Roll

    m_ViewMatrix = glm::inverse(transform);
    m_ViewProjectionMatrix = m_ProjectionMatrix * m_ViewMatrix;
}

glm::vec3 Camera::GetForward() const {
    glm::mat4 rotation = glm::rotate(glm::mat4(1.0f), glm::radians(m_Rotation.y), glm::vec3(0, 1, 0));
    rotation = glm::rotate(rotation, glm::radians(m_Rotation.x), glm::vec3(1, 0, 0));
    return glm::vec3(rotation * glm::vec4(0, 0, -1, 0));
}

glm::vec3 Camera::GetRight() const {
    glm::mat4 rotation = glm::rotate(glm::mat4(1.0f), glm::radians(m_Rotation.y), glm::vec3(0, 1, 0));
    return glm::vec3(rotation * glm::vec4(1, 0, 0, 0));
}

glm::vec3 Camera::GetUp() const {
    glm::mat4 rotation = glm::rotate(glm::mat4(1.0f), glm::radians(m_Rotation.y), glm::vec3(0, 1, 0));
    rotation = glm::rotate(rotation, glm::radians(m_Rotation.x), glm::vec3(1, 0, 0));
    return glm::vec3(rotation * glm::vec4(0, 1, 0, 0));
}

} // namespace Kaya
