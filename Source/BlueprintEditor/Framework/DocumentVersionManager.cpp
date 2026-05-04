#include "DocumentVersionManager.h"
#include "IGraphDocument.h"
#include "IGraphRenderer.h"
#include "../../system/system_utils.h"
#include <sstream>
#include <chrono>
#include <iomanip>

namespace Olympe
{
    // ========== SINGLETON IMPLEMENTATION ==========

    DocumentVersionManager& DocumentVersionManager::Get()
    {
        static DocumentVersionManager instance;
        return instance;
    }

    DocumentVersionManager::DocumentVersionManager()
        : m_lastRoutingDecision("")
        , m_lastErrorMessage("")
    {
        // Initialize default fallback state for all types (enabled by default)
        // This will be populated dynamically as types are registered
    }

    // ========== STRATEGY REGISTRATION ==========

    void DocumentVersionManager::RegisterStrategy(const std::string& graphType,
                                                  GraphTypeVersion version,
                                                  const DocumentCreationStrategy& strategy)
    {
        // Register strategy
        m_strategies[graphType][version] = strategy;

        // Initialize fallback enabled (default: true)
        if (m_fallbackEnabled.find(graphType) == m_fallbackEnabled.end())
        {
            m_fallbackEnabled[graphType] = true;
        }

        // Set default active version if not already set
        if (m_activeVersions.find(graphType) == m_activeVersions.end())
        {
            // Default to Framework if available, else Legacy
            if (HasVersion(graphType, GraphTypeVersion::Framework))
            {
                m_activeVersions[graphType] = GraphTypeVersion::Framework;
            }
            else
            {
                m_activeVersions[graphType] = GraphTypeVersion::Legacy;
            }
        }

        std::ostringstream oss;
        oss << "[DocumentVersionManager] Registered strategy: "
            << strategy.strategyName
            << " (GraphType=" << graphType
            << ", Version=" << static_cast<int>(version) << ")";
        SYSTEM_LOG << oss.str() << std::endl;
    }

    void DocumentVersionManager::RegisterStrategiesFromConfig(const std::string& configPath)
    {
        // TODO: Implement JSON/YAML based strategy registration
        SYSTEM_LOG << "[DocumentVersionManager] RegisterStrategiesFromConfig not yet implemented: " << configPath << std::endl;
    }

    // ========== VERSION ROUTING ==========

    GraphTypeVersion DocumentVersionManager::GetActiveVersion(const std::string& graphType) const
    {
        auto it = m_activeVersions.find(graphType);
        if (it != m_activeVersions.end())
        {
            return it->second;
        }
        return GraphTypeVersion::Legacy; // Default fallback
    }

    bool DocumentVersionManager::SetActiveVersion(const std::string& graphType, GraphTypeVersion version)
    {
        if (!HasVersion(graphType, version))
        {
            std::ostringstream oss;
            oss << "[DocumentVersionManager] ERROR: Cannot set active version for "
                << graphType << ": version " << static_cast<int>(version) << " not registered";
            m_lastErrorMessage = oss.str();
            SYSTEM_LOG << m_lastErrorMessage << std::endl;
            return false;
        }

        m_activeVersions[graphType] = version;

        std::ostringstream oss;
        oss << "[DocumentVersionManager] Active version changed: "
            << graphType << " -> v" << static_cast<int>(version);
        m_lastRoutingDecision = oss.str();
        SYSTEM_LOG << m_lastRoutingDecision << std::endl;

        return true;
    }

    bool DocumentVersionManager::HasVersion(const std::string& graphType, GraphTypeVersion version) const
    {
        auto typeIt = m_strategies.find(graphType);
        if (typeIt == m_strategies.end())
            return false;

        return typeIt->second.find(version) != typeIt->second.end();
    }

    std::vector<GraphTypeVersion> DocumentVersionManager::GetAvailableVersions(const std::string& graphType) const
    {
        std::vector<GraphTypeVersion> versions;
        auto it = m_strategies.find(graphType);
        if (it != m_strategies.end())
        {
            for (const auto& versionPair : it->second)
            {
                versions.push_back(versionPair.first);
            }
        }
        return versions;
    }

    // ========== DOCUMENT CREATION (Primary API) ==========

