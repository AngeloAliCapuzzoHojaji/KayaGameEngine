# Kaya Game Engine - Packaging Script
# Creates a distributable zip file with all source code and dependencies included.
# End users only need CMake and a C++17 compiler - no manual dependency downloads.
#
# Usage:
#   .\scripts\package.ps1
#   .\scripts\package.ps1 -OutputName "KayaEngine-v1.2"

param(
    [string]$OutputName = "KayaGameEngine"
)

$ErrorActionPreference = "Stop"
$Root = Split-Path -Parent $PSScriptRoot
Set-Location $Root

Write-Host "Kaya Game Engine - Packaging" -ForegroundColor Cyan
Write-Host "==================================" -ForegroundColor Cyan

# Verify submodules are initialized
Write-Host "`nChecking git submodules..." -ForegroundColor Yellow
$submoduleStatus = git submodule status 2>&1
if ($LASTEXITCODE -ne 0) {
    Write-Host "Error: Not a git repository or git not found." -ForegroundColor Red
    exit 1
}

$missingSubmodules = $submoduleStatus | Where-Object { $_ -match "^-" }
if ($missingSubmodules) {
    Write-Host "Initializing missing submodules..." -ForegroundColor Yellow
    git submodule update --init --recursive
    if ($LASTEXITCODE -ne 0) {
        Write-Host "Error: Failed to initialize submodules." -ForegroundColor Red
        exit 1
    }
}
Write-Host "All submodules present." -ForegroundColor Green

# Verify GLAD exists
if (-not (Test-Path "ThirdParty/glad/include/glad/glad.h")) {
    Write-Host "Error: GLAD not found at ThirdParty/glad/." -ForegroundColor Red
    Write-Host "Generate it from https://glad.dav1d.de/ (C/C++, OpenGL 4.6 Core)" -ForegroundColor Yellow
    exit 1
}
Write-Host "GLAD found." -ForegroundColor Green

# Create staging directory
$stagingDir = Join-Path $env:TEMP "$OutputName"
if (Test-Path $stagingDir) {
    Remove-Item $stagingDir -Recurse -Force
}
New-Item -ItemType Directory -Path $stagingDir | Out-Null

Write-Host "`nCopying engine source..." -ForegroundColor Yellow

# Directories and files to include
$includeDirs = @("Source", "Examples", "ThirdParty")
$includeFiles = @(
    "CMakeLists.txt",
    "README.md",
    "SETUP.md",
    "RENDERING_FEATURES.md",
    "CHANGES.md",
    "LICENSE",
    "_config.yml"
)

# Copy root files
foreach ($file in $includeFiles) {
    if (Test-Path $file) {
        Copy-Item $file -Destination $stagingDir
    }
}

# Copy Source and Examples
foreach ($dir in @("Source", "Examples")) {
    if (Test-Path $dir) {
        Copy-Item $dir -Destination $stagingDir -Recurse
    }
}

Write-Host "Copying dependencies..." -ForegroundColor Yellow

# Copy ThirdParty (excluding .git dirs and build artifacts)
$thirdPartyDest = Join-Path $stagingDir "ThirdParty"
New-Item -ItemType Directory -Path $thirdPartyDest | Out-Null

# Copy ThirdParty CMakeLists.txt
Copy-Item "ThirdParty/CMakeLists.txt" -Destination $thirdPartyDest

# Copy each dependency, excluding .git directories and unnecessary files
$deps = @("glfw", "glm", "glad", "JoltPhysics", "imgui", "ImGuizmo", "assimp", "stb")
foreach ($dep in $deps) {
    $srcPath = "ThirdParty/$dep"
    if (Test-Path $srcPath) {
        Write-Host "  $dep" -ForegroundColor Gray
        $destPath = Join-Path $thirdPartyDest $dep

        # Use robocopy for fast copy excluding .git
        robocopy $srcPath $destPath /E /XD .git __pycache__ test tests doc docs samples tools /XF .gitignore .gitattributes .gitmodules /NFL /NDL /NJH /NJS /NC /NS /NP | Out-Null
    }
}

# Create a simple build script for end users
$buildScript = @"
@echo off
echo Kaya Game Engine - Build Script
echo ==================================
echo.

where cmake >nul 2>&1
if %errorlevel% neq 0 (
    echo ERROR: CMake not found. Install from https://cmake.org/download/
    pause
    exit /b 1
)

echo Configuring...
cmake -B build -S .
if %errorlevel% neq 0 (
    echo ERROR: CMake configuration failed.
    pause
    exit /b 1
)

echo.
echo Building (Release)...
cmake --build build --config Release
if %errorlevel% neq 0 (
    echo ERROR: Build failed.
    pause
    exit /b 1
)

echo.
echo Build complete!
echo Run examples from: build\bin\Release\
echo   - Sandbox.exe
echo   - RenderingDemo.exe
echo   - KayaEditor.exe
echo.
pause
"@

Set-Content -Path (Join-Path $stagingDir "build.bat") -Value $buildScript

# Create the zip
$zipPath = Join-Path $Root "$OutputName.zip"
if (Test-Path $zipPath) {
    Remove-Item $zipPath -Force
}

Write-Host "`nCreating zip archive..." -ForegroundColor Yellow
Compress-Archive -Path "$stagingDir\*" -DestinationPath $zipPath -CompressionLevel Optimal

# Cleanup staging
Remove-Item $stagingDir -Recurse -Force

# Report
$zipSize = (Get-Item $zipPath).Length / 1MB
Write-Host "`nPackage created successfully!" -ForegroundColor Green
Write-Host "  File: $zipPath" -ForegroundColor Cyan
Write-Host "  Size: $([math]::Round($zipSize, 1)) MB" -ForegroundColor Cyan
Write-Host "`nEnd users just need to:" -ForegroundColor Yellow
Write-Host "  1. Unzip" -ForegroundColor White
Write-Host "  2. Run build.bat (or cmake -B build -S . && cmake --build build --config Release)" -ForegroundColor White
Write-Host "  3. Run build\bin\Release\Sandbox.exe" -ForegroundColor White
