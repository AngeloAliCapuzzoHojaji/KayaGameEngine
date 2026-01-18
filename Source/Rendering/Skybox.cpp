#include "Rendering/Skybox.h"
#include <glad/glad.h>
#include <stb_image.h>
#include <iostream>

namespace Kaya {

// ========== Cubemap ==========

Cubemap::Cubemap(const std::vector<std::string>& faces) {
    LoadFromFaces(faces);
}

Cubemap::Cubemap(const std::string& hdrPath) {
    LoadFromHDR(hdrPath);
}

Cubemap::Cubemap() {
    // Create procedural gradient skybox
    CreateProceduralGradient();
}

Cubemap::~Cubemap() {
    if (m_RendererID) {
        glDeleteTextures(1, &m_RendererID);
    }
}

void Cubemap::LoadFromFaces(const std::vector<std::string>& faces) {
    glGenTextures(1, &m_RendererID);
    glBindTexture(GL_TEXTURE_CUBE_MAP, m_RendererID);

    int width, height, nrChannels;
    for (unsigned int i = 0; i < faces.size(); i++) {
        unsigned char* data = stbi_load(faces[i].c_str(), &width, &height, &nrChannels, 0);
        if (data) {
            GLenum format = GL_RGB;
            if (nrChannels == 1)
                format = GL_RED;
            else if (nrChannels == 3)
                format = GL_RGB;
            else if (nrChannels == 4)
                format = GL_RGBA;

            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 
                        0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
            stbi_image_free(data);
        } else {
            std::cerr << "Cubemap texture failed to load at path: " << faces[i] << std::endl;
            stbi_image_free(data);
        }
    }

    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    std::cout << "Cubemap loaded from " << faces.size() << " faces" << std::endl;
}

void Cubemap::LoadFromHDR(const std::string& hdrPath) {
    // For HDR loading, we'd need to:
    // 1. Load HDR equirectangular image
    // 2. Create cubemap framebuffer
    // 3. Render equirectangular to cubemap faces
    // This is a simplified version that just creates an empty cubemap
    
    glGenTextures(1, &m_RendererID);
    glBindTexture(GL_TEXTURE_CUBE_MAP, m_RendererID);

    // Create empty cubemap for now
    for (unsigned int i = 0; i < 6; ++i) {
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F,
                    512, 512, 0, GL_RGB, GL_FLOAT, nullptr);
    }

    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    std::cout << "HDR Cubemap created (placeholder)" << std::endl;
}

void Cubemap::CreateProceduralGradient() {
    glGenTextures(1, &m_RendererID);
    glBindTexture(GL_TEXTURE_CUBE_MAP, m_RendererID);

    const int size = 256;
    std::vector<unsigned char> data(size * size * 3);

    // Define colors for gradient (top to bottom)
    unsigned char topColor[3] = {40, 80, 150};      // Sky blue
    unsigned char horizonColor[3] = {180, 150, 120}; // Warm horizon
    unsigned char bottomColor[3] = {60, 60, 80};     // Dark bottom

    // Generate each face
    for (unsigned int face = 0; face < 6; ++face) {
        for (int y = 0; y < size; ++y) {
            for (int x = 0; x < size; ++x) {
                int index = (y * size + x) * 3;
                
                // Calculate gradient based on face and position
                float fy = (float)y / (float)size;
                float t = 0.0f;
                
                switch (face) {
                    case 0: // +X (right)
                    case 1: // -X (left)
                    case 4: // +Z (front)
                    case 5: // -Z (back)
                        t = fy; // Vertical gradient
                        break;
                    case 2: // +Y (top)
                        t = 0.0f; // All top color
                        break;
                    case 3: // -Y (bottom)
                        t = 1.0f; // All bottom color
                        break;
                }
                
                // Smooth gradient interpolation
                unsigned char r, g, b;
                if (t < 0.5f) {
                    float s = t * 2.0f;
                    r = (unsigned char)(topColor[0] * (1.0f - s) + horizonColor[0] * s);
                    g = (unsigned char)(topColor[1] * (1.0f - s) + horizonColor[1] * s);
                    b = (unsigned char)(topColor[2] * (1.0f - s) + horizonColor[2] * s);
                } else {
                    float s = (t - 0.5f) * 2.0f;
                    r = (unsigned char)(horizonColor[0] * (1.0f - s) + bottomColor[0] * s);
                    g = (unsigned char)(horizonColor[1] * (1.0f - s) + bottomColor[1] * s);
                    b = (unsigned char)(horizonColor[2] * (1.0f - s) + bottomColor[2] * s);
                }
                
                data[index + 0] = r;
                data[index + 1] = g;
                data[index + 2] = b;
            }
        }
        
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + face, 0, GL_RGB,
                    size, size, 0, GL_RGB, GL_UNSIGNED_BYTE, data.data());
    }

    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    std::cout << "Procedural gradient skybox created" << std::endl;
}

void Cubemap::Bind(unsigned int slot) const {
    glActiveTexture(GL_TEXTURE0 + slot);
    glBindTexture(GL_TEXTURE_CUBE_MAP, m_RendererID);
}

void Cubemap::Unbind() const {
    glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
}

// ========== Skybox ==========

Skybox::Skybox(const std::shared_ptr<Cubemap>& cubemap)
    : m_Cubemap(cubemap) {
    InitCube();
}

Skybox::~Skybox() {
    if (m_VAO) glDeleteVertexArrays(1, &m_VAO);
    if (m_VBO) glDeleteBuffers(1, &m_VBO);
}

void Skybox::InitCube() {
    float skyboxVertices[] = {
        // positions          
        -1.0f,  1.0f, -1.0f,
        -1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,
         1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,

        -1.0f, -1.0f,  1.0f,
        -1.0f, -1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f,  1.0f,
        -1.0f, -1.0f,  1.0f,

         1.0f, -1.0f, -1.0f,
         1.0f, -1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,

        -1.0f, -1.0f,  1.0f,
        -1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f, -1.0f,  1.0f,
        -1.0f, -1.0f,  1.0f,

        -1.0f,  1.0f, -1.0f,
         1.0f,  1.0f, -1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
        -1.0f,  1.0f,  1.0f,
        -1.0f,  1.0f, -1.0f,

        -1.0f, -1.0f, -1.0f,
        -1.0f, -1.0f,  1.0f,
         1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,
        -1.0f, -1.0f,  1.0f,
         1.0f, -1.0f,  1.0f
    };

    glGenVertexArrays(1, &m_VAO);
    glGenBuffers(1, &m_VBO);
    
    glBindVertexArray(m_VAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), &skyboxVertices, GL_STATIC_DRAW);
    
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    
    glBindVertexArray(0);
}

void Skybox::Draw() {
    glDepthFunc(GL_LEQUAL);
    
    if (m_Cubemap) {
        m_Cubemap->Bind(0);
    }
    
    glBindVertexArray(m_VAO);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindVertexArray(0);
    
    glDepthFunc(GL_LESS);
}

} // namespace Kaya
