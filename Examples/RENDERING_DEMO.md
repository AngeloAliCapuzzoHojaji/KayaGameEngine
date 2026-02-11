# Rendering Demo - User Guide

## Overview

The **RenderingDemo** is a comprehensive showcase of all advanced rendering features in Kaya Game Engine. This interactive demo allows you to explore and toggle different rendering techniques in real-time.

## Running the Demo

### Windows
```bash
cd build/bin/Release
./RenderingDemo.exe
```

### Build from Source
```bash
# From project root
cd build
cmake --build . --config Release
cd bin/Release
./RenderingDemo.exe
```

## Controls

### Camera Movement
- **W/A/S/D** - Move forward/left/backward/right
- **Mouse** - Look around (rotate camera view)
- **Space** - Move up
- **Left Shift** - Move down

### Feature Toggles
- **1** - Toggle Shadow Mapping (ON/OFF)
- **2** - Toggle Post-Processing (Bloom, Tone Mapping, Vignette)
- **3** - Toggle Skybox Rendering
- **4** - Cycle through demo scenes

### General
- **ESC** - Exit application

## Demo Scenes

The demo includes three distinct scenes, each highlighting different rendering capabilities:

### Scene 1: Texture Demo
**What it shows:**
- Texture loading and UV mapping
- Texture coordinate support in shaders
- Multiple textured objects with different materials

**Visual features:**
- Grid of colored cubes (simulating texture variations)
- Ground plane with texture mapping
- Rotating objects demonstrating UV coordinate handling

### Scene 2: PBR Materials Demo
**What it shows:**
- Physically Based Rendering (PBR) with metallic-roughness workflow
- Cook-Torrance BRDF implementation
- Material variations (metallic vs dielectric)

**Visual features:**
- 5x5 grid of spheres with varying metallic (rows) and roughness (columns) values
- Two bright emissive spheres demonstrating bloom effect
- Realistic material response to lighting
- HDR rendering pipeline

**Material ranges:**
- **Metallic**: 0.0 (dielectric) to 1.0 (metallic)
- **Roughness**: 0.05 (smooth/glossy) to 1.0 (rough/matte)

### Scene 3: Shadow Mapping Demo
**What it shows:**
- Real-time shadow mapping with directional light
- PCF (Percentage Closer Filtering) for soft shadows
- Shadow casting and receiving on complex geometry

**Visual features:**
- Circular arrangement of tall pillars casting shadows
- Central sphere as shadow receiver
- Dynamic shadow updates with camera movement
- 2048x2048 shadow map resolution

## Rendering Features Demonstrated

### 1. Texture System
- **Format Support**: PNG, JPG, TGA (via STB Image)
- **Filtering**: Linear, Nearest, Mipmaps
- **Wrapping**: Repeat, Clamp, Mirror
- **UV Mapping**: Full texture coordinate support

### 2. Model Loading
- **Formats**: OBJ, GLTF, FBX (via Assimp)
- **Features**: Multi-mesh support, automatic material/texture loading
- **Data**: Positions, normals, UVs

*Note: Current demo uses procedural geometry. To test model loading, add your own 3D models in supported formats.*

### 3. Shadow Mapping
- **Technique**: Depth buffer shadow mapping
- **Resolution**: 2048x2048 shadow map
- **Filtering**: PCF 3x3 kernel for soft edges
- **Light Type**: Directional light with adjustable direction

**Technical details:**
```cpp
// Shadow map setup
m_Light->Direction = glm::normalize(glm::vec3(-0.5f, -1.0f, -0.3f));
m_Light->CastShadows = true;
m_ShadowMap = std::make_shared<ShadowMap>(2048, 2048);
```

### 4. PBR (Physically Based Rendering)
- **Workflow**: Metallic-Roughness
- **BRDF**: Cook-Torrance with:
  - **Distribution**: GGX (Trowbridge-Reitz)
  - **Geometry**: Smith's Schlick-GGX
  - **Fresnel**: Schlick's approximation
- **Lighting**: HDR with tone mapping

**Material parameters:**
- Albedo (base color)
- Metallic (0 = dielectric, 1 = metallic)
- Roughness (0 = smooth, 1 = rough)
- Ambient Occlusion

### 5. Post-Processing Pipeline
**Effects included:**

#### Bloom
- Bright area extraction (threshold-based)
- Gaussian blur with ping-pong buffers
- 10 blur passes for smooth glow
- Configurable threshold and intensity

#### Tone Mapping
- ACES filmic tone mapping curve
- Exposure control
- HDR to LDR conversion

#### Vignette
- Edge darkening effect
- Adjustable strength (default: 0.35)
- Enhances focus on center of screen

#### Gamma Correction
- sRGB output (gamma 2.2)
- Proper color space conversion

### 6. Skybox System
- **Format**: Cubemap (6 faces)
- **Rendering**: Optimized depth testing (GL_LEQUAL)
- **Shader**: View matrix without translation for infinite distance
- **Support**: HDR environment maps

**Note**: Skybox is disabled by default in the demo. To enable it:

