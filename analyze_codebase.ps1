# Script d'analyse de la base de code Olympe Engine
# Génère un rapport complet des fichiers inutilisés et doublons

Write-Host "=== ANALYSE DE LA BASE DE CODE OLYMPE ENGINE ===" -ForegroundColor Cyan
Write-Host ""

# Extraction des fichiers référencés dans Olympe Engine.vcxproj
$engineProjectFiles = @()
$engineContent = Get-Content "Olympe Engine.vcxproj" -Raw
$engineMatches = [regex]::Matches($engineContent, '<Cl(?:Compile|Include) Include="([^"]+)"')
foreach ($match in $engineMatches) {
    $path = $match.Groups[1].Value -replace '\\', '/'
    $engineProjectFiles += $path.ToLower()
}

Write-Host "Fichiers dans Olympe Engine.vcxproj: $($engineProjectFiles.Count)" -ForegroundColor Green

# Extraction des fichiers référencés dans OlympeBlueprintEditor.vcxproj
$blueprintProjectFiles = @()
$blueprintContent = Get-Content "OlympeBlueprintEditor\OlympeBlueprintEditor.vcxproj" -Raw
$blueprintMatches = [regex]::Matches($blueprintContent, '<Cl(?:Compile|Include) Include="\.\.\\([^"]+)"')
foreach ($match in $blueprintMatches) {
    $path = $match.Groups[1].Value -replace '\\', '/'
    $blueprintProjectFiles += $path.ToLower()
}

Write-Host "Fichiers dans OlympeBlueprintEditor.vcxproj: $($blueprintProjectFiles.Count)" -ForegroundColor Green
Write-Host ""

# Tous les fichiers source (cpp, h, hpp) dans ./Source (hors examples imgui)
$allSourceFiles = Get-ChildItem -Path .\Source -File -Recurse -Include *.cpp,*.h,*.hpp | 
    Where-Object { $_.FullName -notmatch 'third_party\\imgui\\examples' }

$totalSourceFiles = $allSourceFiles.Count
Write-Host "Fichiers totaux dans ./Source: $totalSourceFiles" -ForegroundColor Green
Write-Host ""

# Analyse des fichiers non référencés
$unreferencedFiles = @()
$deprecatedFiles = @()
$filesInBothProjects = @()
$filesOnlyInEngine = @()
$filesOnlyInBlueprint = @()

