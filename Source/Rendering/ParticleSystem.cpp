#include "Rendering/ParticleSystem.h"
#include "Rendering/Shader.h"
#include "Rendering/Texture.h"
#include "Rendering/Camera.h"
#include <glad/glad.h>
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>
#include <cmath>
#include <iostream>

namespace Kaya {

// ============================================================
//  ParticleEmitter
// ============================================================

ParticleEmitter::ParticleEmitter()
    : m_RNG(std::random_device{}())
{
    m_Props = ParticleEmitterProperties{};
    m_Particles.resize(m_Props.MaxParticles);
    InitGPU();
}

ParticleEmitter::ParticleEmitter(const ParticleEmitterProperties& props)
    : m_Props(props), m_RNG(std::random_device{}())
{
    m_Particles.resize(m_Props.MaxParticles);
    InitGPU();
}

ParticleEmitter::~ParticleEmitter() {
    ShutdownGPU();
}

// ---- GPU setup ----

void ParticleEmitter::InitGPU() {
    // Particle shader (instanced billboards)
    std::string vertSrc = R"(
        #version 460 core
        // Per-vertex (unit quad)
        layout(location = 0) in vec2 a_QuadPos;
        layout(location = 1) in vec2 a_QuadUV;

        // Per-instance
        layout(location = 2) in mat4 a_Model;     // locations 2-5
        layout(location = 6) in vec4 a_Color;

        uniform mat4 u_ViewProjection;

        out vec2 v_UV;
        out vec4 v_Color;

        void main() {
            v_UV    = a_QuadUV;
            v_Color = a_Color;
            gl_Position = u_ViewProjection * a_Model * vec4(a_QuadPos, 0.0, 1.0);
        }
    )";

    std::string fragSrc = R"(
        #version 460 core
        in vec2 v_UV;
        in vec4 v_Color;

        uniform sampler2D u_Texture;
        uniform int u_UseTexture;

        layout(location = 0) out vec4 FragColor;

        void main() {
            vec4 texColor = u_UseTexture > 0 ? texture(u_Texture, v_UV) : vec4(1.0);
            FragColor = texColor * v_Color;

            // Soft-edge disc falloff when no texture
            if (u_UseTexture == 0) {
                float dist = length(v_UV - vec2(0.5));
                float alpha = 1.0 - smoothstep(0.35, 0.5, dist);
                FragColor.a *= alpha;
            }
        }
    )";

    m_Shader = std::make_shared<Shader>(vertSrc, fragSrc);

    // Unit quad vertices:  pos(2) + uv(2) = 4 floats × 6 verts
    float quadVerts[] = {
        // pos          uv
        -0.5f, -0.5f,  0.0f, 0.0f,
         0.5f, -0.5f,  1.0f, 0.0f,
         0.5f,  0.5f,  1.0f, 1.0f,

        -0.5f, -0.5f,  0.0f, 0.0f,
         0.5f,  0.5f,  1.0f, 1.0f,
        -0.5f,  0.5f,  0.0f, 1.0f
    };

    glGenVertexArrays(1, &m_VAO);
    glGenBuffers(1, &m_QuadVBO);
    glGenBuffers(1, &m_InstanceVBO);

    glBindVertexArray(m_VAO);

    // --- Quad VBO ---
    glBindBuffer(GL_ARRAY_BUFFER, m_QuadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVerts), quadVerts, GL_STATIC_DRAW);

    // location 0: quad position (vec2)
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);

    // location 1: quad UV (vec2)
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));

    // --- Instance VBO (dynamic) ---
    glBindBuffer(GL_ARRAY_BUFFER, m_InstanceVBO);
    glBufferData(GL_ARRAY_BUFFER,
                 m_Props.MaxParticles * INSTANCE_FLOAT_COUNT * sizeof(float),
                 nullptr, GL_DYNAMIC_DRAW);

    std::size_t stride = INSTANCE_FLOAT_COUNT * sizeof(float);

    // location 2-5: model matrix (mat4, takes 4 attribute slots)
    for (int i = 0; i < 4; ++i) {
        glEnableVertexAttribArray(2 + i);
        glVertexAttribPointer(2 + i, 4, GL_FLOAT, GL_FALSE,
                              static_cast<GLsizei>(stride),
                              (void*)(i * 4 * sizeof(float)));
        glVertexAttribDivisor(2 + i, 1);
    }

    // location 6: color (vec4)
    glEnableVertexAttribArray(6);
    glVertexAttribPointer(6, 4, GL_FLOAT, GL_FALSE,
                          static_cast<GLsizei>(stride),
                          (void*)(16 * sizeof(float)));
    glVertexAttribDivisor(6, 1);

    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void ParticleEmitter::ShutdownGPU() {
    if (m_VAO) { glDeleteVertexArrays(1, &m_VAO); m_VAO = 0; }
    if (m_QuadVBO) { glDeleteBuffers(1, &m_QuadVBO); m_QuadVBO = 0; }
    if (m_InstanceVBO) { glDeleteBuffers(1, &m_InstanceVBO); m_InstanceVBO = 0; }
}

