# Rendering Features Documentation

This document provides an overview of all rendering features implemented in the Kaya Game Engine.

## Completed Features

### 1. Texture System
**Files**: `Rendering/Texture.h`, `Rendering/Texture.cpp`

Load and manage textures with full OpenGL configuration:
- **Formats**: PNG, JPG, TGA (via STB Image)
- **Filtering**: Nearest, Linear, Mipmaps
- **Wrapping**: Repeat, Clamp, Mirror
- **Usage**: Diffuse maps, specular maps, normal maps, PBR texture maps

```cpp
// Load a texture
auto texture = std::make_shared<Texture>("path/to/texture.png");

// Draw textured cube
Renderer::DrawCube(position, size, texture);
```

---

### 2. Model Loading System
**Files**: `Rendering/Model.h`, `Rendering/Model.cpp`, `Rendering/Mesh.h`, `Rendering/Mesh.cpp`

Import complex 3D models with automatic material and texture loading:
- **Formats**: OBJ, GLTF, FBX (via Assimp)
- **Features**: Multi-mesh support, material extraction, texture loading
- **Data**: Positions, normals, texture coordinates

```cpp
// Load a model
auto model = std::make_shared<Model>("path/to/model.obj");

// Add to entity
entity->GetRender().GeometryType = RenderComponent::Type::Model;
entity->GetRender().ModelAsset = model;
```

---

### 3. Shadow Mapping
**Files**: `Rendering/Light.h`, `Rendering/Light.cpp`

Real-time shadows with high-quality filtering:
- **Technique**: Depth buffer shadow mapping
- **Resolution**: 2048x2048 shadow map
- **Filtering**: PCF (Percentage Closer Filtering) for soft shadows
- **Light**: Directional light support

```cpp
// Create directional light with shadows
auto light = std::make_shared<DirectionalLight>();
light->Direction = glm::vec3(-0.5f, -1.0f, -0.3f);
light->Color = glm::vec3(1.0f, 1.0f, 0.9f);
light->Intensity = 1.5f;
light->CastShadows = true;

Renderer::SetDirectionalLight(light.get());

// Create shadow map
auto shadowMap = std::make_shared<ShadowMap>();
Renderer::SetShadowMap(shadowMap.get());
```

---

### 4. PBR Material System
**Files**: `Rendering/PBRMaterial.h`, updated `Rendering/Renderer.cpp`

Physically Based Rendering with industry-standard workflow:
- **Workflow**: Metallic-Roughness
- **BRDF**: Cook-Torrance with GGX distribution
- **Terms**: Fresnel (Schlick), Geometry (Smith), Distribution (GGX)
- **Texture Maps**: Albedo, Metallic, Roughness, AO, Normal

```cpp
// PBR materials are automatically loaded from model files
// Or create manually:
Material material;
material.PBR = std::make_shared<PBRMaterial>();
material.PBR->Albedo = glm::vec3(0.8f, 0.1f, 0.1f);
material.PBR->Metallic = 0.9f;
material.PBR->Roughness = 0.2f;
material.PBR->AO = 1.0f;
```

**Shader Features**:
- HDR tone mapping
- Gamma correction
- Multiple light support
- Texture map overrides

---

### 5. Post-Processing Framework
**Files**: `Rendering/PostProcessor.h`, `Rendering/PostProcessor.cpp`

Full-screen effects pipeline with compositing:
- **Bloom**: Bright area extraction + Gaussian blur (10 passes)
- **Tone Mapping**: ACES filmic tone mapping
- **Vignette**: Customizable edge darkening
- **Gamma Correction**: sRGB output

```cpp
// Create post-processor
auto postProcessor = std::make_shared<PostProcessor>(width, height);

// Configure effects
postProcessor->SetBloomEnabled(true);
postProcessor->SetBloomThreshold(1.0f);
postProcessor->SetExposure(1.2f);
postProcessor->SetVignetteStrength(0.3f);

// Use in render loop
postProcessor->BeginScene();
// ... render scene ...
postProcessor->EndScene();
postProcessor->Process();
```

**Technical Details**:
- Ping-pong framebuffers for multi-pass blur
- 5-tap Gaussian kernel with optimized weights
- Separable blur (horizontal + vertical)
- HDR intermediate buffers

---

### 6. Skybox System
**Files**: `Rendering/Skybox.h`, `Rendering/Skybox.cpp`

Environment mapping with cubemap textures:
- **Loading**: 6-face cubemap or HDR equirectangular
- **Rendering**: Optimized with depth testing tricks
- **Shader**: Removes translation for infinite distance effect
- **Integration**: Scene-level skybox management

```cpp
// Load cubemap skybox
std::vector<std::string> faces = {
    "right.jpg",  // +X
    "left.jpg",   // -X
    "top.jpg",    // +Y
    "bottom.jpg", // -Y
    "front.jpg",  // +Z
    "back.jpg"    // -Z
};
auto cubemap = std::make_shared<Cubemap>(faces);
auto skybox = std::make_shared<Skybox>(cubemap);

// Add to scene
scene->SetSkybox(skybox);

// Render skybox (done automatically in editor)
if (scene->GetSkybox()) {
    Renderer::DrawSkybox(scene->GetSkybox());
}
```

**Shader Features**:
- View matrix without translation
- Depth test optimization (GL_LEQUAL)
- Cubemap sampling with direction vectors

