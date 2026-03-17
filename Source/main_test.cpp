// Source/main_test.cpp — Olympe Engine Version Stamp Usage Example
//
// This file demonstrates correct integration of the VersionStamp system.
// It is NOT the production main.cpp; it is an educational reference showing:
//   1. How to include version.h
//   2. How to call PrintVersionInfo() as the first startup operation
//   3. How to access individual version fields programmatically
//
// Expected output (Debug build):
//   [VersionStamp] Olympe Engine v0.24.0
//   [VersionStamp] Build timestamp : 2026-03-17 10:07:51 UTC
//   [VersionStamp] Git commit SHA  : 0000000000000000000000000000000000000000
//   [VersionStamp] Git branch      : master
//   [VersionStamp] PR reference    : #000
//   [VersionStamp] Phase           : 24-0-VersionStamp
//   [VersionStamp] Build config    : Debug
//   [VersionStamp] Full version    : OLYMPE_VERSION:0.24.0|BUILD:2026-03-17 10:07:51 UTC|...
//
// Binary verification (after build):
//   strings OlympeEngine.exe | grep OLYMPE_VERSION
//   -> OLYMPE_VERSION:0.24.0|BUILD:...|SHA:...|BRANCH:master|PR:#000|PHASE:...|CONFIG:Debug

#include "System/version.h"

int main()
{
    // IMPORTANT: Version info must be logged first, before any other startup logs.
    Olympe::VersionStamp::PrintVersionInfo();

    // Access individual version fields programmatically when needed.
    // Example: conditional behaviour based on build configuration.
    const char* config = Olympe::VersionStamp::BUILD_CONFIG;
    SYSTEM_LOG << "[main_test] Running in " << config << " mode" << std::endl;

    // Example: log the current phase for diagnostic purposes.
    SYSTEM_LOG << "[main_test] Active phase : " << Olympe::VersionStamp::PHASE << std::endl;

    // Example: access the full searchable version string.
    SYSTEM_LOG << "[main_test] Version tag  : " << Olympe::VersionStamp::FULL_VERSION_STRING << std::endl;

    return 0;
}
