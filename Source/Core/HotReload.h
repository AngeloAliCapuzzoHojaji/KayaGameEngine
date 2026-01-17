#pragma once

#include <memory>
#include <string>

namespace Kaya {

// Forward declarations
class Camera;
class PhysicsSystem;
class Scene;
class Window;

// Game state that persists across reloads
struct GameState {
    std::shared_ptr<Camera> Camera;
    std::shared_ptr<PhysicsSystem> Physics;
    std::shared_ptr<Scene> Scene;
    Window* Window = nullptr;
    
    // Custom user data (can store any game-specific state)
    void* UserData = nullptr;
    size_t UserDataSize = 0;
    
    float DeltaTime = 0.0f;
    float TotalTime = 0.0f;
    bool FirstFrame = true;
};

// Interface that hot-reloadable games must implement
class IGameModule {
public:
    virtual ~IGameModule() = default;
    
    // Called once when the DLL is first loaded
    virtual void OnLoad(GameState* state) = 0;
    
    // Called every frame before reload check
    virtual void OnUpdate(GameState* state) = 0;
    
    // Called every frame for rendering
    virtual void OnRender(GameState* state) = 0;
    
    // Called when the DLL is about to be unloaded (before reload)
    virtual void OnUnload(GameState* state) = 0;
    
    // Optional: Called after successful reload
    virtual void OnReload(GameState* state) {}
};

// Function signatures for DLL exports
typedef IGameModule* (*CreateGameModuleFunc)();
typedef void (*DestroyGameModuleFunc)(IGameModule*);

} // namespace Kaya

// Macros to make implementing hot reload modules easier
#define KAYA_EXPORT extern "C" __declspec(dllexport)

#define KAYA_GAME_MODULE(ClassName) \
    KAYA_EXPORT Kaya::IGameModule* CreateGameModule() { \
        return new ClassName(); \
    } \
    KAYA_EXPORT void DestroyGameModule(Kaya::IGameModule* module) { \
        delete module; \
    }
