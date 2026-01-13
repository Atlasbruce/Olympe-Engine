# Add OlympeBlueprintEditor and OlympeTilemapEditor to Olympe Engine.sln
# WITHOUT enabling their build (to avoid SDL conflicts)

Write-Host "Adding editor projects to Olympe Engine.sln (without building them)..." -ForegroundColor Cyan

$slnPath = "Olympe Engine.sln"

# Read solution
$content = Get-Content $slnPath -Raw

# Add BlueprintEditor project reference (after Olympe Engine project)
$blueprintProject = @'
Project("{8BC9CEB8-8B4A-11D0-8D11-00A0C91BC942}") = "OlympeBlueprintEditor", "OlympeBlueprintEditor\OlympeBlueprintEditor.vcxproj", "{9B4C9E2A-0000-0000-0000-000000000001}"
EndProject
'@

# Add TilemapEditor project reference
$tilemapProject = @'
Project("{8BC9CEB8-8B4A-11D0-8D11-00A0C91BC942}") = "OlympeTilemapEditor", "OlympeTilemapEditor\OlympeTilemapEditor.vcxproj", "{8A3D9F7B-1234-5678-9ABC-DEF012345678}"
EndProject
'@

# Insert projects before "Global"
$content = $content -replace '(EndProject\s+)(Global)', "`$1$blueprintProject`r`n$tilemapProject`r`n`$2"

# Add build configurations (ActiveCfg ONLY, NO Build.0 to prevent actual building)
$blueprintConfigs = @'
		{9B4C9E2A-0000-0000-0000-000000000001}.Debug|x64.ActiveCfg = Debug|x64
		{9B4C9E2A-0000-0000-0000-000000000001}.Debug|x86.ActiveCfg = Debug|Win32
		{9B4C9E2A-0000-0000-0000-000000000001}.Release|x64.ActiveCfg = Release|x64
		{9B4C9E2A-0000-0000-0000-000000000001}.Release|x86.ActiveCfg = Release|Win32
'@

$tilemapConfigs = @'
		{8A3D9F7B-1234-5678-9ABC-DEF012345678}.Debug|x64.ActiveCfg = Debug|x64
		{8A3D9F7B-1234-5678-9ABC-DEF012345678}.Debug|x86.ActiveCfg = Debug|Win32
		{8A3D9F7B-1234-5678-9ABC-DEF012345678}.Release|x64.ActiveCfg = Release|x64
		{8A3D9F7B-1234-5678-9ABC-DEF012345678}.Release|x86.ActiveCfg = Release|Win32
'@

# Insert configs before EndGlobalSection
$content = $content -replace '(\t\tEndGlobalSection)', "$blueprintConfigs`r`n$tilemapConfigs`r`n`$1"

# Write back
Set-Content $slnPath $content -NoNewline

Write-Host "Done! Projects added to solution." -ForegroundColor Green
Write-Host ""
Write-Host "Configuration:" -ForegroundColor Yellow
Write-Host "  - OlympeBlueprintEditor: Visible in Solution Explorer, NOT built by default" -ForegroundColor White
Write-Host "  - OlympeTilemapEditor: Visible in Solution Explorer, NOT built by default" -ForegroundColor White
Write-Host "  - Olympe Engine: Will build normally" -ForegroundColor White
Write-Host ""
Write-Host "To build an editor:" -ForegroundColor Cyan
Write-Host "  1. Right-click the editor project in Solution Explorer" -ForegroundColor White
Write-Host "  2. Select 'Build'" -ForegroundColor White
Write-Host "  OR use their dedicated .sln files" -ForegroundColor White
