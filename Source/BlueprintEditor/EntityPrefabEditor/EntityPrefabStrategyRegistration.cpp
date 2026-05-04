/**
 * @file EntityPrefabStrategyRegistration.cpp
 * @brief Registration implementation for EntityPrefab V2 Framework strategy (Phase 2.1 Chunk 2)
 * @author Olympe Engine
 * @date 2026-03-11
 */

#include "EntityPrefabStrategyRegistration.h"
#include "EntityPrefabGraphDocumentV2.h"
#include "EntityPrefabEditorV2.h"
#include "../Framework/IGraphDocument.h"
#include "../Framework/IGraphRenderer.h"
#include "../../system/system_utils.h"

namespace Olympe
{
    DocumentCreationStrategy CreateEntityPrefabFrameworkStrategy()
    {
        SYSTEM_LOG << "[EntityPrefabStrategyRegistration] Creating Framework V2 strategy" << std::endl;

        DocumentCreationStrategy strategy;

        // ====================================================================
        // Factory 1: Create New Document
        // ====================================================================
        strategy.createNewDocument = []() -> IGraphDocument*
        {
            SYSTEM_LOG << "[EntityPrefabStrategyRegistration] Factory: createNewDocument called" << std::endl;

            auto doc = new EntityPrefabGraphDocumentV2();

            SYSTEM_LOG << "[EntityPrefabStrategyRegistration] Factory: New EntityPrefabGraphDocumentV2 created" << std::endl;

            return doc;
        };

        // ====================================================================
        // Factory 2: Load Document From File
        // ====================================================================
        strategy.loadDocumentFromFile = [](const std::string& filePath) -> IGraphDocument*
        {
            SYSTEM_LOG << "[EntityPrefabStrategyRegistration] Factory: loadDocumentFromFile called: " << filePath << std::endl;

            auto doc = new EntityPrefabGraphDocumentV2();

            if (!doc->Load(filePath))
            {
                SYSTEM_LOG << "[EntityPrefabStrategyRegistration] ERROR: Load failed for: " << filePath << std::endl;
                delete doc;
                return nullptr;
            }

            SYSTEM_LOG << "[EntityPrefabStrategyRegistration] Factory: Loaded successfully: " << filePath << std::endl;

            return doc;
        };

        // ====================================================================
        // Factory 3: Create Renderer
        // ====================================================================
        strategy.createRenderer = [](IGraphDocument* document) -> IGraphRenderer*
        {
            if (!document)
            {
                SYSTEM_LOG << "[EntityPrefabStrategyRegistration] ERROR: createRenderer called with nullptr document" << std::endl;
                return nullptr;
            }

            SYSTEM_LOG << "[EntityPrefabStrategyRegistration] Factory: createRenderer called" << std::endl;

            auto v2Doc = static_cast<EntityPrefabGraphDocumentV2*>(document);

            auto renderer = new EntityPrefabEditorV2(v2Doc);

            SYSTEM_LOG << "[EntityPrefabStrategyRegistration] Factory: EntityPrefabEditorV2 created successfully" << std::endl;

            return renderer;
        };

        // ====================================================================
        // Strategy Metadata
        // ====================================================================
        strategy.strategyName = "Framework EntityPrefab V2";
        strategy.version = GraphTypeVersion::Framework;

        SYSTEM_LOG << "[EntityPrefabStrategyRegistration] Strategy created: " << strategy.strategyName << std::endl;

        return strategy;
    }

} // namespace Olympe
