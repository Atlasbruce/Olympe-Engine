# Script de nettoyage - Phase 2: Archivage des fichiers potentiellement obsolètes
# Olympe Engine
# Date: 2026-03-03

Write-Host "`n========================================" -ForegroundColor Cyan
Write-Host "  OLYMPE ENGINE - NETTOYAGE PHASE 2" -ForegroundColor Cyan
Write-Host "  (Archivage des fichiers suspects)" -ForegroundColor Cyan
Write-Host "========================================`n" -ForegroundColor Cyan

Write-Host "⚠️  ATTENTION: Cette phase archive des fichiers qui PEUVENT être utilisés." -ForegroundColor Yellow
Write-Host "   Testez votre application pendant 1-2 semaines après cette phase." -ForegroundColor Yellow
Write-Host "   Si tout fonctionne, vous pourrez supprimer l'archive." -ForegroundColor Yellow
Write-Host ""

$confirm = Read-Host "Voulez-vous continuer? (O/N)"
if ($confirm -ne 'O' -and $confirm -ne 'o') {
    Write-Host "❌ Opération annulée." -ForegroundColor Red
    exit
}

$archiveFolder = ".\Source\_archived_$(Get-Date -Format 'yyyy-MM-dd')"
Write-Host "`nCréation du dossier d'archivage: $archiveFolder" -ForegroundColor Yellow

# Créer le dossier d'archivage
New-Item -Path $archiveFolder -ItemType Directory -Force | Out-Null

$archivedFiles = 0
$archivedSize = 0

# Liste des fichiers à archiver
$toArchive = @(
    '.\Source\objectfactory.h',
    '.\Source\olympeeffectsystem.cpp',
    '.\Source\sprite.cpp',
    '.\Source\sprite.h',
    '.\Source\singleton.h',
    '.\Source\rendercontext.cpp',
    '.\Source\rendercontext.h',
    '.\Source\system\cameramanager.cpp',
    '.\Source\system\cameramanager.h',
    '.\Source\blueprinteditor\imnodes_stub.h',
    '.\Source\nodegraphcore\editorcontext.cpp',
    '.\Source\nodegraphcore\editorcontext.h'
)

Write-Host "`nArchivage en cours..." -ForegroundColor Yellow

foreach ($file in $toArchive) {
    if (Test-Path $file) {
        $fileItem = Get-Item $file
        $fileSize = $fileItem.Length
        
        # Calculer le chemin relatif depuis .\Source\
        $relativePath = $file -replace [regex]::Escape('.\Source\'), ''
        $archivePath = Join-Path $archiveFolder $relativePath
        $archiveDir = Split-Path $archivePath -Parent
        
        # Créer le sous-dossier si nécessaire
        if (-not (Test-Path $archiveDir)) {
            New-Item -Path $archiveDir -ItemType Directory -Force | Out-Null
        }
        
        # Déplacer le fichier
        Move-Item $file $archivePath -Force
        $archivedFiles++
        $archivedSize += $fileSize
        
        $fileName = Split-Path $file -Leaf
        Write-Host "   📦 Archivé: $fileName → $relativePath" -ForegroundColor Green
    } else {
        $fileName = Split-Path $file -Leaf
        Write-Host "   ℹ️  Fichier introuvable: $fileName (déjà supprimé?)" -ForegroundColor Gray
    }
}

# Créer un fichier README dans l'archive
$readmeContent = @"
# Fichiers archivés - Olympe Engine
**Date:** $(Get-Date -Format 'yyyy-MM-dd HH:mm:ss')

## Raison de l'archivage
Ces fichiers n'étaient référencés dans aucun des projets .vcxproj principaux.
Ils ont été archivés par précaution pour éviter toute suppression accidentelle.

## Fichiers archivés ($archivedFiles fichiers)

"@

foreach ($file in $toArchive) {
    $fileName = Split-Path $file -Leaf
    $readmeContent += "- $fileName`n"
}

$readmeContent += @"

## Instructions

### 1. Période de test (1-2 semaines)
Utilisez votre application normalement pendant 1-2 semaines.
Si tout fonctionne correctement, ces fichiers n'étaient pas nécessaires.

### 2. En cas de problème
Si vous rencontrez des erreurs de compilation ou d'exécution :
1. Identifiez le fichier manquant dans le message d'erreur
2. Restaurez-le depuis ce dossier vers .\Source\
3. Ajoutez-le aux fichiers .vcxproj appropriés

### 3. Suppression finale
Si après 2 semaines tout fonctionne parfaitement :
``````powershell
Remove-Item -Path '$archiveFolder' -Recurse -Force
``````

## Commandes de restauration

### Restaurer un fichier spécifique
``````powershell
# Exemple: restaurer objectfactory.h
Copy-Item '$archiveFolder\objectfactory.h' '.\Source\objectfactory.h'
``````

### Restaurer tous les fichiers
``````powershell
Copy-Item '$archiveFolder\*' '.\Source\' -Recurse -Force
``````

---
**Espace total archivé:** $([math]::Round($archivedSize / 1KB, 2)) KB
**Nombre de fichiers:** $archivedFiles
"@

$readmePath = Join-Path $archiveFolder "README.md"
$readmeContent | Out-File -FilePath $readmePath -Encoding UTF8

# Résumé final
Write-Host "`n========================================" -ForegroundColor Cyan
Write-Host "  RÉSUMÉ DE L'ARCHIVAGE" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan
Write-Host "Fichiers archivés: $archivedFiles" -ForegroundColor Green
Write-Host "Espace: $([math]::Round($archivedSize / 1KB, 2)) KB" -ForegroundColor Green
Write-Host "Emplacement: $archiveFolder" -ForegroundColor Cyan
Write-Host "`n✅ Archivage Phase 2 terminé!" -ForegroundColor Green

Write-Host "`n⚠️  IMPORTANT - PROCHAINES ÉTAPES:" -ForegroundColor Yellow
Write-Host "1. ✅ Compiler les deux projets (Olympe Engine + Blueprint Editor)" -ForegroundColor White
Write-Host "2. ✅ Tester toutes les fonctionnalités principales" -ForegroundColor White
Write-Host "3. ⏱️  Utiliser l'application pendant 1-2 semaines" -ForegroundColor White
Write-Host "4. 🗑️  Si tout fonctionne: supprimer le dossier $archiveFolder" -ForegroundColor White
Write-Host "5. 🔄 Si problème: consulter $readmePath" -ForegroundColor White

Write-Host "`nℹ️  En cas d'erreur de compilation, les fichiers peuvent être restaurés" -ForegroundColor Cyan
Write-Host "   depuis le dossier d'archivage." -ForegroundColor Cyan
Write-Host ""
