#pragma once

#include <Jolt/Jolt.h>
#include <Jolt/Core/Factory.h>
#include <Jolt/Core/TempAllocator.h>
#include <Jolt/Core/JobSystemThreadPool.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/Physics/PhysicsSettings.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Collision/Shape/SphereShape.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Body/BodyActivationListener.h>

#include <glm/glm.hpp>
#include <memory>

namespace Kaya {

class PhysicsSystem {
public:
    PhysicsSystem();
    ~PhysicsSystem();

    void Initialize();
    void Shutdown();
    void Update(float deltaTime);

    // Body creation
    JPH::BodyID CreateBox(const glm::vec3& position, const glm::vec3& size, bool isDynamic = true);
    JPH::BodyID CreateSphere(const glm::vec3& position, float radius, bool isDynamic = true);
    
    // Body manipulation
    void SetBodyPosition(JPH::BodyID bodyID, const glm::vec3& position);
    void SetBodyVelocity(JPH::BodyID bodyID, const glm::vec3& velocity);
    void AddForce(JPH::BodyID bodyID, const glm::vec3& force);
    void RemoveBody(JPH::BodyID bodyID);

    // Body queries
    glm::vec3 GetBodyPosition(JPH::BodyID bodyID) const;
    glm::vec3 GetBodyRotation(JPH::BodyID bodyID) const;

    JPH::PhysicsSystem& GetJoltPhysicsSystem() { return *m_PhysicsSystem; }

private:
    std::unique_ptr<JPH::TempAllocatorImpl> m_TempAllocator;
    std::unique_ptr<JPH::JobSystemThreadPool> m_JobSystem;
    std::unique_ptr<JPH::PhysicsSystem> m_PhysicsSystem;
    
    JPH::BodyInterface* m_BodyInterface = nullptr;
    
    const unsigned int m_MaxBodies = 65536;
    const unsigned int m_MaxBodyPairs = 65536;
    const unsigned int m_MaxContactConstraints = 10240;
};

} // namespace Kaya