// ---- Helpers ----

float ParticleEmitter::RandomFloat(float min, float max) {
    std::uniform_real_distribution<float> dist(min, max);
    return dist(m_RNG);
}

glm::vec3 ParticleEmitter::SampleEmitterShape() {
    switch (m_Props.Shape) {
        case EmitterShape::Point:
            return glm::vec3(0.0f);

        case EmitterShape::Sphere: {
            float u = RandomFloat(0.0f, 1.0f);
            float theta = RandomFloat(0.0f, 2.0f * 3.14159265f);
            float phi   = std::acos(2.0f * RandomFloat(0.0f, 1.0f) - 1.0f);
            float r = m_Props.ShapeRadius * std::cbrt(u);
            return glm::vec3(
                r * std::sin(phi) * std::cos(theta),
                r * std::sin(phi) * std::sin(theta),
                r * std::cos(phi)
            );
        }

        case EmitterShape::Hemisphere: {
            float theta = RandomFloat(0.0f, 2.0f * 3.14159265f);
            float phi   = RandomFloat(0.0f, 3.14159265f * 0.5f);
            float r = m_Props.ShapeRadius * std::cbrt(RandomFloat(0.0f, 1.0f));
            return glm::vec3(
                r * std::sin(phi) * std::cos(theta),
                r * std::cos(phi),
                r * std::sin(phi) * std::sin(theta)
            );
        }

        case EmitterShape::Cone: {
            float halfAngle = glm::radians(m_Props.ConeAngle);
            float theta = RandomFloat(0.0f, 2.0f * 3.14159265f);
            float phi   = RandomFloat(0.0f, halfAngle);
            float r = m_Props.ShapeRadius * RandomFloat(0.0f, 1.0f);
            return glm::vec3(
                r * std::sin(phi) * std::cos(theta),
                r * std::cos(phi),
                r * std::sin(phi) * std::sin(theta)
            );
        }

        case EmitterShape::Box: {
            return glm::vec3(
                RandomFloat(-m_Props.BoxExtents.x, m_Props.BoxExtents.x),
                RandomFloat(-m_Props.BoxExtents.y, m_Props.BoxExtents.y),
                RandomFloat(-m_Props.BoxExtents.z, m_Props.BoxExtents.z)
            );
        }
    }
    return glm::vec3(0.0f);
}

