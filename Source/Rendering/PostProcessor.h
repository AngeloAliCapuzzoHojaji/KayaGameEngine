#pragma once

#include <memory>
#include <vector>

namespace Kaya {

class Framebuffer;
class Shader;

enum class PostProcessEffect {
    None,
    Bloom,
    ToneMapping,
    ColorGrading,
    Vignette,
    ChromaticAberration
};

class PostProcessor {
public:
    PostProcessor(unsigned int width, unsigned int height);
    ~PostProcessor();

    void Resize(unsigned int width, unsigned int height);
    
    // Begin rendering to intermediate buffer
    void BeginScene();
    void EndScene();
    
    // Apply post-processing and render to screen
    void Process();
    
    // Enable/disable effects
    void SetBloomEnabled(bool enabled) { m_BloomEnabled = enabled; }
    void SetToneMappingEnabled(bool enabled) { m_ToneMappingEnabled = enabled; }
    void SetVignetteEnabled(bool enabled) { m_VignetteEnabled = enabled; }
    
    // Parameters
    void SetBloomThreshold(float threshold) { m_BloomThreshold = threshold; }
    void SetBloomIntensity(float intensity) { m_BloomIntensity = intensity; }
    void SetExposure(float exposure) { m_Exposure = exposure; }
    void SetVignetteStrength(float strength) { m_VignetteStrength = strength; }

    unsigned int GetFinalTexture() const;

private:
    void InitQuad();
    void ApplyBloom();
    void ApplyToneMapping();
    void RenderQuad();

private:
    unsigned int m_Width, m_Height;
    
    // Framebuffers
    std::shared_ptr<Framebuffer> m_SceneFB;
    std::shared_ptr<Framebuffer> m_PingPongFB[2];
    std::shared_ptr<Framebuffer> m_FinalFB;
    
    // Shaders
    std::shared_ptr<Shader> m_BloomShader;
    std::shared_ptr<Shader> m_BlurShader;
    std::shared_ptr<Shader> m_FinalShader;
    
    // Settings
    bool m_BloomEnabled = true;
    bool m_ToneMappingEnabled = true;
    bool m_VignetteEnabled = false;
    
    float m_BloomThreshold = 1.0f;
    float m_BloomIntensity = 0.5f;
    float m_Exposure = 1.0f;
    float m_VignetteStrength = 0.5f;
    
    // Screen quad
    unsigned int m_QuadVAO = 0;
    unsigned int m_QuadVBO = 0;
};

} // namespace Kaya