---

## Shader Architecture

### Basic Shader
- **Vertex**: 8 floats per vertex (position, normal, UV)
- **Features**: Phong lighting, shadow mapping, texture sampling
- **Uniforms**: Transform, ViewProjection, Light properties, Shadow map

### Shadow Depth Shader
- **Purpose**: Generate depth buffer for shadow mapping
- **Optimization**: Minimal fragment processing
- **Output**: Depth values in light space

### PBR Shader
- **Vertex**: Full attribute pass-through
- **Fragment**: Cook-Torrance BRDF computation
- **Functions**: DistributionGGX, GeometrySmith, FresnelSchlick
- **Lighting**: Multiple light accumulation

### Skybox Shader
- **Vertex**: Position-based direction vectors
- **Fragment**: Cubemap texture sampling
- **Optimization**: xyww depth trick for far plane

### Post-Processing Shaders
- **Bloom Extract**: Brightness threshold filter
- **Gaussian Blur**: 5-tap separable blur
- **Final Composite**: ACES tone mapping + vignette + gamma

---

## Performance Considerations

### Texture System
- Automatic mipmap generation for distant objects
- Texture binding cache to reduce state changes
- Multiple texture units (0-7) for complex materials

### Model Loading
- Mesh batching for multi-mesh models
- Material sorting to reduce shader switches
- Vertex buffer optimization with interleaved data

### Shadow Mapping
- 2048x2048 resolution balances quality and performance
- PCF 3x3 kernel for soft edges
- Single directional light (extendable to multiple)

### PBR Rendering
- Per-fragment lighting for accuracy
- Texture map support reduces uniform updates
- HDR pipeline for extended dynamic range

### Post-Processing
- Half-resolution blur passes for performance
- Separable Gaussian blur (N passes vs N² samples)
- Ping-pong buffers for multi-pass efficiency

### Skybox
- Rendered last with depth test optimization
- Single draw call (36 vertices)
- No translation reduces vertex processing

---

## Integration Examples

### Complete Rendering Setup

```cpp
#include <Kaya.h>

class MyGame : public Kaya::Application {
public:
    void OnStart() override {
        // Setup lighting
        m_Light = std::make_shared<DirectionalLight>();
        m_Light->Direction = glm::normalize(glm::vec3(-1, -1, -1));
        m_Light->CastShadows = true;
        Renderer::SetDirectionalLight(m_Light.get());
        
        // Setup shadows
        m_ShadowMap = std::make_shared<ShadowMap>();
        Renderer::SetShadowMap(m_ShadowMap.get());
        
        // Setup post-processing
        m_PostProcessor = std::make_shared<PostProcessor>(1920, 1080);
        m_PostProcessor->SetBloomEnabled(true);
        
        // Load skybox
        auto cubemap = std::make_shared<Cubemap>(skyboxFaces);
        m_Skybox = std::make_shared<Skybox>(cubemap);
        
        // Load model
        auto model = std::make_shared<Model>("assets/character.gltf");
        auto entity = m_Scene->CreateEntity("Character");
        entity->GetRender().GeometryType = RenderComponent::Type::Model;
        entity->GetRender().ModelAsset = model;
    }
    
    void OnUpdate(float deltaTime) override {
        // Shadow pass
        Renderer::BeginShadowPass();
        // ... render shadow-casting objects ...
        Renderer::EndShadowPass();
        
        // Main render pass
        m_PostProcessor->BeginScene();
        Renderer::BeginScene(*m_Camera);
        
        // Render scene objects
        for (auto& entity : m_Scene->GetEntities()) {
            // ... render entities ...
        }
        
        Renderer::EndScene();
        
        // Draw skybox
        Renderer::DrawSkybox(m_Skybox.get());
        
        m_PostProcessor->EndScene();
        m_PostProcessor->Process();
    }
    
private:
    std::shared_ptr<DirectionalLight> m_Light;
    std::shared_ptr<ShadowMap> m_ShadowMap;
    std::shared_ptr<PostProcessor> m_PostProcessor;
    std::shared_ptr<Skybox> m_Skybox;
};
```

---

## Future Enhancements

Potential improvements for the rendering system:

1. **Normal Mapping** - Implement normal map support in PBR shader
2. **Multiple Lights** - Point lights, spot lights, area lights
3. **Cascaded Shadow Maps** - Better shadow quality for large scenes
4. **IBL** - Image-Based Lighting with irradiance maps
5. **SSAO** - Screen-Space Ambient Occlusion
6. **Deferred Rendering** - G-buffer based rendering for many lights
7. **TAA** - Temporal Anti-Aliasing
8. **Volumetric Lighting** - God rays and fog effects

---

## Resources

- [Learn OpenGL - PBR Theory](https://learnopengl.com/PBR/Theory)
- [Learn OpenGL - Shadow Mapping](https://learnopengl.com/Advanced-Lighting/Shadows/Shadow-Mapping)
- [Learn OpenGL - HDR](https://learnopengl.com/Advanced-Lighting/HDR)
- [Real Shading in Unreal Engine 4](https://blog.selfshadow.com/publications/s2013-shading-course/)
- [Assimp Documentation](https://assimp-docs.readthedocs.io/)

---

**Last Updated**: 2024
**Engine Version**: 1.0.0
