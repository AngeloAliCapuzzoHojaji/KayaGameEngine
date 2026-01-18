#pragma once

#include <string>
#include <memory>
#include <glm/glm.hpp>
#include <Jolt/Jolt.h>
#include <Jolt/Physics/Body/BodyID.h>

namespace Kaya {

    // Forward declarations
    class PhysicsSystem;
    class Texture;

    struct TransformComponent {
        glm::vec3 Position = glm::vec3(0.0f);
        glm::vec3 Rotation = glm::vec3(0.0f); // Euler angles in degrees
        glm::vec3 Scale = glm::vec3(1.0f);

        glm::mat4 GetTransform() const;
    };

    struct RenderComponent {
        enum class Type {
            None,
            Cube,
            Sphere
        };

        Type GeometryType = Type::Cube;
        glm::vec4 Color = glm::vec4(1.0f);
        bool Visible = true;
        std::shared_ptr<Texture> TextureMap = nullptr;
        bool UseTexture = false;
    };

    struct PhysicsComponent {
        JPH::BodyID BodyID;
        bool IsDynamic = false;
        float Mass = 1.0f;
        
        bool IsValid() const { return !BodyID.IsInvalid(); }
    };

    class Entity {
    public:
        Entity(const std::string& name = "Entity");
        ~Entity() = default;

        // Basic properties
        const std::string& GetName() const { return m_Name; }
        void SetName(const std::string& name) { m_Name = name; }
        
        uint32_t GetID() const { return m_ID; }
        bool IsSelected() const { return m_Selected; }
        void SetSelected(bool selected) { m_Selected = selected; }

        // Components
        TransformComponent& GetTransform() { return m_Transform; }
        const TransformComponent& GetTransform() const { return m_Transform; }
        
        RenderComponent& GetRender() { return m_Render; }
        const RenderComponent& GetRender() const { return m_Render; }
        
        PhysicsComponent& GetPhysics() { return m_Physics; }
        const PhysicsComponent& GetPhysics() const { return m_Physics; }

        bool HasPhysics() const { return m_Physics.IsValid(); }

        // Sync transform from physics
        void SyncFromPhysics(PhysicsSystem* physics);

    private:
        uint32_t m_ID;
        std::string m_Name;
        bool m_Selected = false;

        TransformComponent m_Transform;
        RenderComponent m_Render;
        PhysicsComponent m_Physics;

        static uint32_t s_NextID;
    };

}
