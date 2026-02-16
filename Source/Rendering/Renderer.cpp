#include "Rendering/Renderer.h"
#include "Rendering/Shader.h"
#include "Rendering/Camera.h"
#include "Rendering/Texture.h"
#include "Rendering/Model.h"
#include "Rendering/Mesh.h"
#include "Rendering/Light.h"
#include "Rendering/PBRMaterial.h"
#include "Rendering/Skybox.h"
#include "Rendering/ParticleSystem.h"
#include "Rendering/GPUMetrics.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>
#include <cmath>
#include <iostream>
#include <stdexcept>

namespace Kaya {

Renderer::RendererData* Renderer::s_Data = nullptr;

void Renderer::Init() {
    s_Data = new RendererData();

    // Initialize OpenGL
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        throw std::runtime_error("Failed to initialize GLAD");
    }

    std::cout << "OpenGL Info:" << std::endl;
    std::cout << "  Vendor: " << glGetString(GL_VENDOR) << std::endl;
    std::cout << "  Renderer: " << glGetString(GL_RENDERER) << std::endl;
    std::cout << "  Version: " << glGetString(GL_VERSION) << std::endl;

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Face culling — skip back-facing triangles
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);

    // Basic shader
    std::string vertexSrc = R"(
        #version 460 core
        layout(location = 0) in vec3 a_Position;
        layout(location = 1) in vec3 a_Normal;
        layout(location = 2) in vec2 a_TexCoord;
        
        uniform mat4 u_ViewProjection;
        uniform mat4 u_Transform;
        uniform mat3 u_NormalMatrix;
        uniform mat4 u_LightSpaceMatrix;
        
        out vec3 v_Normal;
        out vec3 v_FragPos;
        out vec2 v_TexCoord;
        out vec4 v_FragPosLightSpace;
        
        void main() {
            v_FragPos = vec3(u_Transform * vec4(a_Position, 1.0));
            v_Normal = u_NormalMatrix * a_Normal;
            v_TexCoord = a_TexCoord;
            v_FragPosLightSpace = u_LightSpaceMatrix * vec4(v_FragPos, 1.0);
            gl_Position = u_ViewProjection * vec4(v_FragPos, 1.0);
        }
    )";

    std::string fragmentSrc = R"(
        #version 460 core
        layout(location = 0) out vec4 color;
        
        in vec3 v_Normal;
        in vec3 v_FragPos;
        in vec2 v_TexCoord;
        in vec4 v_FragPosLightSpace;
        
        uniform vec4 u_Color;
        uniform vec3 u_LightDir;
        uniform vec3 u_LightColor;
        uniform sampler2D u_Texture;
        uniform sampler2D u_ShadowMap;
        uniform int u_UseTexture;
        uniform int u_UseShadows;
        
        float ShadowCalculation(vec4 fragPosLightSpace) {
            vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
            projCoords = projCoords * 0.5 + 0.5;
            
            if(projCoords.z > 1.0)
                return 0.0;
            
            float closestDepth = texture(u_ShadowMap, projCoords.xy).r;
            float currentDepth = projCoords.z;
            
            vec3 normal = normalize(v_Normal);
            vec3 lightDir = normalize(-u_LightDir);
            float bias = max(0.005 * (1.0 - dot(normal, lightDir)), 0.001);
            
            float shadow = 0.0;
            vec2 texelSize = 1.0 / textureSize(u_ShadowMap, 0);
            for(int x = -1; x <= 1; ++x) {
                for(int y = -1; y <= 1; ++y) {
                    float pcfDepth = texture(u_ShadowMap, projCoords.xy + vec2(x, y) * texelSize).r;
                    shadow += currentDepth - bias > pcfDepth ? 1.0 : 0.0;
                }
            }
            shadow /= 9.0;
            
            return shadow;
        }
        
        void main() {
            vec3 norm = normalize(v_Normal);
            vec3 lightDir = normalize(-u_LightDir);
            float diff = max(dot(norm, lightDir), 0.0);
            
            vec4 texColor = u_UseTexture > 0 ? texture(u_Texture, v_TexCoord) : u_Color;
            
            float shadow = u_UseShadows > 0 ? ShadowCalculation(v_FragPosLightSpace) : 0.0;
            
            vec3 ambient = 0.3 * texColor.rgb * u_LightColor;
            vec3 diffuse = (1.0 - shadow) * diff * texColor.rgb * u_LightColor;
            color = vec4(ambient + diffuse, texColor.a);
        }
    )";

    s_Data->BasicShader = std::make_shared<Shader>(vertexSrc, fragmentSrc);

    // Debug line shader (for AABB wireframes)
    std::string debugLineVertSrc = R"(
        #version 460 core
        layout(location = 0) in vec3 a_Position;
        layout(location = 1) in vec4 a_Color;
        uniform mat4 u_ViewProjection;
        out vec4 v_Color;
        void main() {
            v_Color = a_Color;
            gl_Position = u_ViewProjection * vec4(a_Position, 1.0);
        }
    )";
    std::string debugLineFragSrc = R"(
        #version 460 core
        in vec4 v_Color;
        layout(location = 0) out vec4 FragColor;
        void main() {
            FragColor = v_Color;
        }
    )";
    s_Data->DebugLineShader = std::make_shared<Shader>(debugLineVertSrc, debugLineFragSrc);

    // Debug overlay shader (flat color for back-face highlights)
    std::string debugOverlayVertSrc = R"(
        #version 460 core
        layout(location = 0) in vec3 a_Position;
        layout(location = 1) in vec3 a_Normal;
        layout(location = 2) in vec2 a_TexCoord;
        uniform mat4 u_ViewProjection;
        uniform mat4 u_Transform;
        void main() {
            gl_Position = u_ViewProjection * u_Transform * vec4(a_Position, 1.0);
        }
    )";
    std::string debugOverlayFragSrc = R"(
        #version 460 core
        layout(location = 0) out vec4 FragColor;
        uniform vec4 u_OverlayColor;
        void main() {
            FragColor = u_OverlayColor;
        }
    )";
    s_Data->DebugOverlayShader = std::make_shared<Shader>(debugOverlayVertSrc, debugOverlayFragSrc);

    // Debug line VAO/VBO (dynamic)
    glGenVertexArrays(1, &s_Data->DebugLineVAO);
    glGenBuffers(1, &s_Data->DebugLineVBO);
    glBindVertexArray(s_Data->DebugLineVAO);
    glBindBuffer(GL_ARRAY_BUFFER, s_Data->DebugLineVBO);
    // position (3 floats) + color (4 floats) = 7 floats per vertex
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 7 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 7 * sizeof(float), (void*)(3 * sizeof(float)));
    glBindVertexArray(0);

    // Shadow depth shader
    std::string shadowVertexSrc = R"(
        #version 460 core
        layout(location = 0) in vec3 a_Position;
        
        uniform mat4 u_LightSpaceMatrix;
        uniform mat4 u_Transform;
        
        void main() {
            gl_Position = u_LightSpaceMatrix * u_Transform * vec4(a_Position, 1.0);
        }
    )";
    
    std::string shadowFragmentSrc = R"(
        #version 460 core
        
        void main() {
            // Depth is automatically written
        }
    )";
    
    s_Data->ShadowShader = std::make_shared<Shader>(shadowVertexSrc, shadowFragmentSrc);
    
    // PBR shader
    std::string pbrVertexSrc = R"(
        #version 460 core
        layout(location = 0) in vec3 a_Position;
        layout(location = 1) in vec3 a_Normal;
        layout(location = 2) in vec2 a_TexCoord;
        
        uniform mat4 u_ViewProjection;
        uniform mat4 u_Transform;
        
        out vec3 v_WorldPos;
        out vec3 v_Normal;
        out vec2 v_TexCoord;
        
        void main() {
            v_WorldPos = vec3(u_Transform * vec4(a_Position, 1.0));
            v_Normal = mat3(transpose(inverse(u_Transform))) * a_Normal;
            v_TexCoord = a_TexCoord;
            gl_Position = u_ViewProjection * vec4(v_WorldPos, 1.0);
        }
    )";
    
    std::string pbrFragmentSrc = R"(
        #version 460 core
        layout(location = 0) out vec4 FragColor;
        
        in vec3 v_WorldPos;
        in vec3 v_Normal;
        in vec2 v_TexCoord;
        
        // Material parameters
        uniform vec3 u_Albedo;
        uniform float u_Metallic;
        uniform float u_Roughness;
        uniform float u_AO;
        
        // Textures
        uniform sampler2D u_AlbedoMap;
        uniform sampler2D u_MetallicMap;
        uniform sampler2D u_RoughnessMap;
        uniform sampler2D u_AOMap;
        uniform sampler2D u_NormalMap;
        
        uniform int u_UseAlbedoMap;
        uniform int u_UseMetallicMap;
        uniform int u_UseRoughnessMap;
        uniform int u_UseAOMap;
        uniform int u_UseNormalMap;
        
        // Lighting
        uniform vec3 u_LightDir;
        uniform vec3 u_LightColor;
        uniform vec3 u_CamPos;
        
        const float PI = 3.14159265359;
        
        // Normal Distribution Function
        float DistributionGGX(vec3 N, vec3 H, float roughness) {
            float a = roughness * roughness;
            float a2 = a * a;
            float NdotH = max(dot(N, H), 0.0);
            float NdotH2 = NdotH * NdotH;
            
            float nom = a2;
            float denom = (NdotH2 * (a2 - 1.0) + 1.0);
            denom = PI * denom * denom;
            
            return nom / denom;
        }
        
        // Geometry Function
        float GeometrySchlickGGX(float NdotV, float roughness) {
            float r = (roughness + 1.0);
            float k = (r * r) / 8.0;
            
            float nom = NdotV;
            float denom = NdotV * (1.0 - k) + k;
            
            return nom / denom;
        }
        
        float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness) {
            float NdotV = max(dot(N, V), 0.0);
            float NdotL = max(dot(N, L), 0.0);
            float ggx2 = GeometrySchlickGGX(NdotV, roughness);
            float ggx1 = GeometrySchlickGGX(NdotL, roughness);
            
            return ggx1 * ggx2;
        }
        
        // Fresnel-Schlick Approximation
        vec3 fresnelSchlick(float cosTheta, vec3 F0) {
            return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
        }
        
        void main() {
            // Sample material properties
            vec3 albedo = u_UseAlbedoMap > 0 ? pow(texture(u_AlbedoMap, v_TexCoord).rgb, vec3(2.2)) : u_Albedo;
            float metallic = u_UseMetallicMap > 0 ? texture(u_MetallicMap, v_TexCoord).r : u_Metallic;
            float roughness = u_UseRoughnessMap > 0 ? texture(u_RoughnessMap, v_TexCoord).r : u_Roughness;
            float ao = u_UseAOMap > 0 ? texture(u_AOMap, v_TexCoord).r : u_AO;
            
            vec3 N = normalize(v_Normal);
            vec3 V = normalize(u_CamPos - v_WorldPos);
            
            // Calculate reflectance at normal incidence
            vec3 F0 = vec3(0.04);
            F0 = mix(F0, albedo, metallic);
            
            // Reflectance equation
            vec3 Lo = vec3(0.0);
            
            // Directional light
            vec3 L = normalize(-u_LightDir);
            vec3 H = normalize(V + L);
            vec3 radiance = u_LightColor;
            
            // Cook-Torrance BRDF
            float NDF = DistributionGGX(N, H, roughness);
            float G = GeometrySmith(N, V, L, roughness);
            vec3 F = fresnelSchlick(max(dot(H, V), 0.0), F0);
            
            vec3 numerator = NDF * G * F;
            float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001;
            vec3 specular = numerator / denominator;
            
            vec3 kS = F;
            vec3 kD = vec3(1.0) - kS;
            kD *= 1.0 - metallic;
            
            float NdotL = max(dot(N, L), 0.0);
            Lo += (kD * albedo / PI + specular) * radiance * NdotL;
            
            // Ambient lighting
            vec3 ambient = vec3(0.03) * albedo * ao;
            vec3 color = ambient + Lo;
            
            // HDR tonemapping
            color = color / (color + vec3(1.0));
            // Gamma correction
            color = pow(color, vec3(1.0/2.2));
            
            FragColor = vec4(color, 1.0);
        }
    )";
    
    s_Data->PBRShader = std::make_shared<Shader>(pbrVertexSrc, pbrFragmentSrc);
    
    // Skybox shader
    std::string skyboxVertexSrc = R"(
        #version 460 core
        layout (location = 0) in vec3 a_Position;

        out vec3 v_TexCoords;

        uniform mat4 u_Projection;
        uniform mat4 u_View;

        void main()
        {
            v_TexCoords = a_Position;
            vec4 pos = u_Projection * u_View * vec4(a_Position, 1.0);
            gl_Position = pos.xyww;
        }
    )";
    
    std::string skyboxFragmentSrc = R"(
        #version 460 core
        out vec4 FragColor;

        in vec3 v_TexCoords;

        uniform samplerCube u_Skybox;

        void main()
        {    
            FragColor = texture(u_Skybox, v_TexCoords);
        }
    )";
    
    s_Data->SkyboxShader = std::make_shared<Shader>(skyboxVertexSrc, skyboxFragmentSrc);

    // Depth-only pre-pass shader
    std::string depthVertexSrc = R"(
        #version 460 core
        layout(location = 0) in vec3 a_Position;

        uniform mat4 u_ViewProjection;
        uniform mat4 u_Transform;

        void main() {
            gl_Position = u_ViewProjection * u_Transform * vec4(a_Position, 1.0);
        }
    )";

    std::string depthFragmentSrc = R"(
        #version 460 core
        void main() {
            // depth is written automatically
        }
    )";

    s_Data->DepthOnlyShader = std::make_shared<Shader>(depthVertexSrc, depthFragmentSrc);
    
    InitCube();
    InitSphere();
}

