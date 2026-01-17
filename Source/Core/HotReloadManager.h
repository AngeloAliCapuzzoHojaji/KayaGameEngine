#pragma once

#include "Core/HotReload.h"
#include <Windows.h>
#include <string>
#include <functional>

namespace Kaya {

class HotReloadManager {
public:
    HotReloadManager(const std::string& dllPath);
    ~HotReloadManager();
    
    // Load or reload the DLL
    bool Load();
    bool Reload();
    void Unload();
    
    // Check if DLL has been modified and reload if necessary
    bool CheckAndReload();
    
    // Get the game module interface
    IGameModule* GetModule() { return m_Module; }
    
    // Check if module is loaded
    bool IsLoaded() const { return m_Module != nullptr; }
    
private:
    std::string m_DllPath;
    std::string m_TempDllPath;
    HMODULE m_DllHandle = nullptr;
    IGameModule* m_Module = nullptr;
    
    CreateGameModuleFunc m_CreateFunc = nullptr;
    DestroyGameModuleFunc m_DestroyFunc = nullptr;
    
    FILETIME m_LastWriteTime = {};
    
    bool LoadDll(const std::string& path);
    void UnloadDll();
    FILETIME GetFileWriteTime(const std::string& path);
    bool CopyDllToTemp();
};

} // namespace Kaya
