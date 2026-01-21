#include "Physics/PhysicsSystem.h"
#include <iostream>
#include <cstdarg>
#include <Jolt/RegisterTypes.h>

// Jolt Physics namespace
using namespace JPH;

// Callback for traces
static void TraceImpl(const char* inFMT, ...) {
    va_list list;
    va_start(list, inFMT);
    char buffer[1024];
    vsnprintf(buffer, sizeof(buffer), inFMT, list);
    va_end(list);
    std::cout << buffer << std::endl;
}

#ifdef JPH_ENABLE_ASSERTS
static bool AssertFailedImpl(const char* inExpression, const char* inMessage, const char* inFile, uint inLine) {
    std::cerr << inFile << ":" << inLine << ": (" << inExpression << ") " << (inMessage != nullptr ? inMessage : "") << std::endl;
    return true;
};
#endif

// Layer definitions
namespace Layers {
    static constexpr ObjectLayer NON_MOVING = 0;
    static constexpr ObjectLayer MOVING = 1;
    static constexpr ObjectLayer NUM_LAYERS = 2;
};

// Object vs Broadphase layer pair filter
class ObjectLayerPairFilterImpl : public ObjectLayerPairFilter {
public:
    virtual bool ShouldCollide(ObjectLayer inObject1, ObjectLayer inObject2) const override {
        switch (inObject1) {
            case Layers::NON_MOVING:
                return inObject2 == Layers::MOVING;
            case Layers::MOVING:
                return true;
            default:
                return false;
        }
    }
};

// BroadPhase layer interface
namespace BroadPhaseLayers {
    static constexpr BroadPhaseLayer NON_MOVING(0);
    static constexpr BroadPhaseLayer MOVING(1);
    static constexpr uint NUM_LAYERS(2);
};

class BPLayerInterfaceImpl final : public BroadPhaseLayerInterface {
public:
    BPLayerInterfaceImpl() {
        m_ObjectToBroadPhase[Layers::NON_MOVING] = BroadPhaseLayers::NON_MOVING;
        m_ObjectToBroadPhase[Layers::MOVING] = BroadPhaseLayers::MOVING;
    }

    virtual uint GetNumBroadPhaseLayers() const override {
        return BroadPhaseLayers::NUM_LAYERS;
    }

    virtual BroadPhaseLayer GetBroadPhaseLayer(ObjectLayer inLayer) const override {
        return m_ObjectToBroadPhase[inLayer];
    }

    virtual const char* GetBroadPhaseLayerName(BroadPhaseLayer inLayer) const override {
        switch ((BroadPhaseLayer::Type)inLayer) {
            case (BroadPhaseLayer::Type)BroadPhaseLayers::NON_MOVING: return "NON_MOVING";
            case (BroadPhaseLayer::Type)BroadPhaseLayers::MOVING: return "MOVING";
            default: return "INVALID";
        }
    }

private:
    BroadPhaseLayer m_ObjectToBroadPhase[Layers::NUM_LAYERS];
};

class ObjectVsBroadPhaseLayerFilterImpl : public ObjectVsBroadPhaseLayerFilter {
public:
    virtual bool ShouldCollide(ObjectLayer inLayer1, BroadPhaseLayer inLayer2) const override {
        switch (inLayer1) {
            case Layers::NON_MOVING:
                return inLayer2 == BroadPhaseLayers::MOVING;
            case Layers::MOVING:
                return true;
            default:
                return false;
        }
    }
};

// Contact listener (optional)
class MyContactListener : public ContactListener {
public:
    virtual ValidateResult OnContactValidate(const Body& inBody1, const Body& inBody2, 
                                             RVec3Arg inBaseOffset, const CollideShapeResult& inCollisionResult) override {
        return ValidateResult::AcceptAllContactsForThisBodyPair;
    }

    virtual void OnContactAdded(const Body& inBody1, const Body& inBody2, 
                               const ContactManifold& inManifold, ContactSettings& ioSettings) override {
    }

    virtual void OnContactPersisted(const Body& inBody1, const Body& inBody2, 
                                   const ContactManifold& inManifold, ContactSettings& ioSettings) override {
    }

    virtual void OnContactRemoved(const SubShapeIDPair& inSubShapePair) override {
    }
};