void Renderer::Shutdown() {
    glDeleteVertexArrays(1, &s_Data->CubeVAO);
    glDeleteBuffers(1, &s_Data->CubeVBO);
    glDeleteVertexArrays(1, &s_Data->SphereVAO);
    glDeleteBuffers(1, &s_Data->SphereVBO);
    glDeleteBuffers(1, &s_Data->SphereEBO);
    glDeleteVertexArrays(1, &s_Data->DebugLineVAO);
    glDeleteBuffers(1, &s_Data->DebugLineVBO);
    delete s_Data;
}

void Renderer::BeginScene(Camera& camera) {
    s_Data->ViewProjectionMatrix = camera.GetViewProjectionMatrix();
    s_Data->ViewMatrix = camera.GetViewMatrix();
    s_Data->ProjectionMatrix = camera.GetProjectionMatrix();
    s_Data->CameraPosition = camera.GetPosition();

    // Extract frustum planes for CPU-side culling
    s_Data->SceneFrustum.ExtractPlanes(s_Data->ViewProjectionMatrix);

    s_Data->BasicShader->Bind();
    GPUMetricsManager::RecordShaderBind();
    s_Data->BasicShader->SetMat4("u_ViewProjection", s_Data->ViewProjectionMatrix);

    // Clear debug line buffer for new frame
    s_Data->DebugLineVertices.clear();

    // Set light properties
    if (s_Data->DirectionalLight) {
        s_Data->BasicShader->SetFloat3("u_LightDir", s_Data->DirectionalLight->Direction);
        s_Data->BasicShader->SetFloat3("u_LightColor", s_Data->DirectionalLight->Color * s_Data->DirectionalLight->Intensity);
        
        if (s_Data->DirectionalLight->CastShadows && s_Data->ShadowMap) {
            s_Data->BasicShader->SetMat4("u_LightSpaceMatrix", s_Data->DirectionalLight->GetLightSpaceMatrix());
            s_Data->BasicShader->SetInt("u_UseShadows", 1);
            s_Data->BasicShader->SetInt("u_ShadowMap", 1);
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, s_Data->ShadowMap->GetDepthMapTexture());
        } else {
            s_Data->BasicShader->SetInt("u_UseShadows", 0);
        }
    } else {
        // Default light
        s_Data->BasicShader->SetFloat3("u_LightDir", glm::vec3(-0.3f, -1.0f, -0.3f));
        s_Data->BasicShader->SetFloat3("u_LightColor", glm::vec3(1.0f));
        s_Data->BasicShader->SetInt("u_UseShadows", 0);
    }
}