    IGraphDocument* DocumentVersionManager::CreateNewDocument(const std::string& graphType)
    {
        GraphTypeVersion effectiveVersion = GetEffectiveVersion(graphType);

        std::ostringstream routingLog;
        routingLog << "[DocumentVersionManager] CreateNewDocument: "
                   << graphType << " (v" << static_cast<int>(effectiveVersion) << ")";

        if (IsVersionForced(graphType))
        {
            routingLog << " [FORCED]";
        }

        m_lastRoutingDecision = routingLog.str();
        SYSTEM_LOG << m_lastRoutingDecision << std::endl;

        const DocumentCreationStrategy* strategy = GetStrategy(graphType, effectiveVersion);
        if (!strategy || !strategy->createNewDocument)
        {
            std::ostringstream oss;
            oss << "Strategy not found or createNewDocument not implemented";
            LogError(graphType, effectiveVersion, oss.str(), GetCallerContext());

            // Try fallback to Legacy
            if (effectiveVersion != GraphTypeVersion::Legacy && IsFallbackEnabled(graphType))
            {
                LogFallback(graphType, effectiveVersion, GraphTypeVersion::Legacy, oss.str());
                const DocumentCreationStrategy* legacyStrategy = GetStrategy(graphType, GraphTypeVersion::Legacy);
                if (legacyStrategy && legacyStrategy->createNewDocument)
                {
                    IGraphDocument* doc = TryCreateWithStrategy(graphType, GraphTypeVersion::Legacy, legacyStrategy, "create");
                    if (doc)
                    {
                        m_fallbackCount[graphType]++;
                        return doc;
                    }
                }
            }

            return nullptr;
        }

        return TryCreateWithStrategy(graphType, effectiveVersion, strategy, "create");
    }

    IGraphDocument* DocumentVersionManager::LoadDocument(const std::string& graphType,
                                                         const std::string& filePath)
    {
        GraphTypeVersion effectiveVersion = GetEffectiveVersion(graphType);

        std::ostringstream routingLog;
        routingLog << "[DocumentVersionManager] LoadDocument: "
                   << graphType << " from '" << filePath << "' (v" << static_cast<int>(effectiveVersion) << ")";
        m_lastRoutingDecision = routingLog.str();
        SYSTEM_LOG << m_lastRoutingDecision << std::endl;

        const DocumentCreationStrategy* strategy = GetStrategy(graphType, effectiveVersion);
        if (!strategy || !strategy->loadDocumentFromFile)
        {
            std::ostringstream oss;
            oss << "Strategy not found or loadDocumentFromFile not implemented";
            LogError(graphType, effectiveVersion, oss.str(), GetCallerContext());

            // Try fallback to Legacy
            if (effectiveVersion != GraphTypeVersion::Legacy && IsFallbackEnabled(graphType))
            {
                LogFallback(graphType, effectiveVersion, GraphTypeVersion::Legacy, oss.str());
                const DocumentCreationStrategy* legacyStrategy = GetStrategy(graphType, GraphTypeVersion::Legacy);
                if (legacyStrategy && legacyStrategy->loadDocumentFromFile)
                {
                    IGraphDocument* doc = TryCreateWithStrategy(graphType, GraphTypeVersion::Legacy, legacyStrategy, "load", filePath);
                    if (doc)
                    {
                        m_fallbackCount[graphType]++;
                        return doc;
                    }
                }
            }

            return nullptr;
        }

        return TryCreateWithStrategy(graphType, effectiveVersion, strategy, "load", filePath);
    }

