#include "Core/HotReloadManager.h"
#include <iostream>
#include <filesystem>

namespace Kaya {

HotReloadManager::HotReloadManager(const std::string& dllPath)
    : m_DllPath(dllPath)
{
    m_TempDllPath = dllPath + ".temp";
}

HotReloadManager::~HotReloadManager() {
    Unload();
}

bool HotReloadManager::Load() {
    if (m_DllHandle) {
        std::cout << "DLL already loaded, unloading first..." << std::endl;
        Unload();
    }
    
    // Copy DLL to temp location to allow recompilation
    if (!CopyDllToTemp()) {
        std::cerr << "Failed to copy DLL to temp location" << std::endl;
        return false;
    }
    
    if (!LoadDll(m_TempDllPath)) {
        return false;
    }
    
    m_LastWriteTime = GetFileWriteTime(m_DllPath);
    std::cout << "Hot reload: DLL loaded successfully" << std::endl;
    return true;
}

bool HotReloadManager::Reload() {
    std::cout << "Hot reload: Reloading DLL..." << std::endl;
    
    // Unload current DLL
    UnloadDll();
    
    // Small delay to ensure file operations complete
    Sleep(100);
    
    // Copy new version
    if (!CopyDllToTemp()) {
        std::cerr << "Failed to copy DLL to temp location" << std::endl;
        return false;
    }
    
    // Load new version
    if (!LoadDll(m_TempDllPath)) {
        std::cerr << "Failed to load new DLL version" << std::endl;
        return false;
    }
    
    m_LastWriteTime = GetFileWriteTime(m_DllPath);
    std::cout << "Hot reload: DLL reloaded successfully!" << std::endl;
    return true;
}

void HotReloadManager::Unload() {
    UnloadDll();
    
    // Clean up temp file
    if (std::filesystem::exists(m_TempDllPath)) {
        std::filesystem::remove(m_TempDllPath);
    }
}

bool HotReloadManager::CheckAndReload() {
    if (!std::filesystem::exists(m_DllPath)) {
        return false;
    }
    
    FILETIME currentWriteTime = GetFileWriteTime(m_DllPath);
    
    // Compare file times
    if (CompareFileTime(&currentWriteTime, &m_LastWriteTime) > 0) {
        return Reload();
    }
    
    return false;
}

bool HotReloadManager::LoadDll(const std::string& path) {
    m_DllHandle = LoadLibraryA(path.c_str());
    if (!m_DllHandle) {
        DWORD error = GetLastError();
        std::cerr << "Failed to load DLL: " << path << " (Error: " << error << ")" << std::endl;
        return false;
    }
    
    // Get function pointers
    m_CreateFunc = (CreateGameModuleFunc)GetProcAddress(m_DllHandle, "CreateGameModule");
    m_DestroyFunc = (DestroyGameModuleFunc)GetProcAddress(m_DllHandle, "DestroyGameModule");
    
    if (!m_CreateFunc || !m_DestroyFunc) {
        std::cerr << "Failed to get function pointers from DLL" << std::endl;
        FreeLibrary(m_DllHandle);
        m_DllHandle = nullptr;
        return false;
    }
    
    // Create module instance
    m_Module = m_CreateFunc();
    if (!m_Module) {
        std::cerr << "Failed to create game module" << std::endl;
        FreeLibrary(m_DllHandle);
        m_DllHandle = nullptr;
        return false;
    }
    
    return true;
}

void HotReloadManager::UnloadDll() {
    if (m_Module && m_DestroyFunc) {
        m_DestroyFunc(m_Module);
        m_Module = nullptr;
    }
    
    if (m_DllHandle) {
        FreeLibrary(m_DllHandle);
        m_DllHandle = nullptr;
    }
    
    m_CreateFunc = nullptr;
    m_DestroyFunc = nullptr;
}

FILETIME HotReloadManager::GetFileWriteTime(const std::string& path) {
    FILETIME lastWriteTime = {};
    
    WIN32_FILE_ATTRIBUTE_DATA data;
    if (GetFileAttributesExA(path.c_str(), GetFileExInfoStandard, &data)) {
        lastWriteTime = data.ftLastWriteTime;
    }
    
    return lastWriteTime;
}

bool HotReloadManager::CopyDllToTemp() {
    try {
        std::filesystem::copy_file(
            m_DllPath,
            m_TempDllPath,
            std::filesystem::copy_options::overwrite_existing
        );
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Failed to copy DLL: " << e.what() << std::endl;
        return false;
    }
}

} // namespace Kaya