1. Create a `skybox` folder in the executable directory (`build/bin/Release/skybox`)
2. Add 6 cubemap texture files:
   - `right.jpg` (+X)
   - `left.jpg` (-X)
   - `top.jpg` (+Y)
   - `bottom.jpg` (-Y)
   - `front.jpg` (+Z)
   - `back.jpg` (-Z)
3. Uncomment the skybox creation code in `RenderingDemo.cpp`:

```cpp
// In CreateSkybox() method:
std::vector<std::string> faces = {
    "skybox/right.jpg",
    "skybox/left.jpg",
    "skybox/top.jpg",
    "skybox/bottom.jpg",
    "skybox/front.jpg",
    "skybox/back.jpg"
};
auto cubemap = std::make_shared<Cubemap>(faces);
m_Skybox = std::make_shared<Skybox>(cubemap);
```

4. Rebuild and press **3** to toggle skybox rendering

## Performance Information

The demo displays real-time FPS counter in the console output.

**Expected performance** (on modern hardware):
- **1920x1080**: 200-500+ FPS
- **Shadow Pass**: ~5-10% overhead
- **Post-Processing**: ~10-15% overhead
- **Full Pipeline**: 150-300+ FPS

Performance varies based on:
- GPU capabilities
- Number of objects in scene
- Shadow map resolution
- Post-processing complexity

## Learning Resources

### Code Structure
The demo is organized to be easy to understand:

```cpp
OnInit() {
    // Setup lighting, shadows, post-processing
    // Create demo scenes
}

OnUpdate(float deltaTime) {
    // Handle input
    // Shadow pass
    // Main render pass
    // Post-processing
}
```

### Key Rendering Pipeline
```
1. Shadow Pass (if enabled)
   └─ Render scene from light's perspective to shadow map

2. Main Render Pass
   ├─ Begin post-processing framebuffer (if enabled)
   ├─ Begin scene (set camera, clear, setup shaders)
   ├─ Render all entities
   │  ├─ Cubes (textured or colored)
   │  ├─ Spheres (PBR materials)
   │  └─ Models (if loaded)
   └─ End scene

3. Skybox Rendering (if enabled)
   └─ Render cubemap with depth optimization

4. Post-Processing (if enabled)
   ├─ Bloom extraction
   ├─ Gaussian blur (ping-pong)
   ├─ Final composition (tone mapping + vignette + gamma)
   └─ Output to screen
```

## 🔧 Customization

### Adding Your Own Models
```cpp
// In OnInit(), add:
auto model = std::make_shared<Model>("path/to/model.gltf");
auto entity = std::make_shared<Entity>("MyModel");
entity->GetTransform().Position = glm::vec3(0, 0, 0);
entity->GetRender().GeometryType = RenderComponent::Type::Model;
entity->GetRender().ModelAsset = model;
m_TextureScene.push_back(entity);  // Or other scene
```

### Adjusting Post-Processing
```cpp
// In OnInit(), modify:
m_PostProcessor->SetBloomThreshold(1.0f);    // Brightness threshold
m_PostProcessor->SetExposure(1.2f);          // Tone mapping exposure
m_PostProcessor->SetVignetteStrength(0.35f); // Edge darkening
```

### Changing Light Direction
```cpp
// In OnInit(), modify:
m_Light->Direction = glm::normalize(glm::vec3(-1.0f, -1.0f, 0.0f));
m_Light->Intensity = 2.0f;
m_Light->Color = glm::vec3(1.0f, 0.9f, 0.8f); // Warm sunset color
```

## 🐛 Troubleshooting

### Low FPS
- Try disabling post-processing (press **2**)
- Try disabling shadows (press **1**)
- Reduce shadow map resolution in code
- Ensure GPU drivers are up to date

### Black Screen
- Check that OpenGL 4.6 Core is supported
- Verify GLFW/GLAD initialization
- Check console output for error messages

### Missing Skybox
- Default uses placeholder cubemap
- Add actual texture files to see proper skybox
- Press **3** to toggle skybox rendering

### Compilation Errors
```bash
# Reconfigure CMake
cmake -B build -DCMAKE_BUILD_TYPE=Release

# Clean build
rm -rf build
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

## 📚 Additional Documentation

- [RENDERING_FEATURES.md](../RENDERING_FEATURES.md) - Detailed rendering system documentation
- [README.md](../README.md) - Engine overview and setup guide
- [SETUP.md](../SETUP.md) - Build instructions

## 🎯 Next Steps

After exploring the demo, try:

1. **Modify scenes** - Add your own objects and materials
2. **Load models** - Import your 3D models (OBJ, GLTF, FBX)
3. **Add textures** - Load texture images for realistic materials
4. **Experiment with lighting** - Change light direction, color, intensity
5. **Tune post-processing** - Adjust bloom, exposure, vignette values
6. **Create new scenes** - Build your own demo scene showcasing specific features

## 💡 Tips

- Use **Scene 1** to understand basic texture mapping
- Use **Scene 2** to understand PBR material properties
- Use **Scene 3** to see shadow quality and performance
- Toggle features on/off to understand their impact
- Watch console for FPS and scene info
- Move camera to different angles to see lighting/shadow changes

---

**Enjoy exploring the rendering capabilities of Kaya Game Engine!** 🚀