void Renderer::EndScene() {
    s_Data->BasicShader->Unbind();

    // Flush debug wireframes
    if (s_Data->DebugCullingMode) {
        FlushDebugDraw();
    }
}

void Renderer::Clear(const glm::vec4& color) {
    glClearColor(color.r, color.g, color.b, color.a);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void Renderer::SetViewport(unsigned int x, unsigned int y, unsigned int width, unsigned int height) {
    glViewport(x, y, width, height);
}

void Renderer::InitCube() {
    // All faces use CCW winding when viewed from outside.
    // Verified via cross product: (v1-v0)x(v2-v0) matches outward normal.
    float vertices[] = {
        // positions          // normals           // texture coords
        // Back face (normal: 0,0,-1)
        -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f, 0.0f,
         0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f, 1.0f,
         0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f, 0.0f,
         0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f, 1.0f,
        -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f, 0.0f,
        -0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f, 1.0f,

        // Front face (normal: 0,0,+1)
        -0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f, 0.0f,
         0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f, 0.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f, 1.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f, 1.0f,
        -0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f, 1.0f,
        -0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f, 0.0f,

        // Left face (normal: -1,0,0)
        -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  1.0f, 0.0f,
        -0.5f,  0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  1.0f, 1.0f,
        -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
        -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
        -0.5f, -0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  0.0f, 0.0f,
        -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  1.0f, 0.0f,

        // Right face (normal: +1,0,0)
         0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f,
         0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
         0.5f,  0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f,
         0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
         0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f,
         0.5f, -0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  0.0f, 0.0f,

        // Bottom face (normal: 0,-1,0)
        -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  0.0f, 1.0f,
         0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  1.0f, 1.0f,
         0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  1.0f, 0.0f,
         0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  1.0f, 0.0f,
        -0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  0.0f, 0.0f,
        -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  0.0f, 1.0f,

        // Top face (normal: 0,+1,0)
        -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  0.0f, 1.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  1.0f, 0.0f,
         0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  1.0f, 1.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  1.0f, 0.0f,
        -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  0.0f, 1.0f,
        -0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  0.0f, 0.0f
    };

    glGenVertexArrays(1, &s_Data->CubeVAO);
    glGenBuffers(1, &s_Data->CubeVBO);

    glBindVertexArray(s_Data->CubeVAO);
    glBindBuffer(GL_ARRAY_BUFFER, s_Data->CubeVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));

    glBindVertexArray(0);
}

