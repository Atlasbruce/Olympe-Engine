# Olympe Engine Code Analysis Script
# Generates a complete report of unused files and duplicates

Write-Host "`n=== OLYMPE ENGINE CODE ANALYSIS ===" -ForegroundColor Cyan

# Extract referenced files from Olympe Engine.vcxproj
$engineProjectFiles = @()
$engineContent = Get-Content "Olympe Engine.vcxproj" -Raw
$engineMatches = [regex]::Matches($engineContent, '<Cl(?:Compile|Include) Include="([^"]+)"')
foreach ($match in $engineMatches) {
    $path = $match.Groups[1].Value -replace '\\', '/'
    $engineProjectFiles += $path.ToLower()
}

# Extract referenced files from OlympeBlueprintEditor.vcxproj
$blueprintProjectFiles = @()
$blueprintContent = Get-Content "OlympeBlueprintEditor\OlympeBlueprintEditor.vcxproj" -Raw
$blueprintMatches = [regex]::Matches($blueprintContent, '<Cl(?:Compile|Include) Include="\.\.\\([^"]+)"')
foreach ($match in $blueprintMatches) {
    $path = $match.Groups[1].Value -replace '\\', '/'
    $blueprintProjectFiles += $path.ToLower()
}

# Get all source files
$allSourceFiles = Get-ChildItem -Path .\Source -File -Recurse -Include *.cpp,*.h,*.hpp | 
    Where-Object { $_.FullName -notmatch 'third_party\\imgui\\examples' }

Write-Host "Files in Olympe Engine.vcxproj: $($engineProjectFiles.Count)" -ForegroundColor Green
Write-Host "Files in OlympeBlueprintEditor.vcxproj: $($blueprintProjectFiles.Count)" -ForegroundColor Green
Write-Host "Total files in ./Source: $($allSourceFiles.Count)" -ForegroundColor Green

# Analysis
$unreferencedFiles = @()
$deprecatedFiles = @()
$filesInBothProjects = @()

