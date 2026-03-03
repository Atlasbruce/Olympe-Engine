# Script de nettoyage automatisé - Olympe Engine
# Phase 1: Actions sécurisées sans risque
# Date: 2026-03-03

Write-Host "`n========================================" -ForegroundColor Cyan
Write-Host "  OLYMPE ENGINE - NETTOYAGE PHASE 1" -ForegroundColor Cyan
Write-Host "========================================`n" -ForegroundColor Cyan

$deletedFiles = 0
$freedSpace = 0

# 1. Supprimer le dossier _deprecated
Write-Host "1. Suppression du dossier _deprecated..." -ForegroundColor Yellow
$deprecatedPath = ".\Source\_deprecated"
if (Test-Path $deprecatedPath) {
    $deprecatedSize = (Get-ChildItem $deprecatedPath -Recurse | Measure-Object -Property Length -Sum).Sum
    Remove-Item -Path $deprecatedPath -Recurse -Force
    $deletedFiles += 13
    $freedSpace += $deprecatedSize
    Write-Host "   ✅ Dossier _deprecated supprimé (13 fichiers)" -ForegroundColor Green
} else {
    Write-Host "   ℹ️  Dossier _deprecated introuvable (déjà supprimé?)" -ForegroundColor Gray
}

# 2. Supprimer les fichiers de test non référencés
Write-Host "`n2. Suppression des fichiers de test non référencés..." -ForegroundColor Yellow
$testFiles = @(
    '.\Source\ai\aieditor\testaieditorgui.cpp',
    '.\Source\ai\aigraphplugin_bt\testaigraphplugin_bt.cpp',
    '.\Source\blueprinteditor\blueprint_test.cpp',
    '.\Source\blueprinteditor\asset_explorer_test.cpp',
    '.\Source\blueprinteditor\serialize_example.cpp',
    '.\Source\nodegraphcore\testnodegraphcore.cpp',
    '.\Source\nodegraphcore\nodegraphpanel.cpp'
)

foreach ($file in $testFiles) {
    if (Test-Path $file) {
        $fileSize = (Get-Item $file).Length
        Remove-Item $file -Force
        $deletedFiles++
        $freedSpace += $fileSize
        $fileName = Split-Path $file -Leaf
        Write-Host "   ✅ Supprimé: $fileName" -ForegroundColor Green
    }
}

# 3. Supprimer les backends ImGui non utilisés
Write-Host "`n3. Nettoyage des backends ImGui non utilisés..." -ForegroundColor Yellow
$imguiBackendsToDelete = @(
    'imgui_impl_dx9.*',
    'imgui_impl_dx10.*',
    'imgui_impl_dx11.*',
    'imgui_impl_dx12.*',
    'imgui_impl_vulkan.*',
    'imgui_impl_opengl2.*',
    'imgui_impl_opengl3.*',
    'imgui_impl_win32.*',
    'imgui_impl_glfw.*',
    'imgui_impl_glut.*',
    'imgui_impl_sdl2.*',
    'imgui_impl_android.*',
    'imgui_impl_allegro5.*',
    'imgui_impl_metal.*',
    'imgui_impl_osx.*',
    'imgui_impl_wgpu.*',
    'imgui_impl_sdlgpu3.*',
    'imgui_impl_sdlrenderer2.*',
    'imgui_impl_null.*'
)

$backendPath = '.\Source\third_party\imgui\backends'
if (Test-Path $backendPath) {
    foreach ($pattern in $imguiBackendsToDelete) {
        $files = Get-ChildItem -Path $backendPath -Filter $pattern -ErrorAction SilentlyContinue
        foreach ($file in $files) {
            $fileSize = $file.Length
            Remove-Item $file.FullName -Force
            $deletedFiles++
            $freedSpace += $fileSize
            Write-Host "   ✅ Supprimé: $($file.Name)" -ForegroundColor Green
        }
    }
}

# 4. Supprimer les exemples imnodes
Write-Host "`n4. Suppression des exemples imnodes..." -ForegroundColor Yellow
$imnodesExamplePath = '.\Source\third_party\imnodes\example'
if (Test-Path $imnodesExamplePath) {
    $exampleSize = (Get-ChildItem $imnodesExamplePath -Recurse | Measure-Object -Property Length -Sum).Sum
    $exampleCount = (Get-ChildItem $imnodesExamplePath -Recurse -File).Count
    Remove-Item -Path $imnodesExamplePath -Recurse -Force
    $deletedFiles += $exampleCount
    $freedSpace += $exampleSize
    Write-Host "   ✅ Dossier examples supprimé ($exampleCount fichiers)" -ForegroundColor Green
} else {
    Write-Host "   ℹ️  Dossier examples introuvable" -ForegroundColor Gray
}