void Renderer::InitSphere() {
    std::vector<float> vertices;
    std::vector<unsigned int> indices;

    const unsigned int X_SEGMENTS = 32;
    const unsigned int Y_SEGMENTS = 32;
    const float PI = 3.14159265359f;

    for (unsigned int y = 0; y <= Y_SEGMENTS; ++y) {
        for (unsigned int x = 0; x <= X_SEGMENTS; ++x) {
            float xSegment = (float)x / (float)X_SEGMENTS;
            float ySegment = (float)y / (float)Y_SEGMENTS;
            float xPos = std::cos(xSegment * 2.0f * PI) * std::sin(ySegment * PI);
            float yPos = std::cos(ySegment * PI);
            float zPos = std::sin(xSegment * 2.0f * PI) * std::sin(ySegment * PI);

            vertices.push_back(xPos * 0.5f);
            vertices.push_back(yPos * 0.5f);
            vertices.push_back(zPos * 0.5f);
            vertices.push_back(xPos);
            vertices.push_back(yPos);
            vertices.push_back(zPos);
            vertices.push_back(xSegment);
            vertices.push_back(ySegment);
        }
    }

    for (unsigned int y = 0; y < Y_SEGMENTS; ++y) {
        for (unsigned int x = 0; x < X_SEGMENTS; ++x) {
            // First triangle (CCW from outside)
            indices.push_back(y * (X_SEGMENTS + 1) + x);
            indices.push_back((y + 1) * (X_SEGMENTS + 1) + x + 1);
            indices.push_back((y + 1) * (X_SEGMENTS + 1) + x);

            // Second triangle (CCW from outside)
            indices.push_back(y * (X_SEGMENTS + 1) + x);
            indices.push_back(y * (X_SEGMENTS + 1) + x + 1);
            indices.push_back((y + 1) * (X_SEGMENTS + 1) + x + 1);
        }
    }

    s_Data->SphereIndexCount = indices.size();

    glGenVertexArrays(1, &s_Data->SphereVAO);
    glGenBuffers(1, &s_Data->SphereVBO);
    glGenBuffers(1, &s_Data->SphereEBO);

    glBindVertexArray(s_Data->SphereVAO);
    glBindBuffer(GL_ARRAY_BUFFER, s_Data->SphereVBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, s_Data->SphereEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));

    glBindVertexArray(0);
}

