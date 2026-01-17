#pragma once

#include <vector>
#include <memory>
#include <string>
#include "Entity.h"

namespace Kaya {

    class PhysicsSystem;

    class Scene {
    public:
        Scene(const std::string& name = "Untitled Scene");
        ~Scene() = default;

        // Scene management
        const std::string& GetName() const { return m_Name; }
        void SetName(const std::string& name) { m_Name = name; }

        // Entity management
        Entity* CreateEntity(const std::string& name = "Entity");
        void RemoveEntity(Entity* entity);
        void Clear();

        const std::vector<std::shared_ptr<Entity>>& GetEntities() const { return m_Entities; }
        
        // Selection
        Entity* GetSelectedEntity() const;
        void SetSelectedEntity(Entity* entity);
        void ClearSelection();

        // Update
        void Update(float deltaTime, PhysicsSystem* physics);

    private:
        std::string m_Name;
        std::vector<std::shared_ptr<Entity>> m_Entities;
    };

}
