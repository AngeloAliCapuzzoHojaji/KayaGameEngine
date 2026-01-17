#pragma once

#include <string>

namespace Kaya {

enum class TextureFormat {
    None = 0,
    RGB = 3,
    RGBA = 4
};

enum class TextureFilter {
    Linear,
    Nearest
};

enum class TextureWrap {
    Repeat,
    ClampToEdge,
    MirroredRepeat
};

struct TextureSpecification {
    TextureFilter MinFilter = TextureFilter::Linear;
    TextureFilter MagFilter = TextureFilter::Linear;
    TextureWrap WrapS = TextureWrap::Repeat;
    TextureWrap WrapT = TextureWrap::Repeat;
    bool GenerateMipmaps = true;
};

class Texture {
public:
    Texture(const std::string& path, const TextureSpecification& spec = TextureSpecification());
    Texture(unsigned int width, unsigned int height, TextureFormat format, const void* data = nullptr, const TextureSpecification& spec = TextureSpecification());
    ~Texture();

    void Bind(unsigned int slot = 0) const;
    void Unbind() const;

    unsigned int GetWidth() const { return m_Width; }
    unsigned int GetHeight() const { return m_Height; }
    unsigned int GetRendererID() const { return m_RendererID; }
    TextureFormat GetFormat() const { return m_Format; }

    void SetData(const void* data, unsigned int size);

private:
    void CreateTexture(const TextureSpecification& spec);

private:
    unsigned int m_RendererID = 0;
    unsigned int m_Width = 0;
    unsigned int m_Height = 0;
    TextureFormat m_Format = TextureFormat::None;
};

} // namespace Kaya