void Renderer::DrawCube(const glm::vec3& position, const glm::vec3& size, const glm::vec4& color) {
    AABB box = AABB::FromCenterSize(position, size);

    // Frustum cull
    if (s_Data->FrustumCullingEnabled) {
        bool culled = !s_Data->SceneFrustum.IsBoxVisible(box);
        GPUMetricsManager::RecordFrustumTest(culled);
        if (culled) {
            if (s_Data->DebugCullingMode)
                AddDebugAABB(box, glm::vec4(1.0f, 0.3f, 0.3f, 1.0f)); // Red = culled
            return;
        }
    }

    // Debug: show bounding box for visible objects
    if (s_Data->DebugCullingMode)
        AddDebugAABB(box, glm::vec4(0.2f, 1.0f, 0.2f, 1.0f)); // Green = visible

    glm::mat4 transform = glm::translate(glm::mat4(1.0f), position);
    transform = glm::scale(transform, size);

    s_Data->BasicShader->SetMat4("u_Transform", transform);
    s_Data->BasicShader->SetMat3("u_NormalMatrix", glm::mat3(glm::transpose(glm::inverse(transform))));
    s_Data->BasicShader->SetFloat4("u_Color", color);
    s_Data->BasicShader->SetInt("u_UseTexture", 0);

    glBindVertexArray(s_Data->CubeVAO);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    GPUMetricsManager::RecordDrawCall(36);
    glBindVertexArray(0);

    // Debug overlay: show culled back-faces as red transparent highlight
    if (s_Data->DebugCullingMode) {
        // Switch to overlay shader and draw only back-faces
        s_Data->BasicShader->Unbind();
        s_Data->DebugOverlayShader->Bind();
        s_Data->DebugOverlayShader->SetMat4("u_ViewProjection", s_Data->ViewProjectionMatrix);
        s_Data->DebugOverlayShader->SetMat4("u_Transform", transform);
        s_Data->DebugOverlayShader->SetFloat4("u_OverlayColor", glm::vec4(1.0f, 0.15f, 0.15f, 0.35f));

        glDisable(GL_DEPTH_TEST);
        glDisable(GL_CULL_FACE);
        glCullFace(GL_FRONT); // Only back-faces
        glEnable(GL_CULL_FACE);

        glBindVertexArray(s_Data->CubeVAO);
        glDrawArrays(GL_TRIANGLES, 0, 36);
        glBindVertexArray(0);

        // Restore state
        glCullFace(GL_BACK);
        glEnable(GL_DEPTH_TEST);
        if (!s_Data->FaceCullingEnabled) glDisable(GL_CULL_FACE);
        s_Data->DebugOverlayShader->Unbind();
        s_Data->BasicShader->Bind();
    }
}

void Renderer::DrawSphere(const glm::vec3& position, float radius, const glm::vec4& color) {
    AABB box = AABB::FromSphere(position, radius);

    // Frustum cull
    if (s_Data->FrustumCullingEnabled) {
        bool culled = !s_Data->SceneFrustum.IsSphereVisible(position, radius);
        GPUMetricsManager::RecordFrustumTest(culled);
        if (culled) {
            if (s_Data->DebugCullingMode)
                AddDebugAABB(box, glm::vec4(1.0f, 0.3f, 0.3f, 1.0f));
            return;
        }
    }

    if (s_Data->DebugCullingMode)
        AddDebugAABB(box, glm::vec4(0.2f, 1.0f, 0.2f, 1.0f));

    glm::mat4 transform = glm::translate(glm::mat4(1.0f), position);
    transform = glm::scale(transform, glm::vec3(radius * 2.0f));

    s_Data->BasicShader->SetMat4("u_Transform", transform);
    s_Data->BasicShader->SetMat3("u_NormalMatrix", glm::mat3(glm::transpose(glm::inverse(transform))));
    s_Data->BasicShader->SetFloat4("u_Color", color);
    s_Data->BasicShader->SetInt("u_UseTexture", 0);

    glBindVertexArray(s_Data->SphereVAO);
    glDrawElements(GL_TRIANGLES, s_Data->SphereIndexCount, GL_UNSIGNED_INT, 0);
    GPUMetricsManager::RecordIndexedDrawCall(s_Data->SphereIndexCount);
    glBindVertexArray(0);

    // Debug overlay: show culled back-faces
    if (s_Data->DebugCullingMode) {
        s_Data->BasicShader->Unbind();
        s_Data->DebugOverlayShader->Bind();
        s_Data->DebugOverlayShader->SetMat4("u_ViewProjection", s_Data->ViewProjectionMatrix);
        s_Data->DebugOverlayShader->SetMat4("u_Transform", transform);
        s_Data->DebugOverlayShader->SetFloat4("u_OverlayColor", glm::vec4(1.0f, 0.15f, 0.15f, 0.35f));

        glDisable(GL_DEPTH_TEST);
        glDisable(GL_CULL_FACE);
        glCullFace(GL_FRONT);
        glEnable(GL_CULL_FACE);

        glBindVertexArray(s_Data->SphereVAO);
        glDrawElements(GL_TRIANGLES, s_Data->SphereIndexCount, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);

        glCullFace(GL_BACK);
        glEnable(GL_DEPTH_TEST);
        if (!s_Data->FaceCullingEnabled) glDisable(GL_CULL_FACE);
        s_Data->DebugOverlayShader->Unbind();
        s_Data->BasicShader->Bind();
    }
}

