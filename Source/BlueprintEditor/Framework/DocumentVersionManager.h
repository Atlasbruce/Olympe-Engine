#pragma once

#include <string>
#include <map>
#include <functional>
#include <memory>
#include <vector>
#include "../Framework/IGraphDocument.h"

namespace Olympe
{
    // Forward declarations
    class IGraphRenderer;

    /**
     * @enum GraphTypeVersion
     * @brief Document version for each graph type
     * 
     * Strategy: Centralized versioning system that scales to multiple graph types.
     * Allows fine-grained control over which implementation is used for each graph type.
     * 
     * Example progression:
     * - VisualScript: V1 (legacy imnodes) → V2 (framework) → V3 (future improvements)
     * - BehaviorTree: V1 (legacy NodeGraphPanel) → V2 (framework) → V3 (future)
     * - EntityPrefab: V1 (legacy) → V2 (framework) → V3 (future)
     */
    enum class GraphTypeVersion
    {
        Legacy = 1,      // V1: Original implementation
        Framework = 2,   // V2: Framework-compliant with IGraphDocument
        Next = 3,        // V3: Future enhancements (reserved)
    };

    /**
     * @struct DocumentCreationStrategy
     * @brief Factory strategy for creating documents and renderers
     * 
     * Decouples creation logic from version selection.
     * Supports different implementations for same graph type.
     */
    struct DocumentCreationStrategy
    {
        // Factory function to create a new empty document
        std::function<IGraphDocument*()> createNewDocument;

        // Factory function to load document from file
        std::function<IGraphDocument*(const std::string& filePath)> loadDocumentFromFile;

        // Factory function to create renderer (optional, can be nullptr)
        std::function<IGraphRenderer*(IGraphDocument* document)> createRenderer;

        // Display name for diagnostics and logging
        std::string strategyName;

        // Version this strategy represents
        GraphTypeVersion version;
    };

    /**
     * @class DocumentVersionManager
     * @brief Central registry for document creation strategies and version routing
     * 
     * **Architecture Pattern: Strategy + Registry + Factory + Fallback**
     * 
     * Responsibilities:
     * 1. Register creation strategies for each (GraphType, Version) pair
     * 2. Route document creation to appropriate strategy
     * 3. Provide diagnostics and logging of routing decisions
     * 4. Support runtime version switching (for testing/migration)
     * 5. Enforce consistent routing across entire application
     * 6. **Auto-fallback to Legacy on errors** (robustness)
     * 7. **Comprehensive error logging** (debugging)
     * 
     * Key Properties:
     * - **Singleton Pattern**: Single source of truth for routing
     * - **No Magic Booleans**: Explicit, discoverable, traceable
     * - **Extensible**: Add new graph types or versions without code recompilation
     * - **Reversible**: Easy rollback or dual-path testing
     * - **Diagnostic-Friendly**: Full logging of routing decisions
     * - **Resilient**: Automatic fallback to Legacy on errors
     * - **Audit Trail**: Complete error tracking with stack context
     * 
     * Usage:
     * @code
     *     // Initialization (done once on engine startup)
     *     DocumentVersionManager& manager = DocumentVersionManager::Get();
     *     manager.RegisterStrategy("EntityPrefab", GraphTypeVersion::Legacy, 
     *                              legacyStrategy);
     *     manager.RegisterStrategy("EntityPrefab", GraphTypeVersion::Framework, 
     *                              frameworkStrategy);
     *     
     *     // Routing (transparent to caller)
     *     IGraphDocument* doc = manager.CreateNewDocument("EntityPrefab");
     *     // Automatically uses configured version (no flags to check)
     *     // On error: Auto-fallback to Legacy and logs full context
     * 
     *     // Runtime configuration
     *     manager.SetActiveVersion("EntityPrefab", GraphTypeVersion::Framework);
     *     // Subsequent creates use Framework version
     * 
     *     // Diagnostic: Check what happened
     *     if (manager.GetFallbackCount("EntityPrefab") > 0)
     *     {
     *         std::string log = manager.GetFallbackLog("EntityPrefab");
     *         // Output: "Framework strategy failed: 'Load failed: file not found'
     *         //          Fell back to Legacy successfully at 2024-01-15 14:23:45.123"
     *     }
     * @endcode
     * 
     * **Error Handling Strategy:**
     * 1. Try to create document with active version
     * 2. If strategy returns nullptr OR throws exception:
     *    - Log error with full context (type, version, caller, reason)
     *    - Automatically retry with Legacy version
     *    - Log fallback event
     * 3. If Legacy also fails:
     *    - Log critical error
     *    - Return nullptr
     *    - Caller responsible for handling nullptr
     * 
     * **Why This Approach?**
     * ✓ Scales to 5+ graph types without boolean explosion
     * ✓ Version changes centralized (one place to update)
     * ✓ Clear audit trail (logging shows routing decisions)
     * ✓ Easy to extend (add new version by registering strategy)
     * ✓ Testable (can inject strategies for testing)
     * ✓ Pro (used in industry for plugin systems, factory patterns)
     * ✓ Reversible (instant rollback by changing version)
     * ✓ Diagnostic-friendly (inspect what's registered, what's active)
     * ✓ **Resilient (auto-fallback prevents crashes)**
     * ✓ **Observable (full error logging for debugging)**
     */
    class DocumentVersionManager
    {
    public:
        // ========== SINGLETON ACCESS ==========

