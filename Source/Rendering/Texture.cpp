#include "Rendering/Texture.h"
#include <glad/glad.h>
#include <iostream>
#include <stdexcept>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

namespace Kaya {

static GLenum TextureFormatToGL(TextureFormat format) {
    switch (format) {
        case TextureFormat::RGB:  return GL_RGB;
        case TextureFormat::RGBA: return GL_RGBA;
        default: return GL_RGBA;
    }
}

static GLenum TextureFilterToGL(TextureFilter filter) {
    switch (filter) {
        case TextureFilter::Linear:  return GL_LINEAR;
        case TextureFilter::Nearest: return GL_NEAREST;
        default: return GL_LINEAR;
    }
}

static GLenum TextureWrapToGL(TextureWrap wrap) {
    switch (wrap) {
        case TextureWrap::Repeat:         return GL_REPEAT;
        case TextureWrap::ClampToEdge:    return GL_CLAMP_TO_EDGE;
        case TextureWrap::MirroredRepeat: return GL_MIRRORED_REPEAT;
        default: return GL_REPEAT;
    }
}

Texture::Texture(const std::string& path, const TextureSpecification& spec) {
    // Load image using stb_image
    stbi_set_flip_vertically_on_load(1);
    
    int width, height, channels;
    unsigned char* data = stbi_load(path.c_str(), &width, &height, &channels, 0);
    
    if (!data) {
        std::cerr << "Failed to load texture: " << path << std::endl;
        throw std::runtime_error("Failed to load texture: " + path);
    }

    m_Width = width;
    m_Height = height;
    m_Format = (channels == 4) ? TextureFormat::RGBA : TextureFormat::RGB;

    std::cout << "Loaded texture: " << path << " (" << width << "x" << height << ", " << channels << " channels)" << std::endl;

    CreateTexture(spec);
    
    // Upload texture data
    GLenum internalFormat = (channels == 4) ? GL_RGBA8 : GL_RGB8;
    GLenum dataFormat = TextureFormatToGL(m_Format);
    
    glBindTexture(GL_TEXTURE_2D, m_RendererID);
    glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, m_Width, m_Height, 0, dataFormat, GL_UNSIGNED_BYTE, data);
    
    if (spec.GenerateMipmaps) {
        glGenerateMipmap(GL_TEXTURE_2D);
    }

    stbi_image_free(data);
}

Texture::Texture(unsigned int width, unsigned int height, TextureFormat format, const void* data, const TextureSpecification& spec)
    : m_Width(width), m_Height(height), m_Format(format) {
    
    CreateTexture(spec);
    
    if (data) {
        SetData(data, width * height * static_cast<unsigned int>(format));
    }
}

Texture::~Texture() {
    if (m_RendererID) {
        glDeleteTextures(1, &m_RendererID);
    }
}

void Texture::CreateTexture(const TextureSpecification& spec) {
    glGenTextures(1, &m_RendererID);
    glBindTexture(GL_TEXTURE_2D, m_RendererID);

    // Set texture parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, TextureFilterToGL(spec.MinFilter));
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, TextureFilterToGL(spec.MagFilter));
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, TextureWrapToGL(spec.WrapS));
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, TextureWrapToGL(spec.WrapT));
}

void Texture::Bind(unsigned int slot) const {
    glActiveTexture(GL_TEXTURE0 + slot);
    glBindTexture(GL_TEXTURE_2D, m_RendererID);
}

void Texture::Unbind() const {
    glBindTexture(GL_TEXTURE_2D, 0);
}

void Texture::SetData(const void* data, unsigned int size) {
    unsigned int bpp = static_cast<unsigned int>(m_Format);
    if (size != m_Width * m_Height * bpp) {
        std::cerr << "Data size mismatch! Expected " << (m_Width * m_Height * bpp) << " but got " << size << std::endl;
        return;
    }

    GLenum internalFormat = (m_Format == TextureFormat::RGBA) ? GL_RGBA8 : GL_RGB8;
    GLenum dataFormat = TextureFormatToGL(m_Format);
    
    glBindTexture(GL_TEXTURE_2D, m_RendererID);
    glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, m_Width, m_Height, 0, dataFormat, GL_UNSIGNED_BYTE, data);
}

} // namespace Kaya
