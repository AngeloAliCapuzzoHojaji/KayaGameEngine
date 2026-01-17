#include <Kaya.h>
#include "Core/HotReloadManager.h"
#include <iostream>
#include <GLFW/glfw3.h>

class HotReloadHost : public Kaya::Application {
public:
    HotReloadHost(const std::string& gameDllPath)
        : Application("Hot Reload Host"),
          m_HotReload(gameDllPath)
    {
    }
    
    void OnInit() override {
        std::cout << "Hot Reload Host initialized" << std::endl;
        std::cout << "Press F5 to manually reload, or edit code and it will auto-reload" << std::endl;
        
        // Initialize game state
        m_GameState.Window = &GetWindow();
        m_GameState.FirstFrame = true;
        
        // Load the game DLL
        if (!m_HotReload.Load()) {
            std::cerr << "Failed to load game DLL!" << std::endl;
            GetWindow().Close();
            return;
        }
        
        // Initialize the game
        m_HotReload.GetModule()->OnLoad(&m_GameState);
    }
    
    void OnUpdate(float deltaTime) override {
        m_GameState.DeltaTime = deltaTime;
        m_GameState.TotalTime += deltaTime;
        
        // Check for manual reload (F5)
        if (Kaya::Input::IsKeyPressed(Kaya::KeyCode::F5)) {
            if (!m_ReloadKeyPressed) {
                std::cout << "Manual reload triggered..." << std::endl;
                HandleReload();
                m_ReloadKeyPressed = true;
            }
        } else {
            m_ReloadKeyPressed = false;
        }
        
        // Auto-reload check (every 0.5 seconds)
        m_ReloadCheckTimer += deltaTime;
        if (m_ReloadCheckTimer >= 0.5f) {
            m_ReloadCheckTimer = 0.0f;
            if (m_HotReload.CheckAndReload()) {
                HandleReload();
            }
        }
        
        // Update game
        if (m_HotReload.IsLoaded()) {
            m_HotReload.GetModule()->OnUpdate(&m_GameState);
        }
        
        m_GameState.FirstFrame = false;
    }
    
    void OnRender() override {
        // Render game
        if (m_HotReload.IsLoaded()) {
            m_HotReload.GetModule()->OnRender(&m_GameState);
        }
    }
    
    void OnShutdown() override {
        if (m_HotReload.IsLoaded()) {
            m_HotReload.GetModule()->OnUnload(&m_GameState);
        }
        
        // Clean up persistent state
        if (m_GameState.Physics) {
            m_GameState.Physics->Shutdown();
        }
    }
    
private:
    void HandleReload() {
        // Notify old module it's being unloaded
        if (m_HotReload.IsLoaded()) {
            m_HotReload.GetModule()->OnUnload(&m_GameState);
        }
        
        // New module is now loaded, call OnLoad and OnReload
        if (m_HotReload.IsLoaded()) {
            m_HotReload.GetModule()->OnLoad(&m_GameState);
            m_HotReload.GetModule()->OnReload(&m_GameState);
        }
    }
    
    Kaya::HotReloadManager m_HotReload;
    Kaya::GameState m_GameState;
    float m_ReloadCheckTimer = 0.0f;
    bool m_ReloadKeyPressed = false;
};

Kaya::Application* Kaya::CreateApplication() {
    // Use absolute path or relative from exe location
    char buffer[MAX_PATH];
    GetModuleFileNameA(NULL, buffer, MAX_PATH);
    std::string exePath(buffer);
    std::string exeDir = exePath.substr(0, exePath.find_last_of("\\/"));
    std::string dllPath = exeDir + "\\FPSGame.dll";
    
    return new HotReloadHost(dllPath);
}
