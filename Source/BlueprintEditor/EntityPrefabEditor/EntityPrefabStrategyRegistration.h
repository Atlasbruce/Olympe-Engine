/**
 * @file EntityPrefabStrategyRegistration.h
 * @brief Registration helper for EntityPrefab V2 Framework strategy (Phase 2.1 Chunk 2)
 * @author Olympe Engine
 * @date 2026-03-11
 *
 * @details
 * Provides the factory function to register EntityPrefab V2 (Framework) strategy
 * with DocumentVersionManager. This enables EntityPrefab graphs to be managed
 * through the framework's centralized document and renderer creation system.
 */

#pragma once

#include "../Framework/DocumentVersionManager.h"

namespace Olympe
{
    /**
     * @brief Create and configure the EntityPrefab Framework V2 strategy
     * @return DocumentCreationStrategy ready for registration
     *
     * @details
     * This function creates the complete strategy for EntityPrefab V2:
     * - New document factory: EntityPrefabGraphDocumentV2
     * - File loader: EntityPrefabGraphDocumentV2::LoadFromFile
     * - Renderer factory: EntityPrefabEditorV2
     *
     * Usage:
     * @code
     * DocumentVersionManager& manager = DocumentVersionManager::Get();
     * DocumentCreationStrategy strategy = CreateEntityPrefabFrameworkStrategy();
     * manager.RegisterStrategy("EntityPrefab", GraphTypeVersion::Framework, strategy);
     * @endcode
     */
    DocumentCreationStrategy CreateEntityPrefabFrameworkStrategy();

} // namespace Olympe