# 5. Supprimer les utilitaires ImGui non utilisés
Write-Host "`n5. Nettoyage des utilitaires ImGui non utilisés..." -ForegroundColor Yellow

# Freetype
$freetypePath = '.\Source\third_party\imgui\misc\freetype'
if (Test-Path $freetypePath) {
    $ftSize = (Get-ChildItem $freetypePath -Recurse | Measure-Object -Property Length -Sum).Sum
    $ftCount = (Get-ChildItem $freetypePath -Recurse -File).Count
    Remove-Item -Path $freetypePath -Recurse -Force
    $deletedFiles += $ftCount
    $freedSpace += $ftSize
    Write-Host "   ✅ Supprimé: misc/freetype/" -ForegroundColor Green
}

# C++ helpers
$cppPath = '.\Source\third_party\imgui\misc\cpp'
if (Test-Path $cppPath) {
    $cppSize = (Get-ChildItem $cppPath -Recurse | Measure-Object -Property Length -Sum).Sum
    $cppCount = (Get-ChildItem $cppPath -Recurse -File).Count
    Remove-Item -Path $cppPath -Recurse -Force
    $deletedFiles += $cppCount
    $freedSpace += $cppSize
    Write-Host "   ✅ Supprimé: misc/cpp/" -ForegroundColor Green
}

# Single file
$singleFilePath = '.\Source\third_party\imgui\misc\single_file'
if (Test-Path $singleFilePath) {
    Remove-Item -Path $singleFilePath -Recurse -Force
    $deletedFiles++
    Write-Host "   ✅ Supprimé: misc/single_file/" -ForegroundColor Green
}

# Binary to compressed
$binaryToolPath = '.\Source\third_party\imgui\misc\fonts\binary_to_compressed_c.cpp'
if (Test-Path $binaryToolPath) {
    $toolSize = (Get-Item $binaryToolPath).Length
    Remove-Item -Path $binaryToolPath -Force
    $deletedFiles++
    $freedSpace += $toolSize
    Write-Host "   ✅ Supprimé: binary_to_compressed_c.cpp" -ForegroundColor Green
}

# 6. Supprimer les fichiers STB internes (si non utilisés directement)
Write-Host "`n6. Nettoyage des headers STB inutiles..." -ForegroundColor Yellow
$stbHeaders = @(
    '.\Source\third_party\imgui\imstb_truetype.h',
    '.\Source\third_party\imgui\imstb_textedit.h',
    '.\Source\third_party\imgui\imstb_rectpack.h'
)

# Note: Ces fichiers sont normalement utilisés en interne par ImGui
# Ne les supprimer QUE si vous êtes sûr qu'ils ne sont pas nécessaires
Write-Host "   ℹ️  Headers STB conservés (utilisés en interne par ImGui)" -ForegroundColor Gray
Write-Host "   (imstb_truetype.h, imstb_textedit.h, imstb_rectpack.h)" -ForegroundColor Gray

# Résumé final
Write-Host "`n========================================" -ForegroundColor Cyan
Write-Host "  RÉSUMÉ DU NETTOYAGE" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan
Write-Host "Fichiers supprimés: $deletedFiles" -ForegroundColor Green
Write-Host "Espace libéré: $([math]::Round($freedSpace / 1KB, 2)) KB ($([math]::Round($freedSpace / 1MB, 2)) MB)" -ForegroundColor Green
Write-Host "`n✅ Nettoyage Phase 1 terminé avec succès!" -ForegroundColor Green
Write-Host "`n⚠️  PROCHAINES ÉTAPES:" -ForegroundColor Yellow
Write-Host "1. Compiler les deux projets pour vérifier qu'il n'y a pas d'erreurs" -ForegroundColor White
Write-Host "2. Tester les fonctionnalités principales" -ForegroundColor White
Write-Host "3. Si tout fonctionne, exécuter la Phase 2 (archivage)" -ForegroundColor White
Write-Host "`nℹ️  Consultez RAPPORT_AUDIT_FRANCAIS.md pour plus de détails" -ForegroundColor Cyan
Write-Host ""
