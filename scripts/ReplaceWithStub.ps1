# Replace BTDebugAdapter.cpp with stub

$content = @"
/**
 * @file BTDebugAdapter.cpp
 * @brief BehaviorTree Debug Adapter (PHASE 2 - STUB)
 * STATUS: Excluded from build (placeholder for future implementation)
 */

// PHASE 2: All implementation disabled
// Uncomment when unified architecture is ready
"@

Set-Content "Source\NodeGraphShared\BTDebugAdapter.cpp" -Value $content
Write-Host "BTDebugAdapter.cpp replaced with stub"
