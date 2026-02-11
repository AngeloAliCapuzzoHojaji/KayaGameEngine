#pragma once

#include <glm/glm.hpp>
#include <array>

namespace Kaya {

// Axis-Aligned Bounding Box
struct AABB {
    glm::vec3 Min;
    glm::vec3 Max;

    AABB() : Min(0.0f), Max(0.0f) {}
    AABB(const glm::vec3& min, const glm::vec3& max) : Min(min), Max(max) {}

    // Build AABB from center + half-extents
    static AABB FromCenterSize(const glm::vec3& center, const glm::vec3& size) {
        glm::vec3 half = size * 0.5f;
        return AABB(center - half, center + half);
    }

    // Build AABB from center + radius (sphere bounding box)
    static AABB FromSphere(const glm::vec3& center, float radius) {
        return AABB(center - glm::vec3(radius), center + glm::vec3(radius));
    }

    glm::vec3 GetCenter() const { return (Min + Max) * 0.5f; }
    glm::vec3 GetExtents() const { return (Max - Min) * 0.5f; }
};

// Frustum plane: ax + by + cz + d = 0
struct Plane {
    glm::vec3 Normal = { 0.0f, 1.0f, 0.0f };
    float Distance = 0.0f;

    Plane() = default;
    Plane(const glm::vec3& normal, float distance)
        : Normal(normal), Distance(distance) {}

    // Signed distance from point to plane
    float DistanceToPoint(const glm::vec3& point) const {
        return glm::dot(Normal, point) + Distance;
    }

    // Normalize the plane equation
    void Normalize() {
        float length = glm::length(Normal);
        if (length > 0.0f) {
            Normal /= length;
            Distance /= length;
        }
    }
};

// View frustum with 6 planes extracted from the ViewProjection matrix
class Frustum {
public:
    enum Side { LEFT = 0, RIGHT, BOTTOM, TOP, NEAR_PLANE, FAR_PLANE, COUNT };

    Frustum() = default;

    // Extract frustum planes from a combined View-Projection matrix
    // Uses the Griggs-Hartmann method
    void ExtractPlanes(const glm::mat4& vp) {
        // Left: row3 + row0
        m_Planes[LEFT].Normal.x = vp[0][3] + vp[0][0];
        m_Planes[LEFT].Normal.y = vp[1][3] + vp[1][0];
        m_Planes[LEFT].Normal.z = vp[2][3] + vp[2][0];
        m_Planes[LEFT].Distance = vp[3][3] + vp[3][0];

        // Right: row3 - row0
        m_Planes[RIGHT].Normal.x = vp[0][3] - vp[0][0];
        m_Planes[RIGHT].Normal.y = vp[1][3] - vp[1][0];
        m_Planes[RIGHT].Normal.z = vp[2][3] - vp[2][0];
        m_Planes[RIGHT].Distance = vp[3][3] - vp[3][0];

        // Bottom: row3 + row1
        m_Planes[BOTTOM].Normal.x = vp[0][3] + vp[0][1];
        m_Planes[BOTTOM].Normal.y = vp[1][3] + vp[1][1];
        m_Planes[BOTTOM].Normal.z = vp[2][3] + vp[2][1];
        m_Planes[BOTTOM].Distance = vp[3][3] + vp[3][1];

        // Top: row3 - row1
        m_Planes[TOP].Normal.x = vp[0][3] - vp[0][1];
        m_Planes[TOP].Normal.y = vp[1][3] - vp[1][1];
        m_Planes[TOP].Normal.z = vp[2][3] - vp[2][1];
        m_Planes[TOP].Distance = vp[3][3] - vp[3][1];

        // Near: row3 + row2
        m_Planes[NEAR_PLANE].Normal.x = vp[0][3] + vp[0][2];
        m_Planes[NEAR_PLANE].Normal.y = vp[1][3] + vp[1][2];
        m_Planes[NEAR_PLANE].Normal.z = vp[2][3] + vp[2][2];
        m_Planes[NEAR_PLANE].Distance = vp[3][3] + vp[3][2];

        // Far: row3 - row2
        m_Planes[FAR_PLANE].Normal.x = vp[0][3] - vp[0][2];
        m_Planes[FAR_PLANE].Normal.y = vp[1][3] - vp[1][2];
        m_Planes[FAR_PLANE].Normal.z = vp[2][3] - vp[2][2];
        m_Planes[FAR_PLANE].Distance = vp[3][3] - vp[3][2];

        // Normalize all planes
        for (auto& plane : m_Planes) {
            plane.Normalize();
        }
    }

    // Test if an AABB is inside or intersects the frustum
    bool IsBoxVisible(const AABB& box) const {
        for (const auto& plane : m_Planes) {
            // Find the positive vertex (the one furthest along the normal)
            glm::vec3 pVertex;
            pVertex.x = (plane.Normal.x >= 0.0f) ? box.Max.x : box.Min.x;
            pVertex.y = (plane.Normal.y >= 0.0f) ? box.Max.y : box.Min.y;
            pVertex.z = (plane.Normal.z >= 0.0f) ? box.Max.z : box.Min.z;

            // If the positive vertex is outside, the entire box is outside
            if (plane.DistanceToPoint(pVertex) < 0.0f) {
                return false;
            }
        }
        return true;
    }

    // Test if a sphere is inside or intersects the frustum
    bool IsSphereVisible(const glm::vec3& center, float radius) const {
        for (const auto& plane : m_Planes) {
            if (plane.DistanceToPoint(center) < -radius) {
                return false;
            }
        }
        return true;
    }

    // Test if a point is inside the frustum
    bool IsPointVisible(const glm::vec3& point) const {
        for (const auto& plane : m_Planes) {
            if (plane.DistanceToPoint(point) < 0.0f) {
                return false;
            }
        }
        return true;
    }

private:
    std::array<Plane, COUNT> m_Planes;
};

} // namespace Kaya