glm::vec3 ParticleEmitter::SampleDirection() {
    glm::vec3 dir = glm::normalize(m_Props.Direction);
    if (m_Props.DirectionSpread > 0.0f) {
        float angleOff = RandomFloat(0.0f, m_Props.DirectionSpread);
        float rotAngle = RandomFloat(0.0f, 2.0f * 3.14159265f);

        // Build an orthonormal basis around dir
        glm::vec3 tangent;
        if (std::abs(dir.x) < 0.9f)
            tangent = glm::normalize(glm::cross(dir, glm::vec3(1, 0, 0)));
        else
            tangent = glm::normalize(glm::cross(dir, glm::vec3(0, 1, 0)));
        glm::vec3 bitangent = glm::cross(dir, tangent);

        dir = glm::normalize(
            dir * std::cos(angleOff) +
            (tangent * std::cos(rotAngle) + bitangent * std::sin(rotAngle)) * std::sin(angleOff)
        );
    }
    return dir;
}

Particle ParticleEmitter::SpawnParticle() {
    Particle p;
    p.Position = m_Props.WorldSpace ? m_Position + SampleEmitterShape()
                                     : SampleEmitterShape();
    float speed = RandomFloat(m_Props.Speed.Min, m_Props.Speed.Max);
    p.Velocity = SampleDirection() * speed;
    p.MaxLife  = RandomFloat(m_Props.Lifetime.Min, m_Props.Lifetime.Max);
    p.Life     = p.MaxLife;
    p.Size     = RandomFloat(m_Props.StartSize.Min, m_Props.StartSize.Max);
    p.Color    = m_Props.StartColor;
    p.Rotation = RandomFloat(m_Props.StartRotation.Min, m_Props.StartRotation.Max);
    p.AngularVelocity = RandomFloat(m_Props.AngularVelocity.Min, m_Props.AngularVelocity.Max);
    return p;
}

// ---- Emission ----

void ParticleEmitter::Emit(int count) {
    for (int i = 0; i < count && m_ActiveCount < m_Props.MaxParticles; ++i) {
        m_Particles[m_ActiveCount] = SpawnParticle();
        ++m_ActiveCount;
    }
}

void ParticleEmitter::Burst(int count) {
    int n = (count < 0) ? m_Props.BurstCount : count;
    Emit(n);
}

void ParticleEmitter::Play()  { m_Playing = true; }
void ParticleEmitter::Pause() { m_Playing = false; }
void ParticleEmitter::Stop()  { m_Playing = false; m_ActiveCount = 0; }

// ---- Update ----

void ParticleEmitter::Update(float deltaTime) {
    if (!m_Playing) return;

    // Continuous emission
    if (m_Props.EmissionRate > 0.0f && m_Props.BurstCount == 0) {
        m_EmitAccum += m_Props.EmissionRate * deltaTime;
        int toEmit = static_cast<int>(m_EmitAccum);
        if (toEmit > 0) {
            Emit(toEmit);
            m_EmitAccum -= static_cast<float>(toEmit);
        }
    }

    // Simulate
    for (int i = 0; i < m_ActiveCount; ) {
        Particle& p = m_Particles[i];
        p.Life -= deltaTime;
        if (p.Life <= 0.0f) {
            // Swap with last alive
            m_Particles[i] = m_Particles[m_ActiveCount - 1];
            --m_ActiveCount;
            continue; // re-check index
        }

        // Physics
        p.Velocity += m_Props.Gravity * deltaTime;
        if (m_Props.Drag > 0.0f) {
            p.Velocity *= 1.0f / (1.0f + m_Props.Drag * deltaTime);
        }
        p.Position += p.Velocity * deltaTime;
        p.Rotation += p.AngularVelocity * deltaTime;

        // Interpolate color and size over lifetime
        float t = 1.0f - (p.Life / p.MaxLife); // 0→1 as particle ages
        p.Color = Lerp(m_Props.StartColor, m_Props.EndColor, t);
        float startSz = (m_Props.StartSize.Min + m_Props.StartSize.Max) * 0.5f;
        float endSz   = (m_Props.EndSize.Min   + m_Props.EndSize.Max)   * 0.5f;
        p.Size  = Lerp(startSz, endSz, t);

        ++i;
    }
}

