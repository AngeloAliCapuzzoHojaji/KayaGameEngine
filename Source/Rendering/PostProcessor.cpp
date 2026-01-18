#include "Rendering/PostProcessor.h"
#include "Rendering/Framebuffer.h"
#include "Rendering/Shader.h"
#include <glad/glad.h>
#include <iostream>

namespace Kaya {

PostProcessor::PostProcessor(unsigned int width, unsigned int height)
    : m_Width(width), m_Height(height) {
    
    // Create framebuffers
    m_SceneFB = std::make_shared<Framebuffer>(width, height);
    m_PingPongFB[0] = std::make_shared<Framebuffer>(width, height);
    m_PingPongFB[1] = std::make_shared<Framebuffer>(width, height);
    m_FinalFB = std::make_shared<Framebuffer>(width, height);
    
    // Bloom extraction shader
    std::string bloomVertexSrc = R"(
        #version 460 core
        layout(location = 0) in vec2 a_Position;
        layout(location = 1) in vec2 a_TexCoord;
        
        out vec2 v_TexCoord;
        
        void main() {
            v_TexCoord = a_TexCoord;
            gl_Position = vec4(a_Position, 0.0, 1.0);
        }
    )";
    
    std::string bloomFragmentSrc = R"(
        #version 460 core
        layout(location = 0) out vec4 FragColor;
        
        in vec2 v_TexCoord;
        
        uniform sampler2D u_Texture;
        uniform float u_Threshold;
        
        void main() {
            vec3 color = texture(u_Texture, v_TexCoord).rgb;
            float brightness = dot(color, vec3(0.2126, 0.7152, 0.0722));
            
            if(brightness > u_Threshold)
                FragColor = vec4(color, 1.0);
            else
                FragColor = vec4(0.0, 0.0, 0.0, 1.0);
        }
    )";
    
    m_BloomShader = std::make_shared<Shader>(bloomVertexSrc, bloomFragmentSrc);
    
    // Gaussian blur shader
    std::string blurFragmentSrc = R"(
        #version 460 core
        layout(location = 0) out vec4 FragColor;
        
        in vec2 v_TexCoord;
        
        uniform sampler2D u_Texture;
        uniform bool u_Horizontal;
        
        uniform float weight[5] = float[] (0.227027, 0.1945946, 0.1216216, 0.054054, 0.016216);
        
        void main() {
            vec2 tex_offset = 1.0 / textureSize(u_Texture, 0);
            vec3 result = texture(u_Texture, v_TexCoord).rgb * weight[0];
            
            if(u_Horizontal) {
                for(int i = 1; i < 5; ++i) {
                    result += texture(u_Texture, v_TexCoord + vec2(tex_offset.x * i, 0.0)).rgb * weight[i];
                    result += texture(u_Texture, v_TexCoord - vec2(tex_offset.x * i, 0.0)).rgb * weight[i];
                }
            } else {
                for(int i = 1; i < 5; ++i) {
                    result += texture(u_Texture, v_TexCoord + vec2(0.0, tex_offset.y * i)).rgb * weight[i];
                    result += texture(u_Texture, v_TexCoord - vec2(0.0, tex_offset.y * i)).rgb * weight[i];
                }
            }
            
            FragColor = vec4(result, 1.0);
        }
    )";
    
    m_BlurShader = std::make_shared<Shader>(bloomVertexSrc, blurFragmentSrc);
    
    // Final composition shader
    std::string finalFragmentSrc = R"(
        #version 460 core
        layout(location = 0) out vec4 FragColor;
        
        in vec2 v_TexCoord;
        
        uniform sampler2D u_SceneTexture;
        uniform sampler2D u_BloomTexture;
        uniform bool u_BloomEnabled;
        uniform bool u_ToneMappingEnabled;
        uniform bool u_VignetteEnabled;
        uniform float u_BloomIntensity;
        uniform float u_Exposure;
        uniform float u_VignetteStrength;
        
        vec3 ACESFilm(vec3 x) {
            float a = 2.51;
            float b = 0.03;
            float c = 2.43;
            float d = 0.59;
            float e = 0.14;
            return clamp((x * (a * x + b)) / (x * (c * x + d) + e), 0.0, 1.0);
        }
        
        void main() {
            vec3 hdrColor = texture(u_SceneTexture, v_TexCoord).rgb;
            
            // Bloom
            if(u_BloomEnabled) {
                vec3 bloomColor = texture(u_BloomTexture, v_TexCoord).rgb;
                hdrColor += bloomColor * u_BloomIntensity;
            }
            
            // Tone mapping
            vec3 mapped = hdrColor;
            if(u_ToneMappingEnabled) {
                mapped = vec3(1.0) - exp(-hdrColor * u_Exposure);
                mapped = ACESFilm(mapped);
            }
            
            // Gamma correction
            mapped = pow(mapped, vec3(1.0 / 2.2));
            
            // Vignette
            if(u_VignetteEnabled) {
                vec2 uv = v_TexCoord * 2.0 - 1.0;
                float vignette = 1.0 - dot(uv, uv) * u_VignetteStrength;
                mapped *= vignette;
            }
            
            FragColor = vec4(mapped, 1.0);
        }
    )";
    
    m_FinalShader = std::make_shared<Shader>(bloomVertexSrc, finalFragmentSrc);
    
    InitQuad();
    
    std::cout << "PostProcessor initialized: " << width << "x" << height << std::endl;
}

