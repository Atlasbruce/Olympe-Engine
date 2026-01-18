# Script PowerShell pour ajouter TiledLevelLoader au projet Olympe Engine
# Usage : .\add_tiled_to_project.ps1

Write-Host "============================================" -ForegroundColor Cyan
Write-Host "  Ajout de TiledLevelLoader au projet" -ForegroundColor Cyan
Write-Host "============================================" -ForegroundColor Cyan
Write-Host ""

# Vérifier que le fichier existe
$vcxproj = "Olympe Engine.vcxproj"
if (-not (Test-Path $vcxproj)) {
    Write-Host "? Erreur : Fichier '$vcxproj' non trouvé" -ForegroundColor Red
    Write-Host "   Assurez-vous d'exécuter ce script depuis le dossier racine du projet" -ForegroundColor Yellow
    exit 1
}

# Créer un backup avec timestamp
$timestamp = Get-Date -Format 'yyyyMMdd_HHmmss'
$backup = "$vcxproj.backup_$timestamp"
Write-Host "?? Création backup : $backup" -ForegroundColor Yellow
Copy-Item $vcxproj $backup

# Lire le contenu du fichier
Write-Host "?? Lecture du fichier projet..." -ForegroundColor Yellow
$content = Get-Content $vcxproj -Encoding UTF8 -Raw

# Vérifier si les fichiers sont déjà présents
if ($content -match 'TiledLevelLoader\.cpp') {
    Write-Host "? Les fichiers TiledLevelLoader sont déjà dans le projet !" -ForegroundColor Green
    Write-Host "   Aucune modification nécessaire." -ForegroundColor Green
    exit 0
}

# Rechercher la ligne avec imnodes.cpp
$pattern = '(    <ClCompile Include="Source\\third_party\\imnodes\\imnodes\.cpp" />)'
$newLines = @'
$1
    <ClCompile Include="Source\TiledLevelLoader\src\TiledLevelLoader.cpp" />
    <ClCompile Include="Source\TiledLevelLoader\src\TiledToOlympe.cpp" />
'@

# Effectuer le remplacement
Write-Host "??  Ajout des fichiers au projet..." -ForegroundColor Yellow
$newContent = $content -replace $pattern, $newLines

# Vérifier que la modification a été effectuée
if ($newContent -eq $content) {
    Write-Host "? Erreur : Impossible de trouver le point d'insertion" -ForegroundColor Red
    Write-Host "   Le fichier vcxproj a peut-être une structure différente" -ForegroundColor Yellow
    Write-Host "   Veuillez ajouter les fichiers manuellement via Visual Studio" -ForegroundColor Yellow
    exit 1
}

# Sauvegarder
Write-Host "?? Sauvegarde des modifications..." -ForegroundColor Yellow
[System.IO.File]::WriteAllText((Resolve-Path $vcxproj), $newContent, [System.Text.Encoding]::UTF8)

# Vérification finale
Write-Host ""
Write-Host "============================================" -ForegroundColor Green
Write-Host "? SUCCÈS : Fichiers ajoutés au projet !" -ForegroundColor Green
Write-Host "============================================" -ForegroundColor Green
Write-Host ""
Write-Host "?? Fichiers ajoutés :" -ForegroundColor Cyan
Write-Host "   • Source\TiledLevelLoader\src\TiledLevelLoader.cpp" -ForegroundColor White
Write-Host "   • Source\TiledLevelLoader\src\TiledToOlympe.cpp" -ForegroundColor White
Write-Host ""
Write-Host "?? Prochaines étapes :" -ForegroundColor Cyan
Write-Host "   1. Ouvrir le projet dans Visual Studio" -ForegroundColor White
Write-Host "   2. Rebuild Solution (Ctrl+Shift+B)" -ForegroundColor White
Write-Host "   3. Vérifier qu'il n'y a plus d'erreurs LNK2019" -ForegroundColor White
Write-Host ""
Write-Host "?? Backup créé : $backup" -ForegroundColor Yellow
Write-Host "   (En cas de problème, restaurez ce fichier)" -ForegroundColor Yellow
Write-Host ""

# Proposer d'ouvrir VS
$response = Read-Host "Voulez-vous ouvrir le projet dans Visual Studio maintenant ? (O/N)"
if ($response -match '^[Oo]') {
    Write-Host "?? Ouverture de Visual Studio..." -ForegroundColor Cyan
    Start-Process "Olympe Engine.sln"
}

Write-Host "? Terminé !" -ForegroundColor Green