void Renderer::DrawCube(const glm::vec3& position, const glm::vec3& size, const std::shared_ptr<Texture>& texture) {
    AABB box = AABB::FromCenterSize(position, size);

    // Frustum cull
    if (s_Data->FrustumCullingEnabled) {
        bool culled = !s_Data->SceneFrustum.IsBoxVisible(box);
        GPUMetricsManager::RecordFrustumTest(culled);
        if (culled) {
            if (s_Data->DebugCullingMode)
                AddDebugAABB(box, glm::vec4(1.0f, 0.3f, 0.3f, 1.0f));
            return;
        }
    }

    if (s_Data->DebugCullingMode)
        AddDebugAABB(box, glm::vec4(0.2f, 1.0f, 0.2f, 1.0f));

    glm::mat4 transform = glm::translate(glm::mat4(1.0f), position);
    transform = glm::scale(transform, size);

    s_Data->BasicShader->SetMat4("u_Transform", transform);
    s_Data->BasicShader->SetMat3("u_NormalMatrix", glm::mat3(glm::transpose(glm::inverse(transform))));
    s_Data->BasicShader->SetInt("u_UseTexture", 1);
    texture->Bind(0);
    GPUMetricsManager::RecordTextureBind();

    glBindVertexArray(s_Data->CubeVAO);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    GPUMetricsManager::RecordDrawCall(36);
    glBindVertexArray(0);

    // Debug overlay: show culled back-faces
    if (s_Data->DebugCullingMode) {
        s_Data->BasicShader->Unbind();
        s_Data->DebugOverlayShader->Bind();
        s_Data->DebugOverlayShader->SetMat4("u_ViewProjection", s_Data->ViewProjectionMatrix);
        s_Data->DebugOverlayShader->SetMat4("u_Transform", transform);
        s_Data->DebugOverlayShader->SetFloat4("u_OverlayColor", glm::vec4(1.0f, 0.15f, 0.15f, 0.35f));

        glDisable(GL_DEPTH_TEST);
        glDisable(GL_CULL_FACE);
        glCullFace(GL_FRONT);
        glEnable(GL_CULL_FACE);

        glBindVertexArray(s_Data->CubeVAO);
        glDrawArrays(GL_TRIANGLES, 0, 36);
        glBindVertexArray(0);

        glCullFace(GL_BACK);
        glEnable(GL_DEPTH_TEST);
        if (!s_Data->FaceCullingEnabled) glDisable(GL_CULL_FACE);
        s_Data->DebugOverlayShader->Unbind();
        s_Data->BasicShader->Bind();
    }
}

void Renderer::DrawSphere(const glm::vec3& position, float radius, const std::shared_ptr<Texture>& texture) {
    AABB box = AABB::FromSphere(position, radius);

    // Frustum cull
    if (s_Data->FrustumCullingEnabled) {
        bool culled = !s_Data->SceneFrustum.IsSphereVisible(position, radius);
        GPUMetricsManager::RecordFrustumTest(culled);
        if (culled) {
            if (s_Data->DebugCullingMode)
                AddDebugAABB(box, glm::vec4(1.0f, 0.3f, 0.3f, 1.0f));
            return;
        }
    }

    if (s_Data->DebugCullingMode)
        AddDebugAABB(box, glm::vec4(0.2f, 1.0f, 0.2f, 1.0f));

    glm::mat4 transform = glm::translate(glm::mat4(1.0f), position);
    transform = glm::scale(transform, glm::vec3(radius * 2.0f));

    s_Data->BasicShader->SetMat4("u_Transform", transform);
    s_Data->BasicShader->SetMat3("u_NormalMatrix", glm::mat3(glm::transpose(glm::inverse(transform))));
    s_Data->BasicShader->SetInt("u_UseTexture", 1);
    texture->Bind(0);
    GPUMetricsManager::RecordTextureBind();

    glBindVertexArray(s_Data->SphereVAO);
    glDrawElements(GL_TRIANGLES, s_Data->SphereIndexCount, GL_UNSIGNED_INT, 0);
    GPUMetricsManager::RecordIndexedDrawCall(s_Data->SphereIndexCount);
    glBindVertexArray(0);

    // Debug overlay: show culled back-faces
    if (s_Data->DebugCullingMode) {
        s_Data->BasicShader->Unbind();
        s_Data->DebugOverlayShader->Bind();
        s_Data->DebugOverlayShader->SetMat4("u_ViewProjection", s_Data->ViewProjectionMatrix);
        s_Data->DebugOverlayShader->SetMat4("u_Transform", transform);
        s_Data->DebugOverlayShader->SetFloat4("u_OverlayColor", glm::vec4(1.0f, 0.15f, 0.15f, 0.35f));

        glDisable(GL_DEPTH_TEST);
        glDisable(GL_CULL_FACE);
        glCullFace(GL_FRONT);
        glEnable(GL_CULL_FACE);

        glBindVertexArray(s_Data->SphereVAO);
        glDrawElements(GL_TRIANGLES, s_Data->SphereIndexCount, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);

        glCullFace(GL_BACK);
        glEnable(GL_DEPTH_TEST);
        if (!s_Data->FaceCullingEnabled) glDisable(GL_CULL_FACE);
        s_Data->DebugOverlayShader->Unbind();
        s_Data->BasicShader->Bind();
    }
}

