/*
	Olympe Engine V2 - 2025
	Nicolas Chereau
	nchereau@gmail.com

	Purpose:
	- Header file for PrefabFactory class, responsible for creating game object prefabs.
*/
#pragma once
#include "world.h"
#include "system/system_utils.h"
#include <map>

using PrefabBuilder = std::function<void(EntityID)>;

class PrefabFactory
{
public:
    static PrefabFactory& Get()
    {
        static PrefabFactory instance;
        return instance;
    }
    // Example: Register("MiliceGuard", [](World& w, EntityID id) { ... });
    void RegisterPrefab(const std::string& name, PrefabBuilder builder)
    {
        m_prefabs[name] = builder;
       SYSTEM_LOG << "PrefabFactory::RegisteredPrefab has registered: " << name << "\n";
    }

    // create an Entity 
    EntityID CreateEntity(const std::string& prefabName)
    {
        if (m_prefabs.find(prefabName) == m_prefabs.end())
        {
            SYSTEM_LOG << "PrefabFactory::RegisteredPrefab"  << "Error: Prefab '" << prefabName << "' unknown.\n";
            return INVALID_ENTITY_ID;
        }

        // 1. Créer l'ID unique via le World (UID Nanoseconde)
        World& world = World::Get();
        EntityID newEntity = world.CreateEntity();

        // 2. Appliquer la recette (ajouter les composants)
        m_prefabs[prefabName](newEntity);

        SYSTEM_LOG << "PrefabFactory::CreateEntity '" << prefabName << "' created (ID: " << newEntity << ")\n";
        return newEntity;
    }

private:
    std::map<std::string, PrefabBuilder> m_prefabs;

    PrefabFactory() = default;
};

// Macro pour faciliter l'enregistrement automatique (similaire à votre ancienne méthode)
#define REGISTER_PREFAB(Name, BuilderLambda) \
    namespace { \
        struct AutoRegister##Name { \
            AutoRegister##Name() { \
                PrefabFactory::Get().RegisterPrefab(#Name, BuilderLambda); \
            } \
        }; \
        AutoRegister##Name global_##Name; \
    }