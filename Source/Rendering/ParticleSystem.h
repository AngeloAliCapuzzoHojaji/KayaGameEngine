#pragma once

#include <glm/glm.hpp>
#include <memory>
#include <vector>
#include <string>
#include <random>
#include <functional>

namespace Kaya {

class Shader;
class Texture;
class Camera;

// ---------- Particle ----------

struct Particle {
    glm::vec3 Position  = glm::vec3(0.0f);
    glm::vec3 Velocity  = glm::vec3(0.0f);
    glm::vec4 Color     = glm::vec4(1.0f);
    float     Size      = 1.0f;
    float     Life      = 0.0f;   // remaining in seconds
    float     MaxLife   = 1.0f;
    float     Rotation  = 0.0f;   // degrees
    float     AngularVelocity = 0.0f;
};

// ---------- Emitter shapes ----------

enum class EmitterShape {
    Point,
    Sphere,
    Hemisphere,
    Cone,
    Box
};

// ---------- Blend modes ----------

enum class ParticleBlendMode {
    Alpha,      // standard alpha blending
    Additive    // additive (fire, sparks, etc.)
};

// ---------- Value ranges ----------

template <typename T>
struct Range {
    T Min{};
    T Max{};

    Range() = default;
    Range(T value) : Min(value), Max(value) {}
    Range(T min, T max) : Min(min), Max(max) {}
};

// ---------- Emitter properties ----------

struct ParticleEmitterProperties {
    // Emission
    float EmissionRate      = 10.0f;   // particles per second
    int   BurstCount        = 0;       // instantaneous burst (0 = continuous)
    int   MaxParticles      = 1000;

    // Shape
    EmitterShape Shape      = EmitterShape::Point;
    float ShapeRadius       = 1.0f;    // sphere/hemisphere/cone radius
    float ConeAngle         = 45.0f;   // degrees, half-angle
    glm::vec3 BoxExtents    = glm::vec3(1.0f);

    // Lifetime
    Range<float> Lifetime   = { 1.0f, 2.0f };

    // Motion
    Range<float> Speed      = { 1.0f, 3.0f };
    glm::vec3 Direction     = glm::vec3(0.0f, 1.0f, 0.0f); // base direction
    float DirectionSpread   = 0.0f;    // radians of random deviation
    glm::vec3 Gravity       = glm::vec3(0.0f, -9.81f, 0.0f);
    float Drag              = 0.0f;    // linear velocity damping

    // Appearance
    Range<float> StartSize  = { 0.1f, 0.3f };
    Range<float> EndSize    = { 0.0f, 0.0f };
    glm::vec4 StartColor    = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
    glm::vec4 EndColor      = glm::vec4(1.0f, 1.0f, 1.0f, 0.0f);
    Range<float> StartRotation   = { 0.0f, 360.0f };
    Range<float> AngularVelocity = { 0.0f, 0.0f };

    // Blending
    ParticleBlendMode BlendMode = ParticleBlendMode::Additive;

    // Texture (optional)
    std::shared_ptr<Texture> ParticleTexture = nullptr;

    // World vs local space
    bool WorldSpace = true;
};

// ---------- Particle Emitter ----------

class ParticleEmitter {
public:
    ParticleEmitter();
    explicit ParticleEmitter(const ParticleEmitterProperties& props);
    ~ParticleEmitter();

    // Core lifecycle
    void Update(float deltaTime);
    void Render(const glm::mat4& viewProjection, const glm::vec3& cameraPosition,
                const glm::vec3& cameraRight, const glm::vec3& cameraUp);

    // Control
    void Play();
    void Pause();
    void Stop();        // stop emitting and clear
    void Burst(int count = -1); // -1 uses BurstCount from props

    // State
    bool IsPlaying() const { return m_Playing; }
    int  ActiveCount() const { return m_ActiveCount; }

    // Transform
    void SetPosition(const glm::vec3& pos) { m_Position = pos; }
    void SetRotation(const glm::vec3& euler) { m_Rotation = euler; }
    void SetScale(const glm::vec3& s) { m_Scale = s; }
    const glm::vec3& GetPosition() const { return m_Position; }

    // Properties (can be changed at runtime)
    ParticleEmitterProperties& GetProperties() { return m_Props; }
    const ParticleEmitterProperties& GetProperties() const { return m_Props; }

private:
    void InitGPU();
    void ShutdownGPU();

    void Emit(int count);
    Particle SpawnParticle();
    glm::vec3 SampleEmitterShape();
    glm::vec3 SampleDirection();

    float RandomFloat(float min, float max);
    template <typename T> T Lerp(const T& a, const T& b, float t) { return a + t * (b - a); }

private:
    ParticleEmitterProperties m_Props;

    // Pool
    std::vector<Particle> m_Particles;
    int m_ActiveCount = 0;

    // State
    bool  m_Playing     = true;
    float m_EmitAccum   = 0.0f;  // fractional particle accumulator

    // Transform
    glm::vec3 m_Position = glm::vec3(0.0f);
    glm::vec3 m_Rotation = glm::vec3(0.0f);
    glm::vec3 m_Scale    = glm::vec3(1.0f);

    // GPU resources
    unsigned int m_VAO = 0;
    unsigned int m_QuadVBO = 0;
    unsigned int m_InstanceVBO = 0;
    std::shared_ptr<Shader> m_Shader;

    // RNG
    std::mt19937 m_RNG;

    // Per-instance data layout: mat4 (model), vec4 (color) = 20 floats per particle
    static constexpr int INSTANCE_FLOAT_COUNT = 20;
};

// ---------- Particle System (manages multiple emitters) ----------

class ParticleSystem {
public:
    ParticleSystem();
    ~ParticleSystem();

    // Create/remove emitters
    ParticleEmitter* AddEmitter(const ParticleEmitterProperties& props = {});
    void RemoveEmitter(ParticleEmitter* emitter);
    void Clear();

    // Lifecycle
    void Update(float deltaTime);
    void Render(const glm::mat4& viewProjection, const glm::vec3& cameraPosition,
                const glm::vec3& cameraRight, const glm::vec3& cameraUp);

    // Global controls
    void PlayAll();
    void PauseAll();
    void StopAll();

    int EmitterCount() const { return static_cast<int>(m_Emitters.size()); }
    int TotalActiveParticles() const;

private:
    std::vector<std::unique_ptr<ParticleEmitter>> m_Emitters;
};

// ---------- Preset factory ----------

namespace ParticlePresets {
    ParticleEmitterProperties Fire();
    ParticleEmitterProperties Smoke();
    ParticleEmitterProperties Sparks();
    ParticleEmitterProperties Snow();
    ParticleEmitterProperties Fountain();
    ParticleEmitterProperties Explosion();
}

} // namespace Kaya