void Renderer::DrawModel(const std::shared_ptr<Model>& model, const glm::vec3& position, const glm::vec3& rotation, const glm::vec3& scale) {
    glm::mat4 transform = glm::mat4(1.0f);
    transform = glm::translate(transform, position);
    transform = glm::rotate(transform, glm::radians(rotation.x), glm::vec3(1, 0, 0));
    transform = glm::rotate(transform, glm::radians(rotation.y), glm::vec3(0, 1, 0));
    transform = glm::rotate(transform, glm::radians(rotation.z), glm::vec3(0, 0, 1));
    transform = glm::scale(transform, scale);

    for (const auto& mesh : model->GetMeshes()) {
        DrawMesh(mesh.get(), transform);
    }
}

void Renderer::DrawMesh(Mesh* mesh, const glm::mat4& transform) {
    const auto& material = mesh->GetMaterial();
    
    // Use PBR shader if material has PBR properties
    if (material.UsePBR && material.PBR) {
        s_Data->PBRShader->Bind();
        GPUMetricsManager::RecordShaderBind();
        s_Data->PBRShader->SetMat4("u_ViewProjection", s_Data->ViewProjectionMatrix);
        s_Data->PBRShader->SetMat4("u_Transform", transform);
        s_Data->PBRShader->SetFloat3("u_CamPos", s_Data->CameraPosition);
        
        // Set light
        if (s_Data->DirectionalLight) {
            s_Data->PBRShader->SetFloat3("u_LightDir", s_Data->DirectionalLight->Direction);
            s_Data->PBRShader->SetFloat3("u_LightColor", s_Data->DirectionalLight->Color * s_Data->DirectionalLight->Intensity);
        } else {
            s_Data->PBRShader->SetFloat3("u_LightDir", glm::vec3(-0.3f, -1.0f, -0.3f));
            s_Data->PBRShader->SetFloat3("u_LightColor", glm::vec3(1.0f));
        }
        
        // Set material properties
        const auto& pbr = material.PBR;
        s_Data->PBRShader->SetFloat3("u_Albedo", pbr->Albedo);
        s_Data->PBRShader->SetFloat("u_Metallic", pbr->Metallic);
        s_Data->PBRShader->SetFloat("u_Roughness", pbr->Roughness);
        s_Data->PBRShader->SetFloat("u_AO", pbr->AO);
        
        // Bind textures
        if (pbr->AlbedoMap) {
            s_Data->PBRShader->SetInt("u_UseAlbedoMap", 1);
            pbr->AlbedoMap->Bind(0);
            s_Data->PBRShader->SetInt("u_AlbedoMap", 0);
        } else {
            s_Data->PBRShader->SetInt("u_UseAlbedoMap", 0);
        }
        
        if (pbr->MetallicMap) {
            s_Data->PBRShader->SetInt("u_UseMetallicMap", 1);
            pbr->MetallicMap->Bind(1);
            s_Data->PBRShader->SetInt("u_MetallicMap", 1);
        } else {
            s_Data->PBRShader->SetInt("u_UseMetallicMap", 0);
        }
        
        if (pbr->RoughnessMap) {
            s_Data->PBRShader->SetInt("u_UseRoughnessMap", 1);
            pbr->RoughnessMap->Bind(2);
            s_Data->PBRShader->SetInt("u_RoughnessMap", 2);
        } else {
            s_Data->PBRShader->SetInt("u_UseRoughnessMap", 0);
        }
        
        if (pbr->AOMap) {
            s_Data->PBRShader->SetInt("u_UseAOMap", 1);
            pbr->AOMap->Bind(3);
            s_Data->PBRShader->SetInt("u_AOMap", 3);
        } else {
            s_Data->PBRShader->SetInt("u_UseAOMap", 0);
        }
        
        s_Data->PBRShader->SetInt("u_UseNormalMap", 0); // TODO: implement normal mapping
        
        mesh->Draw();
        s_Data->PBRShader->Unbind();
    } else {
        // Use basic shader for non-PBR materials
        s_Data->BasicShader->SetMat4("u_Transform", transform);
        s_Data->BasicShader->SetMat3("u_NormalMatrix", glm::mat3(glm::transpose(glm::inverse(transform))));
        
        if (material.DiffuseMap) {
            s_Data->BasicShader->SetInt("u_UseTexture", 1);
            material.DiffuseMap->Bind(0);
        } else {
            s_Data->BasicShader->SetInt("u_UseTexture", 0);
            s_Data->BasicShader->SetFloat4("u_Color", material.Color);
        }

        mesh->Draw();
    }
}

void Renderer::SetDirectionalLight(DirectionalLight* light) {
    s_Data->DirectionalLight = light;
}

void Renderer::SetCameraPosition(const glm::vec3& position) {
    s_Data->CameraPosition = position;
}

void Renderer::SetShadowMap(ShadowMap* shadowMap) {
    s_Data->ShadowMap = shadowMap;
}