    IGraphRenderer* DocumentVersionManager::CreateRenderer(const std::string& graphType,
                                                           IGraphDocument* document)
    {
        if (!document)
        {
            m_lastErrorMessage = "Cannot create renderer: document is nullptr";
            SYSTEM_LOG << "[DocumentVersionManager] ERROR: " << m_lastErrorMessage << std::endl;
            return nullptr;
        }

        GraphTypeVersion effectiveVersion = GetEffectiveVersion(graphType);

        std::ostringstream routingLog;
        routingLog << "[DocumentVersionManager] CreateRenderer: "
                   << graphType << " (v" << static_cast<int>(effectiveVersion) << ")";
        m_lastRoutingDecision = routingLog.str();
        SYSTEM_LOG << m_lastRoutingDecision << std::endl;

        const DocumentCreationStrategy* strategy = GetStrategy(graphType, effectiveVersion);
        if (!strategy)
        {
            m_lastErrorMessage = "Strategy not found";
            SYSTEM_LOG << "[DocumentVersionManager] ERROR: " << m_lastErrorMessage << std::endl;
            return nullptr;
        }

        // Note: createRenderer can be nullptr (some versions may not provide renderer)
        if (!strategy->createRenderer)
        {
            SYSTEM_LOG << "[DocumentVersionManager] INFO: Strategy has no renderer factory" << std::endl;
            return nullptr;
        }

        try
        {
            IGraphRenderer* renderer = strategy->createRenderer(document);
            if (!renderer)
            {
                std::ostringstream oss;
                oss << "[DocumentVersionManager] ERROR: createRenderer returned nullptr for "
                    << graphType << " v" << static_cast<int>(effectiveVersion);
                m_lastErrorMessage = oss.str();
                SYSTEM_LOG << m_lastErrorMessage << std::endl;
            }
            return renderer;
        }
        catch (const std::exception& e)
        {
            std::ostringstream oss;
            oss << "[DocumentVersionManager] EXCEPTION in createRenderer: " << e.what();
            m_lastErrorMessage = oss.str();
            SYSTEM_LOG << m_lastErrorMessage << std::endl;
            return nullptr;
        }
    }

    // ========== DIAGNOSTICS & LOGGING ==========

    std::string DocumentVersionManager::GetDiagnosticInfo() const
    {
        std::ostringstream oss;
        oss << "\n========== DocumentVersionManager Diagnostic Info ==========\n";

        // Registered types
        oss << "\n[Registered Graph Types]\n";
        auto types = GetRegisteredTypes();
        if (types.empty())
        {
            oss << "  (no types registered)\n";
        }
        else
        {
            for (const auto& type : types)
            {
                oss << "  - " << type << "\n";

                // Versions for this type
                auto versions = GetAvailableVersions(type);
                for (const auto& version : versions)
                {
                    oss << "    v" << static_cast<int>(version);

                    // Mark active version
                    if (GetActiveVersion(type) == version)
                    {
                        oss << " [ACTIVE]";
                    }

                    // Mark forced version
                    if (IsVersionForced(type))
                    {
                        if (GetForcedVersion(type) == version)
                        {
                            oss << " [FORCED]";
                        }
                    }

                    // Show fallback count
                    auto fallbackIt = m_fallbackCount.find(type);
                    if (fallbackIt != m_fallbackCount.end() && fallbackIt->second > 0)
                    {
                        oss << " [FALLBACKS: " << fallbackIt->second << "]";
                    }

                    oss << "\n";
                }
            }
        }

        // Last routing decision
        oss << "\n[Last Routing Decision]\n";
        if (m_lastRoutingDecision.empty())
        {
            oss << "  (no routing decisions yet)\n";
        }
        else
        {
            oss << "  " << m_lastRoutingDecision << "\n";
        }

        // Last error
        oss << "\n[Last Error]\n";
        if (m_lastErrorMessage.empty())
        {
            oss << "  (no errors)\n";
        }
        else
        {
            oss << "  " << m_lastErrorMessage << "\n";
        }

        oss << "\n==========================================================\n";

        return oss.str();
    }

    void DocumentVersionManager::LogRoutingInfo() const
    {
        SYSTEM_LOG << GetDiagnosticInfo() << std::endl;
    }

    std::string DocumentVersionManager::GetLastRoutingDecision() const
    {
        return m_lastRoutingDecision;
    }

    std::vector<std::string> DocumentVersionManager::GetRegisteredTypes() const
    {
        std::vector<std::string> types;
        for (const auto& typePair : m_strategies)
        {
            types.push_back(typePair.first);
        }
        return types;
    }

    // ========== ADVANCED: TESTING & MIGRATION ==========

    void DocumentVersionManager::ForceVersion(const std::string& graphType, GraphTypeVersion version)
    {
        if (!HasVersion(graphType, version))
        {
            std::ostringstream oss;
            oss << "[DocumentVersionManager] ERROR: Cannot force version for "
                << graphType << ": version " << static_cast<int>(version) << " not registered";
            m_lastErrorMessage = oss.str();
            SYSTEM_LOG << m_lastErrorMessage << std::endl;
            return;
        }

        m_forcedVersions[graphType] = version;

        std::ostringstream oss;
        oss << "[DocumentVersionManager] FORCING version: "
            << graphType << " -> v" << static_cast<int>(version)
            << " (for testing/debugging)";
        SYSTEM_LOG << oss.str() << std::endl;
    }

