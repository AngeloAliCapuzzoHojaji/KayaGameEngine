#include "Entity.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include "../Physics/PhysicsSystem.h"

namespace Kaya {

    uint32_t Entity::s_NextID = 1;

    Entity::Entity(const std::string& name)
        : m_ID(s_NextID++), m_Name(name)
    {
    }

    glm::mat4 TransformComponent::GetTransform() const {
        glm::mat4 transform = glm::translate(glm::mat4(1.0f), Position);
        
        // Apply rotation
        transform = glm::rotate(transform, glm::radians(Rotation.x), glm::vec3(1, 0, 0));
        transform = glm::rotate(transform, glm::radians(Rotation.y), glm::vec3(0, 1, 0));
        transform = glm::rotate(transform, glm::radians(Rotation.z), glm::vec3(0, 0, 1));
        
        // Apply scale
        transform = glm::scale(transform, Scale);
        
        return transform;
    }

    void Entity::SyncFromPhysics(PhysicsSystem* physics) {
        if (HasPhysics() && physics) {
            m_Transform.Position = physics->GetBodyPosition(m_Physics.BodyID);
            // Note: rotation sync could be added later if needed
        }
    }

}
