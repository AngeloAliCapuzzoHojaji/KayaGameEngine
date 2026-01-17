#include "Scene.h"
#include <algorithm>
#include "../Physics/PhysicsSystem.h"

namespace Kaya {

    Scene::Scene(const std::string& name)
        : m_Name(name)
    {
    }

    Entity* Scene::CreateEntity(const std::string& name) {
        auto entity = std::make_shared<Entity>(name);
        m_Entities.push_back(entity);
        return entity.get();
    }

    void Scene::RemoveEntity(Entity* entity) {
        if (!entity) return;

        auto it = std::find_if(m_Entities.begin(), m_Entities.end(),
            [entity](const std::shared_ptr<Entity>& e) {
                return e.get() == entity;
            });

        if (it != m_Entities.end()) {
            m_Entities.erase(it);
        }
    }

    void Scene::Clear() {
        m_Entities.clear();
    }

    Entity* Scene::GetSelectedEntity() const {
        for (const auto& entity : m_Entities) {
            if (entity->IsSelected()) {
                return entity.get();
            }
        }
        return nullptr;
    }

    void Scene::SetSelectedEntity(Entity* entity) {
        // Clear all selections first
        for (auto& e : m_Entities) {
            e->SetSelected(false);
        }

        // Set the new selection
        if (entity) {
            entity->SetSelected(true);
        }
    }

    void Scene::ClearSelection() {
        for (auto& entity : m_Entities) {
            entity->SetSelected(false);
        }
    }

    void Scene::Update(float deltaTime, PhysicsSystem* physics) {
        // Sync entities with physics
        if (physics) {
            for (auto& entity : m_Entities) {
                if (entity->HasPhysics()) {
                    entity->SyncFromPhysics(physics);
                }
            }
        }
    }

}