    GraphTypeVersion DocumentVersionManager::GetForcedVersion(const std::string& graphType) const
    {
        auto it = m_forcedVersions.find(graphType);
        if (it != m_forcedVersions.end())
        {
            return it->second;
        }
        return GetActiveVersion(graphType);
    }

    void DocumentVersionManager::ClearForcedVersions()
    {
        m_forcedVersions.clear();
        SYSTEM_LOG << "[DocumentVersionManager] Cleared all forced versions" << std::endl;
    }

    bool DocumentVersionManager::IsVersionForced(const std::string& graphType) const
    {
        return m_forcedVersions.find(graphType) != m_forcedVersions.end();
    }

    // ========== ERROR HANDLING & FALLBACK ==========

    void DocumentVersionManager::SetFallbackEnabled(const std::string& graphType, bool enabled)
    {
        m_fallbackEnabled[graphType] = enabled;

        std::ostringstream oss;
        oss << "[DocumentVersionManager] Fallback for " << graphType << " is "
            << (enabled ? "ENABLED" : "DISABLED");
        SYSTEM_LOG << oss.str() << std::endl;
    }

    bool DocumentVersionManager::IsFallbackEnabled(const std::string& graphType) const
    {
        auto it = m_fallbackEnabled.find(graphType);
        if (it != m_fallbackEnabled.end())
        {
            return it->second;
        }
        return true; // Default: enabled
    }

    size_t DocumentVersionManager::GetFallbackCount(const std::string& graphType) const
    {
        auto it = m_fallbackCount.find(graphType);
        if (it != m_fallbackCount.end())
        {
            return it->second;
        }
        return 0;
    }

    std::string DocumentVersionManager::GetFallbackLog(const std::string& graphType) const
    {
        std::ostringstream oss;
        oss << "\n========== Fallback Log for '" << graphType << "' ==========\n";

        auto countIt = m_fallbackCount.find(graphType);
        if (countIt != m_fallbackCount.end() && countIt->second > 0)
        {
            oss << "Total Fallbacks: " << countIt->second << "\n\n";
        }
        else
        {
            oss << "No fallbacks recorded.\n";
            oss << "\n=========================================================\n";
            return oss.str();
        }

        auto logIt = m_fallbackLogs.find(graphType);
        if (logIt != m_fallbackLogs.end())
        {
            for (size_t i = 0; i < logIt->second.size(); ++i)
            {
                oss << "[" << (i + 1) << "] " << logIt->second[i] << "\n";
            }
        }

        oss << "\n=========================================================\n";

        return oss.str();
    }

    void DocumentVersionManager::ClearFallbackHistory(const std::string& graphType)
    {
        m_fallbackCount[graphType] = 0;
        m_fallbackLogs[graphType].clear();

        std::ostringstream oss;
        oss << "[DocumentVersionManager] Cleared fallback history for " << graphType;
        SYSTEM_LOG << oss.str() << std::endl;
    }

    void DocumentVersionManager::ClearAllFallbackHistories()
    {
        m_fallbackCount.clear();
        m_fallbackLogs.clear();
        SYSTEM_LOG << "[DocumentVersionManager] Cleared all fallback histories" << std::endl;
    }

    std::string DocumentVersionManager::GetLastErrorMessage() const
    {
        return m_lastErrorMessage;
    }

    // ========== PRIVATE HELPERS ==========

    GraphTypeVersion DocumentVersionManager::GetEffectiveVersion(const std::string& graphType) const
    {
        // Forced version takes precedence
        if (IsVersionForced(graphType))
        {
            return GetForcedVersion(graphType);
        }

        // Otherwise use active version
        return GetActiveVersion(graphType);
    }

    const DocumentCreationStrategy* DocumentVersionManager::GetStrategy(const std::string& graphType,
                                                                         GraphTypeVersion version) const
    {
        auto typeIt = m_strategies.find(graphType);
        if (typeIt == m_strategies.end())
            return nullptr;

        auto versionIt = typeIt->second.find(version);
        if (versionIt == typeIt->second.end())
            return nullptr;

        return &versionIt->second;
    }

    void DocumentVersionManager::LogRoutingDecision(const std::string& decision)
    {
        m_lastRoutingDecision = decision;
        SYSTEM_LOG << decision << std::endl;
    }