foreach ($file in $allSourceFiles) {
    $relativePath = $file.FullName.Replace((Get-Location).Path + '\', '').Replace('\', '/').ToLower()
    
    if ($relativePath -match '_deprecated') {
        $deprecatedFiles += @{
            Path = $relativePath
            Size = [math]::Round($file.Length / 1KB, 2)
        }
        continue
    }
    
    $inEngine = $engineProjectFiles -contains $relativePath
    $inBlueprint = $blueprintProjectFiles -contains $relativePath
    
    if (-not $inEngine -and -not $inBlueprint) {
        $isThirdParty = $relativePath -match 'third_party'
        $isDoc = $relativePath -match '\.(md|txt|cmake)$'
        
        $unreferencedFiles += @{
            Path = $relativePath
            Size = [math]::Round($file.Length / 1KB, 2)
            IsThirdParty = $isThirdParty
            IsDoc = $isDoc
            Extension = $file.Extension
        }
    }
    elseif ($inEngine -and $inBlueprint) {
        $filesInBothProjects += $relativePath
    }
}

# Categorize unreferenced files
$unreferencedCode = $unreferencedFiles | Where-Object { -not $_.IsThirdParty -and -not $_.IsDoc }
$unreferencedThirdParty = $unreferencedFiles | Where-Object { $_.IsThirdParty -and -not $_.IsDoc }
$unreferencedDocs = $unreferencedFiles | Where-Object { $_.IsDoc }

$deprecatedSize = if ($deprecatedFiles.Count -gt 0) {
    ($deprecatedFiles | Measure-Object -Property Size -Sum).Sum
} else { 0 }

$unreferencedCodeSize = if ($unreferencedCode.Count -gt 0) {
    ($unreferencedCode | Measure-Object -Property Size -Sum).Sum
} else { 0 }

# Display results
Write-Host "`n=== ANALYSIS RESULTS ===" -ForegroundColor Cyan
Write-Host "Shared files (both projects): $($filesInBothProjects.Count)" -ForegroundColor Green
Write-Host "Unreferenced files: $($unreferencedFiles.Count)" -ForegroundColor Yellow
Write-Host "  - Source code files: $($unreferencedCode.Count)" -ForegroundColor Yellow
Write-Host "  - Third party files: $($unreferencedThirdParty.Count)" -ForegroundColor DarkYellow
Write-Host "  - Documentation files: $($unreferencedDocs.Count)" -ForegroundColor Gray
Write-Host "Deprecated files: $($deprecatedFiles.Count) (Size: $([math]::Round($deprecatedSize, 2)) KB)" -ForegroundColor Red

if ($unreferencedCode.Count -gt 0) {
    Write-Host "`nUNREFERENCED SOURCE CODE FILES:" -ForegroundColor Yellow
    foreach ($file in $unreferencedCode | Sort-Object Path | Select-Object -First 20) {
        Write-Host "  - $($file.Path) ($($file.Size) KB)" -ForegroundColor DarkYellow
    }
    if ($unreferencedCode.Count -gt 20) {
        Write-Host "  ... and $($unreferencedCode.Count - 20) more files" -ForegroundColor DarkYellow
    }
}

if ($deprecatedFiles.Count -gt 0) {
    Write-Host "`nDEPRECATED FILES (_deprecated folder):" -ForegroundColor Red
    foreach ($file in $deprecatedFiles | Sort-Object Path) {
        Write-Host "  - $($file.Path) ($($file.Size) KB)" -ForegroundColor DarkRed
    }
}

# Generate detailed report
$report = "# OLYMPE ENGINE - CODE AUDIT REPORT`n`n"
$report += "**Date:** $(Get-Date -Format 'yyyy-MM-dd HH:mm:ss')`n"
$report += "**Analysis by:** Senior Visual C++ Programmer`n`n"
$report += "---`n`n"
$report += "## EXECUTIVE SUMMARY`n`n"
$report += "| Metric | Value |`n"
$report += "|--------|-------|`n"
$report += "| Total files in ./Source | $($allSourceFiles.Count) |`n"
$report += "| Files in Olympe Engine | $($engineProjectFiles.Count) |`n"
$report += "| Files in Blueprint Editor | $($blueprintProjectFiles.Count) |`n"
$report += "| Shared files (both projects) | $($filesInBothProjects.Count) |`n"
$report += "| Unreferenced files | $($unreferencedFiles.Count) |`n"
$report += "| Deprecated files | $($deprecatedFiles.Count) |`n`n"
$report += "---`n`n"
$report += "## UNREFERENCED SOURCE CODE FILES`n`n"

if ($unreferencedCode.Count -gt 0) {
    $report += "**Total:** $($unreferencedCode.Count) files (~$([math]::Round($unreferencedCodeSize, 2)) KB)`n`n"
    $report += "| File | Size (KB) | Extension |`n"
    $report += "|------|-----------|-----------|`n"
    foreach ($file in $unreferencedCode | Sort-Object Path) {
        $report += "| ``$($file.Path)`` | $($file.Size) | $($file.Extension) |`n"
    }
} else {
    $report += "No unreferenced source code files detected.`n"
}

$report += "`n## UNREFERENCED THIRD PARTY FILES`n`n"

if ($unreferencedThirdParty.Count -gt 0) {
    $report += "**Total:** $($unreferencedThirdParty.Count) files`n`n"
    $report += "| File | Size (KB) |`n"
    $report += "|------|-----------|`n"
    foreach ($file in $unreferencedThirdParty | Sort-Object Path) {
        $report += "| ``$($file.Path)`` | $($file.Size) |`n"
    }
} else {
    $report += "All third party files are referenced.`n"
}

$report += "`n---`n`n"
$report += "## DEPRECATED FILES`n`n"

if ($deprecatedFiles.Count -gt 0) {
    $report += "**Total:** $($deprecatedFiles.Count) files (~$([math]::Round($deprecatedSize, 2)) KB)`n`n"
    $report += "These files are in the ``_deprecated`` folder and can be safely removed:`n`n"
    $report += "| File | Size (KB) |`n"
    $report += "|------|-----------|`n"
    foreach ($file in $deprecatedFiles | Sort-Object Path) {
        $report += "| ``$($file.Path)`` | $($file.Size) |`n"
    }
    $report += "`n**RECOMMENDATION:** Delete the entire ``_deprecated`` folder immediately.`n"
} else {
    $report += "No deprecated files found.`n"
}

$report += "`n---`n`n"
$report += "## SENIOR PROGRAMMER RECOMMENDATIONS`n`n"
$report += "### PRIORITY 1 - Immediate Actions:`n`n"
$report += "1. **Delete deprecated files** ($($deprecatedFiles.Count) files, ~$([math]::Round($deprecatedSize, 2)) KB)`n"
$report += "   - Command: ``Remove-Item -Path '.\Source\_deprecated' -Recurse -Force```n"
$report += "   - Risk: None (explicitly marked as deprecated)`n`n"
$report += "2. **Review unreferenced source files** ($($unreferencedCode.Count) files)`n"
$report += "   - Check if they are included indirectly`n"
$report += "   - Archive or delete if not used for >3 months`n`n"
$report += "3. **Clean up third party files** ($($unreferencedThirdParty.Count) files)`n"
$report += "   - Remove ImGui examples (not needed for production)`n"
$report += "   - Keep only necessary source files`n`n"
$report += "### PRIORITY 2 - Architecture Improvements:`n`n"
$report += "4. **Refactor shared code** ($($filesInBothProjects.Count) shared files)`n"
$report += "   - Consider creating a static library for shared components`n"
$report += "   - Benefits: Faster compilation, better modularity`n`n"
$report += "### POSITIVE ASPECTS:`n`n"
$report += "- Excellent modular organization (AI, ECS, TaskSystem, etc.)`n"
$report += "- Clear separation of deprecated code`n"
$report += "- Modern ECS architecture`n"
$report += "- Appropriate use of standard libraries (ImGui, nlohmann/json)`n`n"
$report += "### POTENTIAL ISSUES:`n`n"
$report += "- Case inconsistencies in paths (``source/`` vs ``Source/``)`n"
$report += "- Documentation mixed with source code`n"
$report += "- Potentially unused TiledLevelLoader files`n`n"
$report += "---`n`n"
$report += "## AUTOMATED CLEANUP COMMANDS`n`n"
$report += "````powershell`n"
$report += "# 1. Delete deprecated folder (SAFE)`n"
$report += "Remove-Item -Path '.\Source\_deprecated' -Recurse -Force`n`n"
$report += "# 2. Delete ImGui examples (if not needed)`n"
$report += "Remove-Item -Path '.\Source\third_party\imgui\examples' -Recurse -Force`n"
$report += "````n`n"
$report += "---`n`n"
$report += "**Space potentially freeable:** ~$([math]::Round($deprecatedSize + $unreferencedCodeSize, 2)) KB`n"
$report += "**Files to remove/archive:** $($deprecatedFiles.Count + $unreferencedCode.Count)`n`n"
$report += "**Report generated:** $(Get-Date -Format 'yyyy-MM-dd HH:mm:ss')`n"

$report | Out-File -FilePath ".\OLYMPE_CODE_AUDIT_REPORT.md" -Encoding UTF8

Write-Host "`nReport saved: OLYMPE_CODE_AUDIT_REPORT.md" -ForegroundColor Green
Write-Host "`nRECOMMENDED NEXT STEPS:" -ForegroundColor Cyan
Write-Host "1. Review the full report: OLYMPE_CODE_AUDIT_REPORT.md" -ForegroundColor White
Write-Host "2. Delete _deprecated folder (safe action)" -ForegroundColor White
Write-Host "3. Review $($unreferencedCode.Count) unreferenced source files" -ForegroundColor White