// ---- Render ----

void ParticleEmitter::Render(const glm::mat4& viewProjection, const glm::vec3& cameraPosition,
                              const glm::vec3& cameraRight, const glm::vec3& cameraUp) {
    if (m_ActiveCount == 0) return;

    // Sort back-to-front for correct alpha blending
    std::sort(m_Particles.begin(), m_Particles.begin() + m_ActiveCount,
        [&cameraPosition](const Particle& a, const Particle& b) {
            float da = glm::dot(a.Position - cameraPosition, a.Position - cameraPosition);
            float db = glm::dot(b.Position - cameraPosition, b.Position - cameraPosition);
            return da > db; // far first
        });

    // Build instance buffer
    std::vector<float> instanceData(m_ActiveCount * INSTANCE_FLOAT_COUNT);
    for (int i = 0; i < m_ActiveCount; ++i) {
        const Particle& p = m_Particles[i];

        // Billboard model matrix
        glm::mat4 model(1.0f);
        model[0] = glm::vec4(cameraRight * p.Size, 0.0f);
        model[1] = glm::vec4(cameraUp * p.Size, 0.0f);
        model[2] = glm::vec4(glm::normalize(glm::cross(cameraRight, cameraUp)) * p.Size, 0.0f);
        model[3] = glm::vec4(p.Position, 1.0f);

        // Apply rotation around the billboard face normal (Z axis in billboard space)
        if (std::abs(p.Rotation) > 0.001f) {
            float rad = glm::radians(p.Rotation);
            float c = std::cos(rad);
            float s = std::sin(rad);
            glm::vec4 r0 = model[0];
            glm::vec4 r1 = model[1];
            model[0] = r0 * c + r1 * s;
            model[1] = -r0 * s + r1 * c;
        }

        int base = i * INSTANCE_FLOAT_COUNT;
        // mat4 column-major (16 floats)
        for (int col = 0; col < 4; ++col)
            for (int row = 0; row < 4; ++row)
                instanceData[base + col * 4 + row] = model[col][row];
        // color (4 floats)
        instanceData[base + 16] = p.Color.r;
        instanceData[base + 17] = p.Color.g;
        instanceData[base + 18] = p.Color.b;
        instanceData[base + 19] = p.Color.a;
    }

    // Upload
    glBindBuffer(GL_ARRAY_BUFFER, m_InstanceVBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0,
                    m_ActiveCount * INSTANCE_FLOAT_COUNT * sizeof(float),
                    instanceData.data());
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    // Render state
    glDepthMask(GL_FALSE);  // don't write depth — particles are transparent
    glEnable(GL_BLEND);
    if (m_Props.BlendMode == ParticleBlendMode::Additive)
        glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    else
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Disable face culling for billboards
    glDisable(GL_CULL_FACE);

    m_Shader->Bind();
    m_Shader->SetMat4("u_ViewProjection", viewProjection);
    m_Shader->SetInt("u_UseTexture", m_Props.ParticleTexture ? 1 : 0);
    if (m_Props.ParticleTexture) {
        m_Props.ParticleTexture->Bind(0);
        m_Shader->SetInt("u_Texture", 0);
    }

    glBindVertexArray(m_VAO);
    glDrawArraysInstanced(GL_TRIANGLES, 0, 6, m_ActiveCount);
    glBindVertexArray(0);

    m_Shader->Unbind();

    // Restore state
    glEnable(GL_CULL_FACE);
    glDepthMask(GL_TRUE);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

// ============================================================
//  ParticleSystem
// ============================================================

ParticleSystem::ParticleSystem()  = default;
ParticleSystem::~ParticleSystem() = default;

ParticleEmitter* ParticleSystem::AddEmitter(const ParticleEmitterProperties& props) {
    m_Emitters.push_back(std::make_unique<ParticleEmitter>(props));
    return m_Emitters.back().get();
}

void ParticleSystem::RemoveEmitter(ParticleEmitter* emitter) {
    m_Emitters.erase(
        std::remove_if(m_Emitters.begin(), m_Emitters.end(),
            [emitter](const std::unique_ptr<ParticleEmitter>& e) { return e.get() == emitter; }),
        m_Emitters.end());
}

void ParticleSystem::Clear() {
    m_Emitters.clear();
}

void ParticleSystem::Update(float deltaTime) {
    for (auto& e : m_Emitters)
        e->Update(deltaTime);
}

void ParticleSystem::Render(const glm::mat4& viewProjection, const glm::vec3& cameraPosition,
                             const glm::vec3& cameraRight, const glm::vec3& cameraUp) {
    for (auto& e : m_Emitters)
        e->Render(viewProjection, cameraPosition, cameraRight, cameraUp);
}

void ParticleSystem::PlayAll()  { for (auto& e : m_Emitters) e->Play(); }
void ParticleSystem::PauseAll() { for (auto& e : m_Emitters) e->Pause(); }
void ParticleSystem::StopAll()  { for (auto& e : m_Emitters) e->Stop(); }

int ParticleSystem::TotalActiveParticles() const {
    int total = 0;
    for (auto& e : m_Emitters) total += e->ActiveCount();
    return total;
}

// ============================================================
//  Presets
// ============================================================

namespace ParticlePresets {

ParticleEmitterProperties Fire() {
    ParticleEmitterProperties p;
    p.EmissionRate  = 80.0f;
    p.MaxParticles  = 500;
    p.Shape         = EmitterShape::Sphere;
    p.ShapeRadius   = 0.3f;
    p.Lifetime      = { 0.5f, 1.2f };
    p.Speed         = { 1.0f, 2.5f };
    p.Direction     = glm::vec3(0.0f, 1.0f, 0.0f);
    p.DirectionSpread = 0.3f;
    p.Gravity       = glm::vec3(0.0f, 1.0f, 0.0f); // upward buoyancy
    p.Drag          = 0.5f;
    p.StartSize     = { 0.15f, 0.3f };
    p.EndSize       = { 0.0f, 0.05f };
    p.StartColor    = glm::vec4(1.0f, 0.6f, 0.1f, 0.9f);
    p.EndColor      = glm::vec4(1.0f, 0.1f, 0.0f, 0.0f);
    p.BlendMode     = ParticleBlendMode::Additive;
    return p;
}

ParticleEmitterProperties Smoke() {
    ParticleEmitterProperties p;
    p.EmissionRate  = 30.0f;
    p.MaxParticles  = 300;
    p.Shape         = EmitterShape::Sphere;
    p.ShapeRadius   = 0.2f;
    p.Lifetime      = { 2.0f, 4.0f };
    p.Speed         = { 0.3f, 0.8f };
    p.Direction     = glm::vec3(0.0f, 1.0f, 0.0f);
    p.DirectionSpread = 0.4f;
    p.Gravity       = glm::vec3(0.0f, 0.2f, 0.0f);
    p.Drag          = 1.0f;
    p.StartSize     = { 0.2f, 0.4f };
    p.EndSize       = { 0.8f, 1.2f };
    p.StartColor    = glm::vec4(0.5f, 0.5f, 0.5f, 0.6f);
    p.EndColor      = glm::vec4(0.3f, 0.3f, 0.3f, 0.0f);
    p.AngularVelocity = { -30.0f, 30.0f };
    p.BlendMode     = ParticleBlendMode::Alpha;
    return p;
}

ParticleEmitterProperties Sparks() {
    ParticleEmitterProperties p;
    p.EmissionRate  = 50.0f;
    p.MaxParticles  = 200;
    p.Shape         = EmitterShape::Point;
    p.Lifetime      = { 0.3f, 0.8f };
    p.Speed         = { 3.0f, 8.0f };
    p.Direction     = glm::vec3(0.0f, 1.0f, 0.0f);
    p.DirectionSpread = 1.2f;
    p.Gravity       = glm::vec3(0.0f, -9.81f, 0.0f);
    p.Drag          = 0.2f;
    p.StartSize     = { 0.02f, 0.06f };
    p.EndSize       = { 0.0f, 0.01f };
    p.StartColor    = glm::vec4(1.0f, 0.9f, 0.5f, 1.0f);
    p.EndColor      = glm::vec4(1.0f, 0.3f, 0.0f, 0.0f);
    p.BlendMode     = ParticleBlendMode::Additive;
    return p;
}

ParticleEmitterProperties Snow() {
    ParticleEmitterProperties p;
    p.EmissionRate  = 60.0f;
    p.MaxParticles  = 2000;
    p.Shape         = EmitterShape::Box;
    p.BoxExtents    = glm::vec3(15.0f, 0.0f, 15.0f);
    p.Lifetime      = { 5.0f, 10.0f };
    p.Speed         = { 0.1f, 0.3f };
    p.Direction     = glm::vec3(0.0f, -1.0f, 0.0f);
    p.DirectionSpread = 0.2f;
    p.Gravity       = glm::vec3(0.0f, -0.5f, 0.0f);
    p.Drag          = 2.0f;
    p.StartSize     = { 0.03f, 0.08f };
    p.EndSize       = { 0.03f, 0.08f };
    p.StartColor    = glm::vec4(1.0f, 1.0f, 1.0f, 0.9f);
    p.EndColor      = glm::vec4(1.0f, 1.0f, 1.0f, 0.0f);
    p.AngularVelocity = { -20.0f, 20.0f };
    p.BlendMode     = ParticleBlendMode::Alpha;
    return p;
}

ParticleEmitterProperties Fountain() {
    ParticleEmitterProperties p;
    p.EmissionRate  = 100.0f;
    p.MaxParticles  = 1000;
    p.Shape         = EmitterShape::Cone;
    p.ShapeRadius   = 0.1f;
    p.ConeAngle     = 15.0f;
    p.Lifetime      = { 1.5f, 3.0f };
    p.Speed         = { 5.0f, 8.0f };
    p.Direction     = glm::vec3(0.0f, 1.0f, 0.0f);
    p.DirectionSpread = 0.15f;
    p.Gravity       = glm::vec3(0.0f, -9.81f, 0.0f);
    p.Drag          = 0.1f;
    p.StartSize     = { 0.04f, 0.08f };
    p.EndSize       = { 0.02f, 0.04f };
    p.StartColor    = glm::vec4(0.3f, 0.6f, 1.0f, 0.8f);
    p.EndColor      = glm::vec4(0.1f, 0.3f, 0.8f, 0.0f);
    p.BlendMode     = ParticleBlendMode::Alpha;
    return p;
}

ParticleEmitterProperties Explosion() {
    ParticleEmitterProperties p;
    p.EmissionRate  = 0.0f;   // burst only
    p.BurstCount    = 200;
    p.MaxParticles  = 200;
    p.Shape         = EmitterShape::Point;
    p.Lifetime      = { 0.5f, 1.5f };
    p.Speed         = { 3.0f, 10.0f };
    p.Direction     = glm::vec3(0.0f, 1.0f, 0.0f);
    p.DirectionSpread = 3.14159f; // full sphere
    p.Gravity       = glm::vec3(0.0f, -4.0f, 0.0f);
    p.Drag          = 1.5f;
    p.StartSize     = { 0.15f, 0.4f };
    p.EndSize       = { 0.0f, 0.05f };
    p.StartColor    = glm::vec4(1.0f, 0.8f, 0.2f, 1.0f);
    p.EndColor      = glm::vec4(0.8f, 0.2f, 0.0f, 0.0f);
    p.BlendMode     = ParticleBlendMode::Additive;
    return p;
}

} // namespace ParticlePresets

} // namespace Kaya
