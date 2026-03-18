#pragma once

// Source/System/version.h — Olympe Engine Version Stamp System
// Embeds build metadata into the executable for complete traceability.
// Update all fields after each PR merge:
//   - BUILD_TIMESTAMP : UTC ISO 8601 (YYYY-MM-DD HH:MM:SS UTC)
//   - GIT_COMMIT_SHA  : full 40-character commit hash
//   - GIT_BRANCH      : branch name (usually "master")
//   - PR_NUMBER       : merged PR reference (e.g. "#388")
//   - PHASE           : XX-Y format with feature name
//   - MAJOR/MINOR/PATCH : semantic version
//   - BUILD_CONFIG    : "Debug" or "Release"
//   - FULL_VERSION_STRING : must remain one continuous line (binary searchable)
// C++14 compliant. Only dependency: system_utils.h for SYSTEM_LOG.

#include "../system/system_utils.h"

namespace Olympe
{
    struct VersionStamp
    {
        // Semantic versioning
        static constexpr const char* MAJOR = "0";
        static constexpr const char* MINOR = "24";
        static constexpr const char* PATCH = "0";

        // Build metadata — update after each PR merge
        static constexpr const char* BUILD_TIMESTAMP   = "2026-03-18 19:57:31 UTC";
        static constexpr const char* GIT_COMMIT_SHA    = "ee7638865351665048f6a1337ae6771214bc7f7b";
        static constexpr const char* GIT_BRANCH        = "copilot/implement-canvas-node-rendering";
        static constexpr const char* PR_NUMBER         = "#449";
        static constexpr const char* PHASE             = "24-Rendering-FINAL-CORRECTION";
        static constexpr const char* BUILD_CONFIG      = "Debug";

        // Single continuous line — binary searchable with: strings <exe> | grep OLYMPE_VERSION
        static constexpr const char* FULL_VERSION_STRING =
            "OLYMPE_VERSION:0.24.0|BUILD:2026-03-18 19:57:31 UTC"
            "|SHA:ee7638865351665048f6a1337ae6771214bc7f7b"
            "|BRANCH:copilot/implement-canvas-node-rendering"
            "|PR:#449|PHASE:24-Rendering-FINAL-CORRECTION|CONFIG:Debug";

        // Log all version fields via SYSTEM_LOG.
        // Call this as the very first operation in main() before any other logs.
        static void PrintVersionInfo()
        {
            SYSTEM_LOG << "[VersionStamp] Olympe Engine v"
                       << MAJOR << "." << MINOR << "." << PATCH << std::endl;
            SYSTEM_LOG << "[VersionStamp] Build timestamp : " << BUILD_TIMESTAMP  << std::endl;
            SYSTEM_LOG << "[VersionStamp] Git commit SHA  : " << GIT_COMMIT_SHA   << std::endl;
            SYSTEM_LOG << "[VersionStamp] Git branch      : " << GIT_BRANCH       << std::endl;
            SYSTEM_LOG << "[VersionStamp] PR reference    : " << PR_NUMBER        << std::endl;
            SYSTEM_LOG << "[VersionStamp] Phase           : " << PHASE            << std::endl;
            SYSTEM_LOG << "[VersionStamp] Build config    : " << BUILD_CONFIG     << std::endl;
            SYSTEM_LOG << "[VersionStamp] Full version    : " << FULL_VERSION_STRING << std::endl;
        }
    };

} // namespace Olympe
