# Fix Olympe Engine.sln - Remove editor projects to avoid SDL_AppEvent conflicts
# This script removes OlympeBlueprintEditor and OlympeTilemapEditor from Olympe Engine.sln

Write-Host "=============================================" -ForegroundColor Cyan
Write-Host "  Fix Olympe Engine.sln" -ForegroundColor Cyan
Write-Host "=============================================" -ForegroundColor Cyan
Write-Host ""

$slnPath = "Olympe Engine.sln"
$backupPath = "Olympe Engine.sln.backup"

# Check if solution file exists
if (-not (Test-Path $slnPath)) {
    Write-Host "ERROR: $slnPath not found!" -ForegroundColor Red
    exit 1
}

# Create backup
Write-Host "Creating backup: $backupPath" -ForegroundColor Yellow
Copy-Item $slnPath $backupPath -Force

# Read solution file
Write-Host "Reading solution file..." -ForegroundColor Yellow
$content = Get-Content $slnPath -Raw

# Remove OlympeBlueprintEditor project reference
Write-Host "Removing OlympeBlueprintEditor project..." -ForegroundColor Yellow
$content = $content -replace 'Project\("\{8BC9CEB8-8B4A-11D0-8D11-00A0C91BC942\}"\) = "OlympeBlueprintEditor".*?EndProject\s*', ''

# Remove OlympeTilemapEditor project reference
Write-Host "Removing OlympeTilemapEditor project..." -ForegroundColor Yellow
$content = $content -replace 'Project\("\{8BC9CEB8-8B4A-11D0-8D11-00A0C91BC942\}"\) = "OlympeTilemapEditor".*?EndProject\s*', ''

# Remove BlueprintEditor build configurations
Write-Host "Removing OlympeBlueprintEditor build configs..." -ForegroundColor Yellow
$content = $content -replace '\s*\{9B4C9E2A-0000-0000-0000-000000000001\}\..*', ''

# Remove TilemapEditor build configurations
Write-Host "Removing OlympeTilemapEditor build configs..." -ForegroundColor Yellow
$content = $content -replace '\s*\{8A3D9F7B-1234-5678-9ABC-DEF012345678\}\..*', ''

# Write back to file
Write-Host "Writing cleaned solution file..." -ForegroundColor Yellow
Set-Content $slnPath $content -NoNewline

Write-Host ""
Write-Host "=============================================" -ForegroundColor Green
Write-Host "  Solution file fixed successfully!" -ForegroundColor Green
Write-Host "=============================================" -ForegroundColor Green
Write-Host ""
Write-Host "Changes made:" -ForegroundColor Cyan
Write-Host "  - Removed OlympeBlueprintEditor project" -ForegroundColor White
Write-Host "  - Removed OlympeTilemapEditor project" -ForegroundColor White
Write-Host "  - Removed all related build configurations" -ForegroundColor White
Write-Host ""
Write-Host "Backup saved to: $backupPath" -ForegroundColor Yellow
Write-Host ""
Write-Host "Next steps:" -ForegroundColor Cyan
Write-Host "  1. Close Visual Studio if open" -ForegroundColor White
Write-Host "  2. Reopen 'Olympe Engine.sln' (will only build the game)" -ForegroundColor White
Write-Host "  3. To build editors, use their dedicated solutions:" -ForegroundColor White
Write-Host "     - OlympeBlueprintEditor.sln" -ForegroundColor White
Write-Host "     - OlympeTilemapEditor.sln" -ForegroundColor White
Write-Host ""