void Renderer::BeginShadowPass() {
    if (!s_Data->ShadowMap || !s_Data->DirectionalLight) return;
    
    s_Data->ShadowMap->Bind();
    s_Data->ShadowShader->Bind();
    s_Data->ShadowShader->SetMat4("u_LightSpaceMatrix", s_Data->DirectionalLight->GetLightSpaceMatrix());
}

void Renderer::EndShadowPass() {
    if (!s_Data->ShadowMap) return;
    
    s_Data->ShadowMap->Unbind();
    s_Data->ShadowShader->Unbind();
}

void Renderer::DrawSkybox(Skybox* skybox) {
    if (!skybox) return;

    // Disable face culling for skybox (we render inside the cube)
    if (s_Data->FaceCullingEnabled) glDisable(GL_CULL_FACE);
    
    s_Data->SkyboxShader->Bind();
    GPUMetricsManager::RecordShaderBind();
    
    // Remove translation from view matrix
    glm::mat4 view = glm::mat4(glm::mat3(s_Data->ViewMatrix));
    
    s_Data->SkyboxShader->SetMat4("u_View", view);
    s_Data->SkyboxShader->SetMat4("u_Projection", s_Data->ProjectionMatrix);
    s_Data->SkyboxShader->SetInt("u_Skybox", 0);
    
    skybox->Draw();
    
    s_Data->SkyboxShader->Unbind();

    // Re-enable face culling
    if (s_Data->FaceCullingEnabled) glEnable(GL_CULL_FACE);
}

void Renderer::DrawParticles(ParticleSystem* particles) {
    if (!particles) return;

    // Extract camera vectors from the view matrix
    glm::mat4 view = s_Data->ViewMatrix;
    glm::vec3 right = glm::vec3(view[0][0], view[1][0], view[2][0]);
    glm::vec3 up    = glm::vec3(view[0][1], view[1][1], view[2][1]);

    particles->Render(s_Data->ViewProjectionMatrix,
                      s_Data->CameraPosition, right, up);
}

void Renderer::SetFaceCullingEnabled(bool enabled) {
    s_Data->FaceCullingEnabled = enabled;
    if (enabled)
        glEnable(GL_CULL_FACE);
    else
        glDisable(GL_CULL_FACE);
}

void Renderer::SetFrustumCullingEnabled(bool enabled) {
    s_Data->FrustumCullingEnabled = enabled;
}

bool Renderer::IsFaceCullingEnabled() {
    return s_Data->FaceCullingEnabled;
}

bool Renderer::IsFrustumCullingEnabled() {
    return s_Data->FrustumCullingEnabled;
}

void Renderer::SetDebugCullingMode(bool enabled) {
    s_Data->DebugCullingMode = enabled;
}

bool Renderer::IsDebugCullingMode() {
    return s_Data->DebugCullingMode;
}

void Renderer::AddDebugAABB(const AABB& box, const glm::vec4& color) {
    // 8 corners of the AABB
    glm::vec3 c[8] = {
        { box.Min.x, box.Min.y, box.Min.z }, // 0
        { box.Max.x, box.Min.y, box.Min.z }, // 1
        { box.Max.x, box.Min.y, box.Max.z }, // 2
        { box.Min.x, box.Min.y, box.Max.z }, // 3
        { box.Min.x, box.Max.y, box.Min.z }, // 4
        { box.Max.x, box.Max.y, box.Min.z }, // 5
        { box.Max.x, box.Max.y, box.Max.z }, // 6
        { box.Min.x, box.Max.y, box.Max.z }, // 7
    };

    // 12 edges as pairs of corner indices
    static const int edges[12][2] = {
        {0,1}, {1,2}, {2,3}, {3,0}, // bottom
        {4,5}, {5,6}, {6,7}, {7,4}, // top
        {0,4}, {1,5}, {2,6}, {3,7}, // vertical
    };

    auto& verts = s_Data->DebugLineVertices;
    for (int i = 0; i < 12; i++) {
        const glm::vec3& a = c[edges[i][0]];
        const glm::vec3& b = c[edges[i][1]];
        // Vertex A: pos + color
        verts.push_back(a.x); verts.push_back(a.y); verts.push_back(a.z);
        verts.push_back(color.r); verts.push_back(color.g); verts.push_back(color.b); verts.push_back(color.a);
        // Vertex B: pos + color
        verts.push_back(b.x); verts.push_back(b.y); verts.push_back(b.z);
        verts.push_back(color.r); verts.push_back(color.g); verts.push_back(color.b); verts.push_back(color.a);
    }
}

void Renderer::FlushDebugDraw() {
    if (s_Data->DebugLineVertices.empty()) return;

    s_Data->DebugLineShader->Bind();
    s_Data->DebugLineShader->SetMat4("u_ViewProjection", s_Data->ViewProjectionMatrix);

    glBindVertexArray(s_Data->DebugLineVAO);
    glBindBuffer(GL_ARRAY_BUFFER, s_Data->DebugLineVBO);
    glBufferData(GL_ARRAY_BUFFER,
                 s_Data->DebugLineVertices.size() * sizeof(float),
                 s_Data->DebugLineVertices.data(),
                 GL_DYNAMIC_DRAW);

    glLineWidth(2.0f);
    glDrawArrays(GL_LINES, 0, static_cast<GLsizei>(s_Data->DebugLineVertices.size() / 7));
    glLineWidth(1.0f);

    glBindVertexArray(0);
    s_Data->DebugLineShader->Unbind();
}

} // namespace Kaya
