# Kaya Game Engine - Setup Guide

## Quick Start (Windows)

### 1. Install Prerequisites

- **Visual Studio 2019/2022** with C++ development tools
- **CMake 3.20+**: Download from https://cmake.org/download/
- **Git**: Download from https://git-scm.com/

### 2. Clone Dependencies

Open PowerShell in the project root and run:

```powershell
# Create ThirdParty directory
New-Item -ItemType Directory -Force -Path ThirdParty

# Clone GLFW
git clone https://github.com/glfw/glfw.git ThirdParty/glfw

# Clone GLM
git clone https://github.com/g-truc/glm.git ThirdParty/glm

# Clone Jolt Physics
git clone https://github.com/jrouwe/JoltPhysics.git ThirdParty/JoltPhysics
cd ThirdParty/JoltPhysics
git checkout tags/v5.0.0
cd ../..
```

### 3. Generate GLAD

GLAD needs to be generated manually:

1. Go to https://glad.dav1d.de/
2. Settings:
   - Language: **C/C++**
   - API gl: **Version 4.6**
   - Profile: **Core**
3. Click **Generate**
4. Download the zip file
5. Extract to `ThirdParty/glad/`

Your folder structure should look like:
```
ThirdParty/glad/
├── include/
│   ├── glad/
│   │   └── glad.h
│   └── KHR/
│       └── khrplatform.h
└── src/
    └── glad.c
```

### 4. Build the Engine

```powershell
# Create build directory
mkdir build
cd build

# Generate Visual Studio solution
cmake ..

# Build (or open KayaGameEngine.sln in Visual Studio)
cmake --build . --config Release
```

### 5. Run the Sandbox

```powershell
.\bin\Release\Sandbox.exe
```

## Troubleshooting

### "Cannot find GLFW"
Make sure GLFW is cloned to `ThirdParty/glfw/` with the CMakeLists.txt file present.

### "GLAD not found"
Ensure you've generated GLAD from https://glad.dav1d.de/ and extracted it to `ThirdParty/glad/`.

### "Jolt Physics errors"
Make sure you're using Jolt Physics v5.0.0 or compatible version:
```powershell
cd ThirdParty/JoltPhysics
git checkout tags/v5.0.0
```

### Build errors
- Ensure you have C++17 support enabled
- Check that all dependencies are in the correct folders
- Try deleting the `build/` folder and regenerating

## Linux Setup

```bash
# Install dependencies
sudo apt-get install build-essential cmake git libx11-dev libxrandr-dev libxinerama-dev libxcursor-dev libxi-dev

# Clone dependencies (same as Windows)
git clone https://github.com/glfw/glfw.git ThirdParty/glfw
git clone https://github.com/g-truc/glm.git ThirdParty/glm
git clone https://github.com/jrouwe/JoltPhysics.git ThirdParty/JoltPhysics

# Generate GLAD (same as Windows)

# Build
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make -j$(nproc)

# Run
./bin/Sandbox
```

## macOS Setup

```bash
# Install Xcode Command Line Tools
xcode-select --install

# Install CMake
brew install cmake

# Clone dependencies and build (same as Linux)
```

## Development Tips

### Using Visual Studio
After running `cmake ..`, open `build/KayaGameEngine.sln` in Visual Studio. Set Sandbox as the startup project.

### Using VS Code
Install the CMake Tools extension and configure it to use your compiler.

### Custom Applications
Create new applications in the `Examples/` folder or your own directory, linking against `KayaEngine`.
