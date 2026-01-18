#include "Rendering/Renderer.h"
#include "Rendering/Shader.h"
#include "Rendering/Camera.h"
#include "Rendering/Texture.h"
#include "Rendering/Model.h"
#include "Rendering/Mesh.h"
#include "Rendering/Light.h"
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

    // Basic shader
    std::string vertexSrc = R"(
        #version 460 core
        layout(location = 0) in vec3 a_Position;
        layout(location = 1) in vec3 a_Normal;
        layout(location = 2) in vec2 a_TexCoord;
        
        uniform mat4 u_ViewProjection;
        uniform mat4 u_Transform;
        uniform mat4 u_LightSpaceMatrix;
        
        out vec3 v_Normal;
        out vec3 v_FragPos;
        out vec2 v_TexCoord;
        out vec4 v_FragPosLightSpace;
        
        void main() {
            v_FragPos = vec3(u_Transform * vec4(a_Position, 1.0));
            v_Normal = mat3(transpose(inverse(u_Transform))) * a_Normal;
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
    
    InitCube();
    InitSphere();
}

void Renderer::Shutdown() {
    glDeleteVertexArrays(1, &s_Data->CubeVAO);
    glDeleteBuffers(1, &s_Data->CubeVBO);
    glDeleteVertexArrays(1, &s_Data->SphereVAO);
    glDeleteBuffers(1, &s_Data->SphereVBO);
    glDeleteBuffers(1, &s_Data->SphereEBO);
    delete s_Data;
}

void Renderer::BeginScene(Camera& camera) {
    s_Data->ViewProjectionMatrix = camera.GetViewProjectionMatrix();
    s_Data->BasicShader->Bind();
    s_Data->BasicShader->SetMat4("u_ViewProjection", s_Data->ViewProjectionMatrix);
    
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
}

void Renderer::Clear(const glm::vec4& color) {
    glClearColor(color.r, color.g, color.b, color.a);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void Renderer::SetViewport(unsigned int x, unsigned int y, unsigned int width, unsigned int height) {
    glViewport(x, y, width, height);
}

void Renderer::InitCube() {
    float vertices[] = {
        // positions          // normals           // texture coords
        -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f, 0.0f,
         0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f, 0.0f,
         0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f, 1.0f,
         0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f, 1.0f,
        -0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f, 1.0f,
        -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f, 0.0f,

        -0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f, 0.0f,
         0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f, 0.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f, 1.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f, 1.0f,
        -0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f, 1.0f,
        -0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f, 0.0f,

        -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  1.0f, 0.0f,
        -0.5f,  0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  1.0f, 1.0f,
        -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
        -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
        -0.5f, -0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  0.0f, 0.0f,
        -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  1.0f, 0.0f,

         0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f,
         0.5f,  0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f,
         0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
         0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
         0.5f, -0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  0.0f, 0.0f,
         0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f,

        -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  0.0f, 1.0f,
         0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  1.0f, 1.0f,
         0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  1.0f, 0.0f,
         0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  1.0f, 0.0f,
        -0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  0.0f, 0.0f,
        -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  0.0f, 1.0f,

        -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  0.0f, 1.0f,
         0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  1.0f, 1.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  1.0f, 0.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  1.0f, 0.0f,
        -0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  0.0f, 0.0f,
        -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  0.0f, 1.0f
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
            indices.push_back(y * (X_SEGMENTS + 1) + x);
            indices.push_back((y + 1) * (X_SEGMENTS + 1) + x);
            indices.push_back((y + 1) * (X_SEGMENTS + 1) + x + 1);

            indices.push_back(y * (X_SEGMENTS + 1) + x);
            indices.push_back((y + 1) * (X_SEGMENTS + 1) + x + 1);
            indices.push_back(y * (X_SEGMENTS + 1) + x + 1);
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
    glm::mat4 transform = glm::translate(glm::mat4(1.0f), position);
    transform = glm::scale(transform, size);

    s_Data->BasicShader->SetMat4("u_Transform", transform);
    s_Data->BasicShader->SetFloat4("u_Color", color);
    s_Data->BasicShader->SetInt("u_UseTexture", 0);

    glBindVertexArray(s_Data->CubeVAO);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindVertexArray(0);
}

void Renderer::DrawSphere(const glm::vec3& position, float radius, const glm::vec4& color) {
    glm::mat4 transform = glm::translate(glm::mat4(1.0f), position);
    transform = glm::scale(transform, glm::vec3(radius * 2.0f));

    s_Data->BasicShader->SetMat4("u_Transform", transform);
    s_Data->BasicShader->SetFloat4("u_Color", color);
    s_Data->BasicShader->SetInt("u_UseTexture", 0);

    glBindVertexArray(s_Data->SphereVAO);
    glDrawElements(GL_TRIANGLES, s_Data->SphereIndexCount, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}

void Renderer::DrawCube(const glm::vec3& position, const glm::vec3& size, const std::shared_ptr<Texture>& texture) {
    glm::mat4 transform = glm::translate(glm::mat4(1.0f), position);
    transform = glm::scale(transform, size);

    s_Data->BasicShader->SetMat4("u_Transform", transform);
    s_Data->BasicShader->SetInt("u_UseTexture", 1);
    texture->Bind(0);

    glBindVertexArray(s_Data->CubeVAO);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindVertexArray(0);
}

void Renderer::DrawSphere(const glm::vec3& position, float radius, const std::shared_ptr<Texture>& texture) {
    glm::mat4 transform = glm::translate(glm::mat4(1.0f), position);
    transform = glm::scale(transform, glm::vec3(radius * 2.0f));

    s_Data->BasicShader->SetMat4("u_Transform", transform);
    s_Data->BasicShader->SetInt("u_UseTexture", 1);
    texture->Bind(0);

    glBindVertexArray(s_Data->SphereVAO);
    glDrawElements(GL_TRIANGLES, s_Data->SphereIndexCount, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
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
    s_Data->BasicShader->SetMat4("u_Transform", transform);
    
    const auto& material = mesh->GetMaterial();
    if (material.DiffuseMap) {
        s_Data->BasicShader->SetInt("u_UseTexture", 1);
        material.DiffuseMap->Bind(0);
    } else {
        s_Data->BasicShader->SetInt("u_UseTexture", 0);
        s_Data->BasicShader->SetFloat4("u_Color", material.Color);
    }

    mesh->Draw();
}

void Renderer::SetDirectionalLight(DirectionalLight* light) {
    s_Data->DirectionalLight = light;
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

} // namespace Kaya