    void DocumentVersionManager::LogError(const std::string& graphType,
                                          GraphTypeVersion attemptedVersion,
                                          const std::string& errorReason,
                                          const std::string& callerContext)
    {
        std::ostringstream oss;
        oss << "[DocumentVersionManager] ERROR: "
            << graphType << " v" << static_cast<int>(attemptedVersion)
            << " failed: " << errorReason
            << " (caller: " << callerContext << ")";

        m_lastErrorMessage = oss.str();
        SYSTEM_LOG << m_lastErrorMessage << std::endl;
    }

    void DocumentVersionManager::LogFallback(const std::string& graphType,
                                             GraphTypeVersion fromVersion,
                                             GraphTypeVersion toVersion,
                                             const std::string& errorReason)
    {
        std::ostringstream oss;
        oss << "[DocumentVersionManager] FALLBACK: "
            << graphType << " v" << static_cast<int>(fromVersion)
            << " -> v" << static_cast<int>(toVersion)
            << " (reason: " << errorReason << ")"
            << " [" << GetCurrentTimestamp() << "]";

        std::string logEntry = oss.str();
        SYSTEM_LOG << logEntry << std::endl;

        // Record in fallback log
        m_fallbackLogs[graphType].push_back(logEntry);

        // Increment fallback counter
        if (m_fallbackCount.find(graphType) == m_fallbackCount.end())
        {
            m_fallbackCount[graphType] = 1;
        }
        else
        {
            m_fallbackCount[graphType]++;
        }
    }

    std::string DocumentVersionManager::GetCurrentTimestamp()
    {
        auto now = std::chrono::system_clock::now();
        auto time = std::chrono::system_clock::to_time_t(now);
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;

        std::ostringstream oss;
        #pragma warning(push)
        #pragma warning(disable:4996)
        oss << std::put_time(std::localtime(&time), "%Y-%m-%d %H:%M:%S");
        #pragma warning(pop)
        oss << "." << std::setfill('0') << std::setw(3) << ms.count();

        return oss.str();
    }

    IGraphDocument* DocumentVersionManager::TryCreateWithStrategy(const std::string& graphType,
                                                                   GraphTypeVersion version,
                                                                   const DocumentCreationStrategy* strategy,
                                                                   const std::string& operationType,
                                                                   const std::string& operationParam)
    {
        if (!strategy)
        {
            LogError(graphType, version, "Strategy is nullptr", GetCallerContext());
            return nullptr;
        }

        try
        {
            IGraphDocument* result = nullptr;

            if (operationType == "create")
            {
                if (!strategy->createNewDocument)
                {
                    std::ostringstream oss;
                    oss << strategy->strategyName << " has no createNewDocument function";
                    LogError(graphType, version, oss.str(), GetCallerContext());
                    return nullptr;
                }
                result = strategy->createNewDocument();
            }
            else if (operationType == "load")
            {
                if (!strategy->loadDocumentFromFile)
                {
                    std::ostringstream oss;
                    oss << strategy->strategyName << " has no loadDocumentFromFile function";
                    LogError(graphType, version, oss.str(), GetCallerContext());
                    return nullptr;
                }
                result = strategy->loadDocumentFromFile(operationParam);
            }
            else
            {
                LogError(graphType, version, "Unknown operation type: " + operationType, GetCallerContext());
                return nullptr;
            }

            if (result)
            {
                std::ostringstream oss;
                oss << "[DocumentVersionManager] SUCCESS: " << operationType << " completed for "
                    << graphType << " v" << static_cast<int>(version)
                    << " using strategy '" << strategy->strategyName << "'";
                SYSTEM_LOG << oss.str() << std::endl;
            }
            else
            {
                std::ostringstream oss;
                oss << strategy->strategyName << " returned nullptr";
                LogError(graphType, version, oss.str(), GetCallerContext());
            }

            return result;
        }
        catch (const std::exception& e)
        {
            std::ostringstream oss;
            oss << "Exception: " << e.what();
            LogError(graphType, version, oss.str(), GetCallerContext());
            return nullptr;
        }
        catch (...)
        {
            LogError(graphType, version, "Unknown exception", GetCallerContext());
            return nullptr;
        }
    }

    std::string DocumentVersionManager::GetCallerContext()
    {
        // Simplified implementation - just returns a placeholder
        // In production, this would use platform-specific stack walking
        return "unknown:0";

        // TODO: Platform-specific implementation
        // Windows: Use StackWalk64 with dbghelp.dll
        // Linux: Use backtrace() and backtrace_symbols()
    }

} // namespace Olympe