        /**
         * Get singleton instance
         * @return Global DocumentVersionManager instance
         */
        static DocumentVersionManager& Get();

        // Delete copy semantics (singleton)
        DocumentVersionManager(const DocumentVersionManager&) = delete;
        DocumentVersionManager& operator=(const DocumentVersionManager&) = delete;

        // ========== STRATEGY REGISTRATION ==========

        /**
         * Register a creation strategy for a graph type and version
         * @param graphType Graph type (e.g., "EntityPrefab", "VisualScript", "BehaviorTree")
         * @param version Version this strategy implements
         * @param strategy The creation strategy
         */
        void RegisterStrategy(const std::string& graphType, 
                             GraphTypeVersion version,
                             const DocumentCreationStrategy& strategy);

        /**
         * Register a strategy from config file (future: JSON/YAML based)
         * @param configPath Path to strategy configuration
         */
        void RegisterStrategiesFromConfig(const std::string& configPath);

        // ========== VERSION ROUTING ==========

        /**
         * Get currently active version for a graph type
         * @param graphType Graph type to query
         * @return Active version (defaults to Legacy if not configured)
         */
        GraphTypeVersion GetActiveVersion(const std::string& graphType) const;

        /**
         * Set active version for a graph type
         * @param graphType Graph type to configure
         * @param version Version to activate
         * @return true if version is registered, false otherwise
         */
        bool SetActiveVersion(const std::string& graphType, GraphTypeVersion version);

        /**
         * Check if a version is available for graph type
         * @param graphType Graph type to check
         * @param version Version to check
         * @return true if strategy registered, false otherwise
         */
        bool HasVersion(const std::string& graphType, GraphTypeVersion version) const;

        /**
         * Get all available versions for a graph type
         * @param graphType Graph type to query
         * @return Vector of available versions
         */
        std::vector<GraphTypeVersion> GetAvailableVersions(const std::string& graphType) const;

        // ========== DOCUMENT CREATION (Primary API) ==========

        /**
         * Create new empty document
         * Routes to active version's strategy automatically.
         * 
         * @param graphType Graph type (e.g., "EntityPrefab")
         * @return Newly created document, or nullptr if type not registered
         * 
         * Diagnostics:
         * - Logs routing decision: "Creating EntityPrefab via Framework strategy"
         * - Logs success/failure with version info
         * - Returns nullptr with error if strategy fails
         */
        IGraphDocument* CreateNewDocument(const std::string& graphType);

        /**
         * Load document from file
         * Routes to active version's strategy automatically.
         * Version is detected from graphType (not file contents).
         * 
         * @param graphType Graph type
         * @param filePath Path to .json file
         * @return Loaded document, or nullptr if loading failed
         * 
         * Diagnostics:
         * - Logs routing decision: "Loading EntityPrefab from Framework strategy"
         * - Logs file path and detected type
         * - Returns nullptr with error if file not found or parsing fails
         */
        IGraphDocument* LoadDocument(const std::string& graphType, 
                                     const std::string& filePath);

        /**
         * Create renderer for document
         * Routes to active version's strategy automatically.
         * 
         * @param graphType Graph type
         * @param document Document to create renderer for
         * @return Renderer, or nullptr if strategy doesn't support rendering
         */
        IGraphRenderer* CreateRenderer(const std::string& graphType,
                                       IGraphDocument* document);

        // ========== DIAGNOSTICS & LOGGING ==========

        /**
         * Get diagnostic info about registered strategies
         * @return Human-readable summary of strategies, versions, and routing
         */
        std::string GetDiagnosticInfo() const;

        /**
         * Log all routing information
         * Useful for debugging migration issues.
         */
        void LogRoutingInfo() const;

        /**
         * Get last routing decision (for diagnostics)
         * @return String describing last Create* call
         */
        std::string GetLastRoutingDecision() const;

        /**
         * Get all registered graph types
         * @return Vector of graph type names
         */
        std::vector<std::string> GetRegisteredTypes() const;

        // ========== ADVANCED: TESTING & MIGRATION ==========

        /**
         * Force version for specific graph type (overrides default)
         * Useful for A/B testing or controlled migration
         * @param graphType Graph type
         * @param version Version to force
         */
        void ForceVersion(const std::string& graphType, GraphTypeVersion version);

        /**
         * Get forced version (returns Legacy if not forced)
         * @param graphType Graph type
         * @return Forced version or current active
         */
        GraphTypeVersion GetForcedVersion(const std::string& graphType) const;