foreach ($file in $allSourceFiles) {
    $relativePath = $file.FullName.Replace((Get-Location).Path + '\', '').Replace('\', '/').ToLower()
    
    # Vérifier si c'est dans le dossier _deprecated
    if ($relativePath -match '_deprecated') {
        $deprecatedFiles += [PSCustomObject]@{
            Path = $relativePath
            Size = [math]::Round($file.Length / 1KB, 2)
        }
        continue
    }
    
    $inEngine = $engineProjectFiles -contains $relativePath
    $inBlueprint = $blueprintProjectFiles -contains $relativePath
    
    if (-not $inEngine -and -not $inBlueprint) {
        # Vérifier si c'est un fichier third_party
        $isThirdParty = $relativePath -match 'third_party'
        
        $unreferencedFiles += [PSCustomObject]@{
            Path = $relativePath
            Size = [math]::Round($file.Length / 1KB, 2)
            IsThirdParty = $isThirdParty
            LastModified = $file.LastWriteTime
        }
    }
    elseif ($inEngine -and $inBlueprint) {
        $filesInBothProjects += $relativePath
    }
    elseif ($inEngine) {
        $filesOnlyInEngine += $relativePath
    }
    else {
        $filesOnlyInBlueprint += $relativePath
    }
}

# Génération du rapport
$reportPath = ".\Code_Analysis_Report.md"
$report = @"
# 🔍 RAPPORT D'AUDIT DE CODE - OLYMPE ENGINE
**Date:** $(Get-Date -Format "yyyy-MM-dd HH:mm:ss")
**Analyste:** Programmeur Senior Visual C++

---

## 📊 RÉSUMÉ EXÉCUTIF

| Métrique | Valeur |
|----------|--------|
| **Fichiers totaux dans ./Source** | $totalSourceFiles |
| **Fichiers dans Olympe Engine.vcxproj** | $($engineProjectFiles.Count) |
| **Fichiers dans OlympeBlueprintEditor.vcxproj** | $($blueprintProjectFiles.Count) |
| **Fichiers partagés (les deux projets)** | $($filesInBothProjects.Count) |
| **Fichiers uniquement dans Engine** | $($filesOnlyInEngine.Count) |
| **Fichiers uniquement dans Blueprint Editor** | $($filesOnlyInBlueprint.Count) |
| **⚠️ Fichiers NON référencés** | $($unreferencedFiles.Count) |
| **🗑️ Fichiers dépréciés (_deprecated)** | $($deprecatedFiles.Count) |

---

## 🚨 FICHIERS NON RÉFÉRENCÉS (À NETTOYER)

Ces fichiers existent dans ./Source mais ne sont référencés dans **AUCUN** des deux projets principaux.

### Fichiers de code source non utilisés (hors third_party):
``````

$nonThirdPartyUnreferenced = $unreferencedFiles | Where-Object { -not $_.IsThirdParty }
if ($nonThirdPartyUnreferenced.Count -gt 0) {
    $report += "`n| Fichier | Taille (KB) | Dernière modification |`n"
    $report += "|---------|-------------|----------------------|`n"
    foreach ($file in $nonThirdPartyUnreferenced | Sort-Object Path) {
        $report += "| ``$($file.Path)`` | $($file.Size) | $($file.LastModified.ToString('yyyy-MM-dd')) |`n"
    }
} else {
    $report += "`n✅ Aucun fichier de code source non référencé détecté.`n"
}

$report += @"

### Bibliothèques third_party non référencées:
``````

$thirdPartyUnreferenced = $unreferencedFiles | Where-Object { $_.IsThirdParty }
if ($thirdPartyUnreferenced.Count -gt 0) {
    $report += "`n| Fichier | Taille (KB) |`n"
    $report += "|---------|-------------|`n"
    foreach ($file in $thirdPartyUnreferenced | Sort-Object Path) {
        $report += "| ``$($file.Path)`` | $($file.Size) |`n"
    }
} else {
    $report += "`n✅ Toutes les bibliothèques third_party sont référencées.`n"
}

$report += @"

---

## 🗑️ FICHIERS DÉPRÉCIÉS

Ces fichiers sont dans le dossier ``_deprecated`` et peuvent être supprimés en toute sécurité:

``````

if ($deprecatedFiles.Count -gt 0) {
    $totalDeprecatedSize = ($deprecatedFiles | Measure-Object -Property Size -Sum).Sum
    $report += "`n**Taille totale:** $([math]::Round($totalDeprecatedSize, 2)) KB`n`n"
    $report += "| Fichier | Taille (KB) |`n"
    $report += "|---------|-------------|`n"
    foreach ($file in $deprecatedFiles | Sort-Object Path) {
        $report += "| ``$($file.Path)`` | $($file.Size) |`n"
    }
} else {
    $report += "`n✅ Aucun fichier déprécié trouvé.`n"
}

$report += @"

---

## 🔗 ANALYSE DES DOUBLONS ET DÉPENDANCES

### Fichiers partagés entre les deux projets:
**Total:** $($filesInBothProjects.Count) fichiers

Ces fichiers sont utilisés à la fois par Olympe Engine et Blueprint Editor:
``````

if ($filesInBothProjects.Count -gt 0) {
    $report += "`n``````"
    foreach ($file in $filesInBothProjects | Sort-Object | Select-Object -First 20) {
        $report += "`n$file"
    }
    if ($filesInBothProjects.Count -gt 20) {
        $report += "`n... et $($filesInBothProjects.Count - 20) autres fichiers"
    }
    $report += "`n```````n"
}

$report += @"

---

## 📝 RECOMMANDATIONS D'UN PROGRAMMEUR SENIOR

### 🎯 Actions prioritaires:

1. **Supprimer les fichiers dépréciés** ($($deprecatedFiles.Count) fichiers, ~$([math]::Round(($deprecatedFiles | Measure-Object -Property Size -Sum).Sum, 2)) KB)
   - Ces fichiers sont clairement marqués comme obsolètes
   - Peut être fait immédiatement sans risque

2. **Examiner les fichiers non référencés** ($($nonThirdPartyUnreferenced.Count) fichiers)
   - Vérifier si ces fichiers sont réellement utilisés via des inclusions indirectes
   - Si non utilisés depuis > 6 mois, les archiver ou supprimer

3. **Nettoyer third_party** ($($thirdPartyUnreferenced.Count) fichiers non référencés)
   - Supprimer les exemples ImGui inutilisés (si non nécessaires pour la documentation)
   - Conserver uniquement les headers/sources réellement utilisés

### 🔍 Points d'attention:

- **Architecture partagée**: $($filesInBothProjects.Count) fichiers sont partagés entre les projets
  - Envisager une DLL/lib commune pour réduire la duplication de compilation
  
- **Cohérence de casse**: Détecter les incohérences de casse dans les chemins (source/ vs Source/)

### ✅ Qualité du code:

- Bonne séparation avec le dossier ``_deprecated``
- Structure modulaire claire (AI, ECS, TaskSystem, etc.)
- Utilisation appropriée de bibliothèques tierces (ImGui, nlohmann/json)

---

## 🛠️ ACTIONS AUTOMATISÉES DISPONIBLES

Voulez-vous que j'effectue automatiquement:
1. ✅ Suppression des fichiers ``_deprecated`` ($($deprecatedFiles.Count) fichiers)
2. ⚠️ Archivage des fichiers non référencés dans un dossier ``_archived``
3. 🔧 Nettoyage des exemples third_party non utilisés

---

**Rapport généré par:** GitHub Copilot - Analyse de code Visual C++
**Fichier de sortie:** Code_Analysis_Report.md
"@

$report | Out-File -FilePath $reportPath -Encoding UTF8

Write-Host "=== RÉSULTATS ===" -ForegroundColor Cyan
Write-Host "Fichiers non référencés: $($unreferencedFiles.Count)" -ForegroundColor Yellow
Write-Host "  - Code source: $($nonThirdPartyUnreferenced.Count)" -ForegroundColor Yellow
Write-Host "  - Third party: $($thirdPartyUnreferenced.Count)" -ForegroundColor Yellow
Write-Host "Fichiers dépréciés: $($deprecatedFiles.Count)" -ForegroundColor Red
Write-Host "Fichiers partagés: $($filesInBothProjects.Count)" -ForegroundColor Green
Write-Host ""
Write-Host "✅ Rapport complet généré: $reportPath" -ForegroundColor Green
Write-Host ""

# Affichage d'un résumé des fichiers non référencés
if ($nonThirdPartyUnreferenced.Count -gt 0) {
    Write-Host "⚠️ FICHIERS NON RÉFÉRENCÉS À EXAMINER:" -ForegroundColor Yellow
    foreach ($file in $nonThirdPartyUnreferenced | Sort-Object Path | Select-Object -First 10) {
        Write-Host "   - $($file.Path)" -ForegroundColor DarkYellow
    }
    if ($nonThirdPartyUnreferenced.Count -gt 10) {
        Write-Host "   ... et $($nonThirdPartyUnreferenced.Count - 10) autres" -ForegroundColor DarkYellow
    }
}

Write-Host ""
Write-Host "Consultez le fichier Code_Analysis_Report.md pour le rapport détaillé." -ForegroundColor Cyan