namespace Kaya {

PhysicsSystem::PhysicsSystem() {
}

PhysicsSystem::~PhysicsSystem() {
    Shutdown();
}

void PhysicsSystem::Initialize() {
    // Register allocation hook
    RegisterDefaultAllocator();

    // Install callbacks
    Trace = TraceImpl;
    JPH_IF_ENABLE_ASSERTS(AssertFailed = AssertFailedImpl;)

    // Create factory
    Factory::sInstance = new Factory();

    // Register all physics types
    RegisterTypes();

    // Create temp allocator
    m_TempAllocator = std::make_unique<TempAllocatorImpl>(10 * 1024 * 1024);

    // Create job system
    m_JobSystem = std::make_unique<JobSystemThreadPool>(cMaxPhysicsJobs, cMaxPhysicsBarriers, 
                                                         std::thread::hardware_concurrency() - 1);

    // Create physics system
    m_PhysicsSystem = std::make_unique<JPH::PhysicsSystem>();
    
    m_PhysicsSystem->Init(
        m_MaxBodies,
        0, // num body mutexes
        m_MaxBodyPairs,
        m_MaxContactConstraints,
        *new BPLayerInterfaceImpl(),
        *new ObjectVsBroadPhaseLayerFilterImpl(),
        *new ObjectLayerPairFilterImpl()
    );

    // Optional: set contact listener
    m_PhysicsSystem->SetContactListener(new MyContactListener());

    m_BodyInterface = &m_PhysicsSystem->GetBodyInterface();

    std::cout << "Jolt Physics initialized successfully" << std::endl;
}

void PhysicsSystem::Shutdown() {
    if (m_PhysicsSystem) {
        m_PhysicsSystem.reset();
    }
    
    if (m_JobSystem) {
        m_JobSystem.reset();
    }
    
    if (m_TempAllocator) {
        m_TempAllocator.reset();
    }

    UnregisterTypes();
    
    if (Factory::sInstance) {
        delete Factory::sInstance;
        Factory::sInstance = nullptr;
    }
}

void PhysicsSystem::Update(float deltaTime) {
    if (!m_PhysicsSystem) return;

    const int collisionSteps = 1;
    m_PhysicsSystem->Update(deltaTime, collisionSteps, m_TempAllocator.get(), m_JobSystem.get());
}

BodyID PhysicsSystem::CreateBox(const glm::vec3& position, const glm::vec3& size, bool isDynamic) {
    BoxShapeSettings boxSettings(Vec3(size.x * 0.5f, size.y * 0.5f, size.z * 0.5f));
    ShapeSettings::ShapeResult shapeResult = boxSettings.Create();
    ShapeRefC boxShape = shapeResult.Get();

    BodyCreationSettings bodySettings(
        boxShape,
        RVec3(position.x, position.y, position.z),
        Quat::sIdentity(),
        isDynamic ? EMotionType::Dynamic : EMotionType::Static,
        isDynamic ? Layers::MOVING : Layers::NON_MOVING
    );

    Body* body = m_BodyInterface->CreateBody(bodySettings);
    if (!body) {
        std::cerr << "Failed to create box body" << std::endl;
        return BodyID();
    }

    m_BodyInterface->AddBody(body->GetID(), EActivation::Activate);
    return body->GetID();
}

BodyID PhysicsSystem::CreateSphere(const glm::vec3& position, float radius, bool isDynamic) {
    SphereShapeSettings sphereSettings(radius);
    ShapeSettings::ShapeResult shapeResult = sphereSettings.Create();
    ShapeRefC sphereShape = shapeResult.Get();

    BodyCreationSettings bodySettings(
        sphereShape,
        RVec3(position.x, position.y, position.z),
        Quat::sIdentity(),
        isDynamic ? EMotionType::Dynamic : EMotionType::Static,
        isDynamic ? Layers::MOVING : Layers::NON_MOVING
    );

    Body* body = m_BodyInterface->CreateBody(bodySettings);
    if (!body) {
        std::cerr << "Failed to create sphere body" << std::endl;
        return BodyID();
    }

    m_BodyInterface->AddBody(body->GetID(), EActivation::Activate);
    return body->GetID();
}

void PhysicsSystem::SetBodyPosition(BodyID bodyID, const glm::vec3& position) {
    m_BodyInterface->SetPosition(bodyID, RVec3(position.x, position.y, position.z), EActivation::Activate);
}

void PhysicsSystem::SetBodyVelocity(BodyID bodyID, const glm::vec3& velocity) {
    m_BodyInterface->SetLinearVelocity(bodyID, Vec3(velocity.x, velocity.y, velocity.z));
}

void PhysicsSystem::AddForce(BodyID bodyID, const glm::vec3& force) {
    m_BodyInterface->AddForce(bodyID, Vec3(force.x, force.y, force.z));
}

void PhysicsSystem::RemoveBody(BodyID bodyID) {
    m_BodyInterface->RemoveBody(bodyID);
    m_BodyInterface->DestroyBody(bodyID);
}

glm::vec3 PhysicsSystem::GetBodyPosition(BodyID bodyID) const {
    RVec3 pos = m_BodyInterface->GetPosition(bodyID);
    return glm::vec3(pos.GetX(), pos.GetY(), pos.GetZ());
}

glm::vec3 PhysicsSystem::GetBodyRotation(BodyID bodyID) const {
    Quat rot = m_BodyInterface->GetRotation(bodyID);
    Vec3 euler = rot.GetEulerAngles();
    return glm::vec3(euler.GetX(), euler.GetY(), euler.GetZ());
}

glm::vec3 PhysicsSystem::GetBodyVelocity(BodyID bodyID) const {
    Vec3 vel = m_BodyInterface->GetLinearVelocity(bodyID);
    return glm::vec3(vel.GetX(), vel.GetY(), vel.GetZ());
}

} // namespace Kaya