PostProcessor::~PostProcessor() {
    if (m_QuadVAO) glDeleteVertexArrays(1, &m_QuadVAO);
    if (m_QuadVBO) glDeleteBuffers(1, &m_QuadVBO);
}

void PostProcessor::Resize(unsigned int width, unsigned int height) {
    m_Width = width;
    m_Height = height;
    
    m_Width = width;
    m_Height = height;
    
    m_SceneFB->Resize(width, height);
    m_PingPongFB[0]->Resize(width, height);
    m_PingPongFB[1]->Resize(width, height);
    m_FinalFB->Resize(width, height);
}

void PostProcessor::BeginScene() {
    m_SceneFB->Bind();
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void PostProcessor::EndScene() {
    m_SceneFB->Unbind();
}

void PostProcessor::Process() {
    glDisable(GL_DEPTH_TEST);
    
    // Apply bloom if enabled
    if (m_BloomEnabled) {
        ApplyBloom();
    }
    
    // Final composition
    m_FinalFB->Bind();
    m_FinalShader->Bind();
    m_FinalShader->SetInt("u_SceneTexture", 0);
    m_FinalShader->SetInt("u_BloomTexture", 1);
    m_FinalShader->SetInt("u_BloomEnabled", m_BloomEnabled ? 1 : 0);
    m_FinalShader->SetInt("u_ToneMappingEnabled", m_ToneMappingEnabled ? 1 : 0);
    m_FinalShader->SetInt("u_VignetteEnabled", m_VignetteEnabled ? 1 : 0);
    m_FinalShader->SetFloat("u_BloomIntensity", m_BloomIntensity);
    m_FinalShader->SetFloat("u_Exposure", m_Exposure);
    m_FinalShader->SetFloat("u_VignetteStrength", m_VignetteStrength);
    
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_SceneFB->GetColorAttachment());
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, m_PingPongFB[0]->GetColorAttachment());
    
    RenderQuad();
    m_FinalFB->Unbind();
    
    glEnable(GL_DEPTH_TEST);
}

void PostProcessor::ApplyBloom() {
    // Extract bright areas
    m_PingPongFB[1]->Bind();
    m_BloomShader->Bind();
    m_BloomShader->SetInt("u_Texture", 0);
    m_BloomShader->SetFloat("u_Threshold", m_BloomThreshold);
    
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_SceneFB->GetColorAttachment());
    RenderQuad();
    m_PingPongFB[1]->Unbind();
    
    // Blur bright areas
    bool horizontal = true;
    int amount = 10;
    
    m_BlurShader->Bind();
    for (unsigned int i = 0; i < amount; i++) {
        m_PingPongFB[horizontal]->Bind();
        m_BlurShader->SetInt("u_Horizontal", horizontal ? 1 : 0);
        m_BlurShader->SetInt("u_Texture", 0);
        
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, m_PingPongFB[!horizontal]->GetColorAttachment());
        
        RenderQuad();
        horizontal = !horizontal;
    }
    m_PingPongFB[0]->Unbind();
}

void PostProcessor::InitQuad() {
    float quadVertices[] = {
        // positions   // texCoords
        -1.0f,  1.0f,  0.0f, 1.0f,
        -1.0f, -1.0f,  0.0f, 0.0f,
         1.0f, -1.0f,  1.0f, 0.0f,

        -1.0f,  1.0f,  0.0f, 1.0f,
         1.0f, -1.0f,  1.0f, 0.0f,
         1.0f,  1.0f,  1.0f, 1.0f
    };

    glGenVertexArrays(1, &m_QuadVAO);
    glGenBuffers(1, &m_QuadVBO);
    
    glBindVertexArray(m_QuadVAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_QuadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
    
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
}

void PostProcessor::RenderQuad() {
    glBindVertexArray(m_QuadVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
}

unsigned int PostProcessor::GetFinalTexture() const {
    return m_FinalFB->GetColorAttachment();
}

} // namespace Kaya