        /**
         * Clear all forced versions (revert to defaults)
         */
        void ClearForcedVersions();

        /**
         * Check if version is forced for graph type
         * @param graphType Graph type
         * @return true if version is currently forced
         */
        bool IsVersionForced(const std::string& graphType) const;

        // ========== ERROR HANDLING & FALLBACK ==========

        /**
         * Enable/disable automatic fallback to Legacy on errors
         * Default: true (fallback enabled)
         * @param graphType Graph type
         * @param enabled true to enable fallback, false to disable
         */
        void SetFallbackEnabled(const std::string& graphType, bool enabled);

        /**
         * Check if fallback is enabled for graph type
         * @param graphType Graph type
         * @return true if fallback enabled
         */
        bool IsFallbackEnabled(const std::string& graphType) const;

        /**
         * Get fallback count for graph type
         * Useful to detect if version is unstable
         * @param graphType Graph type
         * @return Number of times fallback was triggered
         */
        size_t GetFallbackCount(const std::string& graphType) const;

        /**
         * Get fallback error log for graph type
         * Contains all fallback events with timestamps and errors
         * @param graphType Graph type
         * @return Multi-line error log
         */
        std::string GetFallbackLog(const std::string& graphType) const;

        /**
         * Clear fallback history for graph type
         * @param graphType Graph type
         */
        void ClearFallbackHistory(const std::string& graphType);

        /**
         * Clear all fallback histories
         */
        void ClearAllFallbackHistories();

        /**
         * Get last error message
         * @return Description of last error encountered
         */
        std::string GetLastErrorMessage() const;

    private:
        // ========== PRIVATE MEMBERS ==========

        // Constructor (private for singleton)
        DocumentVersionManager();

        // Strategy registry: graphType -> version -> strategy
        std::map<std::string, std::map<GraphTypeVersion, DocumentCreationStrategy>> m_strategies;

        // Active versions: graphType -> active version
        std::map<std::string, GraphTypeVersion> m_activeVersions;

        // Forced versions (for testing): graphType -> forced version
        std::map<std::string, GraphTypeVersion> m_forcedVersions;

        // Last routing decision (for diagnostics)
        std::string m_lastRoutingDecision;

        // Last error message (for debugging)
        std::string m_lastErrorMessage;

        // Fallback enabled flags: graphType -> enabled (default: true)
        std::map<std::string, bool> m_fallbackEnabled;

        // Fallback counters: graphType -> count
        std::map<std::string, size_t> m_fallbackCount;

        // Fallback logs: graphType -> log entries (type | reason | timestamp)
        std::map<std::string, std::vector<std::string>> m_fallbackLogs;

        // ========== PRIVATE HELPERS ==========

        /**
         * Get effective version for routing
         * Respects forced versions and defaults to active version.
         */
        GraphTypeVersion GetEffectiveVersion(const std::string& graphType) const;

        /**
         * Get strategy for graph type and version
         * @param graphType Graph type
         * @param version Version
         * @return Strategy, or nullptr if not registered
         */
        const DocumentCreationStrategy* GetStrategy(const std::string& graphType,
                                                     GraphTypeVersion version) const;

        /**
         * Log routing decision
         */
        void LogRoutingDecision(const std::string& decision);

        /**
         * Log error with full context
         * @param graphType Type attempting to create
         * @param attemptedVersion Version that failed
         * @param errorReason Why it failed
         * @param callerContext Caller identification (file:line)
         */
        void LogError(const std::string& graphType,
                     GraphTypeVersion attemptedVersion,
                     const std::string& errorReason,
                     const std::string& callerContext);

        /**
         * Log fallback event
         * @param graphType Type falling back
         * @param fromVersion Version that failed
         * @param toVersion Version falling back to
         * @param errorReason Why fallback was triggered
         */
        void LogFallback(const std::string& graphType,
                        GraphTypeVersion fromVersion,
                        GraphTypeVersion toVersion,
                        const std::string& errorReason);

        /**
         * Format timestamp for logging
         * @return ISO 8601 format timestamp (YYYY-MM-DD HH:MM:SS.mmm)
         */
        static std::string GetCurrentTimestamp();

        /**
         * Try to create with strategy, handling errors
         * @param graphType Graph type
         * @param version Version to try
         * @param strategy Strategy to execute
         * @param operationType "create" | "load" for logging
         * @param operationParam Parameter for operation (e.g., filepath for load)
         * @return Created document or nullptr on failure
         */
        IGraphDocument* TryCreateWithStrategy(const std::string& graphType,
                                             GraphTypeVersion version,
                                             const DocumentCreationStrategy* strategy,
                                             const std::string& operationType,
                                             const std::string& operationParam = "");

        /**
         * Extract caller context (file:line) from stack
         * Used for error reporting
         * @return String like "TabManager.cpp:187"
         */
        static std::string GetCallerContext();
    };

} // namespace Olympe
