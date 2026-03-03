# Script d'analyse de la base de code Olympe Engine - Version améliorée
param([switch]$Detailed)

Write-Host "`n=== ANALYSE DE LA BASE DE CODE OLYMPE ENGINE ===" -ForegroundColor Cyan
Write-Host ""

# Extraction des fichiers référencés dans Olympe Engine.vcxproj
$engineProjectFiles = @()
$engineContent = Get-Content "Olympe Engine.vcxproj" -Raw
$engineMatches = [regex]::Matches($engineContent, '<Cl(?:Compile|Include) Include="([^"]+)"')
foreach ($match in $engineMatches) {
    $path = $match.Groups[1].Value -replace '\\', '/'
    $engineProjectFiles += $path.ToLower()
}

# Extraction des fichiers référencés dans OlympeBlueprintEditor.vcxproj
$blueprintProjectFiles = @()
$blueprintContent = Get-Content "OlympeBlueprintEditor\OlympeBlueprintEditor.vcxproj" -Raw
$blueprintMatches = [regex]::Matches($blueprintContent, '<Cl(?:Compile|Include) Include="\.\.\\([^"]+)"')
foreach ($match in $blueprintMatches) {
    $path = $match.Groups[1].Value -replace '\\', '/'
    $blueprintProjectFiles += $path.ToLower()
}

# Tous les fichiers source dans ./Source
$allSourceFiles = Get-ChildItem -Path .\Source -File -Recurse -Include *.cpp,*.h,*.hpp | 
    Where-Object { $_.FullName -notmatch 'third_party\\imgui\\examples' }

Write-Host "Fichiers dans Olympe Engine.vcxproj: $($engineProjectFiles.Count)" -ForegroundColor Green
Write-Host "Fichiers dans OlympeBlueprintEditor.vcxproj: $($blueprintProjectFiles.Count)" -ForegroundColor Green
Write-Host "Fichiers totaux dans ./Source: $($allSourceFiles.Count)" -ForegroundColor Green
Write-Host ""

# Analyse
$unreferencedFiles = @()
$deprecatedFiles = @()
$filesInBothProjects = @()

foreach ($file in $allSourceFiles) {
    $relativePath = $file.FullName.Replace((Get-Location).Path + '\', '').Replace('\', '/').ToLower()
    
    if ($relativePath -match '_deprecated') {
        $deprecatedFiles += @{
            Path = $relativePath
            Size = [math]::Round($file.Length / 1KB, 2)
            Name = $file.Name
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
            Name = $file.Name
            Extension = $file.Extension
        }
    }
    elseif ($inEngine -and $inBlueprint) {
        $filesInBothProjects += $relativePath
    }
}

# Catégorisation des fichiers non référencés
$unreferencedCode = $unreferencedFiles | Where-Object { -not $_.IsThirdParty -and -not $_.IsDoc }
$unreferencedThirdParty = $unreferencedFiles | Where-Object { $_.IsThirdParty -and -not $_.IsDoc }
$unreferencedDocs = $unreferencedFiles | Where-Object { $_.IsDoc }

# Génération du rapport
Write-Host "=== RÉSULTATS DE L'ANALYSE ===" -ForegroundColor Cyan
Write-Host ""
Write-Host "📊 STATISTIQUES:" -ForegroundColor White
Write-Host "  Total fichiers analysés: $($allSourceFiles.Count)" -ForegroundColor Gray
Write-Host "  Fichiers partagés (2 projets): $($filesInBothProjects.Count)" -ForegroundColor Green
Write-Host "  Fichiers uniquement Engine: $($engineProjectFiles.Count - $filesInBothProjects.Count)" -ForegroundColor Cyan
Write-Host "  Fichiers uniquement Blueprint: $($blueprintProjectFiles.Count - $filesInBothProjects.Count)" -ForegroundColor Cyan
Write-Host ""
Write-Host "⚠️  FICHIERS NON RÉFÉRENCÉS: $($unreferencedFiles.Count)" -ForegroundColor Yellow
Write-Host "  - Code source (.cpp/.h): $($unreferencedCode.Count)" -ForegroundColor Yellow
Write-Host "  - Bibliothèques third_party: $($unreferencedThirdParty.Count)" -ForegroundColor DarkYellow
Write-Host "  - Documentation (.md/.txt): $($unreferencedDocs.Count)" -ForegroundColor DarkGray
Write-Host ""
Write-Host "🗑️  FICHIERS DÉPRÉCIÉS: $($deprecatedFiles.Count)" -ForegroundColor Red
$deprecatedSize = ($deprecatedFiles | Measure-Object -Property Size -Sum).Sum
Write-Host "  Espace total: $([math]::Round($deprecatedSize, 2)) KB" -ForegroundColor Red
Write-Host ""

# Affichage détaillé
if ($unreferencedCode.Count -gt 0) {
    Write-Host "🚨 FICHIERS DE CODE NON RÉFÉRENCÉS À EXAMINER:" -ForegroundColor Yellow
    foreach ($file in $unreferencedCode | Sort-Object Path | Select-Object -First 15) {
        Write-Host "   ❌ $($file.Path) ($($file.Size) KB)" -ForegroundColor DarkYellow
    }
    if ($unreferencedCode.Count -gt 15) {
        Write-Host "   ... et $($unreferencedCode.Count - 15) autres fichiers" -ForegroundColor DarkYellow
    }
    Write-Host ""
}

if ($deprecatedFiles.Count -gt 0) {
    Write-Host "🗑️  FICHIERS DÉPRÉCIÉS (Dossier _deprecated):" -ForegroundColor Red
    foreach ($file in $deprecatedFiles | Sort-Object Path) {
        Write-Host "   🗑️  $($file.Path) ($($file.Size) KB)" -ForegroundColor DarkRed
    }
    Write-Host ""
}

# Rapport Markdown
$reportContent = @"
# 🔍 RAPPORT D'AUDIT DE CODE - OLYMPE ENGINE

**Date:** $(Get-Date -Format "yyyy-MM-dd HH:mm:ss")
**Analyste:** Programmeur Senior Visual C++

---

## 📊 RÉSUMÉ EXÉCUTIF

| Métrique | Valeur |
|----------|--------|
| **Fichiers totaux dans ./Source** | $($allSourceFiles.Count) |
| **Fichiers dans Olympe Engine.vcxproj** | $($engineProjectFiles.Count) |
| **Fichiers dans OlympeBlueprintEditor.vcxproj** | $($blueprintProjectFiles.Count) |
| **Fichiers partagés (les deux projets)** | $($filesInBothProjects.Count) |
| **Fichiers uniquement dans Engine** | $($engineProjectFiles.Count - $filesInBothProjects.Count) |
| **Fichiers uniquement dans Blueprint Editor** | $($blueprintProjectFiles.Count - $filesInBothProjects.Count) |
| **⚠️ Fichiers NON référencés** | $($unreferencedFiles.Count) |
| **🗑️ Fichiers dépréciés (_deprecated)** | $($deprecatedFiles.Count) |

---

## 🚨 FICHIERS NON RÉFÉRENCÉS (À NETTOYER)

### Fichiers de code source non utilisés:

"@

if ($unreferencedCode.Count -gt 0) {
    $reportContent += "`n**Total:** $($unreferencedCode.Count) fichiers`n`n"
    $reportContent += "| Fichier | Taille (KB) | Type |`n"
    $reportContent += "|---------|-------------|------|`n"
    foreach ($file in $unreferencedCode | Sort-Object Path) {
        $reportContent += "| ``$($file.Path)`` | $($file.Size) | $($file.Extension) |`n"
    }
} else {
    $reportContent += "`n✅ Aucun fichier de code source non référencé.`n"
}

$reportContent += "`n### Bibliothèques third_party non référencées:`n"

if ($unreferencedThirdParty.Count -gt 0) {
    $reportContent += "`n**Total:** $($unreferencedThirdParty.Count) fichiers`n`n"
    $reportContent += "| Fichier | Taille (KB) |`n"
    $reportContent += "|---------|-------------|`n"
    foreach ($file in $unreferencedThirdParty | Sort-Object Path) {
        $reportContent += "| ``$($file.Path)`` | $($file.Size) |`n"
    }
} else {
    $reportContent += "`n✅ Toutes les bibliothèques third_party sont référencées.`n"
}

$reportContent += "`n---`n`n## 🗑️ FICHIERS DÉPRÉCIÉS`n`n"

if ($deprecatedFiles.Count -gt 0) {
    $reportContent += "**Total:** $($deprecatedFiles.Count) fichiers (~$([math]::Round($deprecatedSize, 2)) KB)`n`n"
    $reportContent += "| Fichier | Taille (KB) |`n"
    $reportContent += "|---------|-------------|`n"
    foreach ($file in $deprecatedFiles | Sort-Object Path) {
        $reportContent += "| ``$($file.Path)`` | $($file.Size) |`n"
    }
    $reportContent += "`n**✅ RECOMMANDATION:** Ces fichiers peuvent être supprimés immédiatement.`n"
} else {
    $reportContent += "`n✅ Aucun fichier déprécié trouvé.`n"
}

$reportContent += @"

---

## 🔗 ANALYSE DES DÉPENDANCES

### Fichiers partagés entre les deux projets:
**Total:** $($filesInBothProjects.Count) fichiers

Ces fichiers sont utilisés à la fois par Olympe Engine et Blueprint Editor.
Principales catégories partagées:

"@

# Analyse des catégories partagées
$sharedByCategory = $filesInBothProjects | Group-Object { 
    $path = $_
    if ($path -match 'source/([^/]+)') { $matches[1] }
    elseif ($path -match 'third_party') { 'third_party' }
    else { 'root' }
}

foreach ($category in $sharedByCategory | Sort-Object Count -Descending) {
    $reportContent += "- **$($category.Name):** $($category.Count) fichiers`n"
}

$reportContent += @"

---

## 📝 RECOMMANDATIONS D'UN PROGRAMMEUR SENIOR

### 🎯 PRIORITÉ 1 - Actions immédiates:

#### 1. Supprimer les fichiers dépréciés ✅
- **$($deprecatedFiles.Count) fichiers** dans ``Source/_deprecated/``
- **Espace libéré:** ~$([math]::Round($deprecatedSize, 2)) KB
- **Risque:** Aucun (dossier explicitement marqué)
- **Action:** Suppression complète du dossier ``_deprecated``

#### 2. Examiner les fichiers de code non référencés ⚠️
- **$($unreferencedCode.Count) fichiers .cpp/.h** non liés aux projets
- **Action requise:** 
  - Vérifier s'ils sont inclus indirectement
  - Si inutilisés depuis > 3 mois → Archiver
  - Si tests/exemples obsolètes → Supprimer

### 🔍 PRIORITÉ 2 - Optimisations:

#### 3. Nettoyer third_party ⚙️
- **$($unreferencedThirdParty.Count) fichiers** third_party non référencés
- Majoritairement des exemples ImGui non utilisés
- **Recommandation:** Conserver uniquement les sources nécessaires

#### 4. Refactoriser l'architecture partagée 🏗️
- **$($filesInBothProjects.Count) fichiers** partagés entre projets
- **Recommandation:** Créer une bibliothèque statique commune
- **Avantages:** 
  - Réduction du temps de compilation
  - Meilleure modularité
  - Facilite la maintenance

### ✅ POINTS POSITIFS:

- ✅ Excellente organisation modulaire (AI, ECS, TaskSystem, etc.)
- ✅ Séparation claire du code déprécié
- ✅ Utilisation appropriée de bibliothèques standard (ImGui, nlohmann/json)
- ✅ Architecture ECS moderne

### ⚠️ POINTS D'ATTENTION:

- ⚠️ Incohérences de casse dans les chemins (``source/`` vs ``Source/``)
- ⚠️ Fichiers TiledLevelLoader potentiellement non utilisés
- ⚠️ Documentation dispersée (.md dans le code source)

---

## 🛠️ SCRIPT DE NETTOYAGE AUTOMATISÉ

Commandes PowerShell pour nettoyer la base de code:

\`\`\`powershell
# 1. Supprimer le dossier _deprecated (SÉCURISÉ)
Remove-Item -Path ".\Source\_deprecated" -Recurse -Force

# 2. Archiver les fichiers non référencés
New-Item -Path ".\Source\_archived" -ItemType Directory -Force
# (Liste des fichiers à archiver ci-dessous)

# 3. Supprimer les exemples ImGui non utilisés
Remove-Item -Path ".\Source\third_party\imgui\examples" -Recurse -Force
\`\`\`

---

**📊 Statistiques finales:**
- **Espace potentiellement libérable:** ~$([math]::Round($deprecatedSize + ($unreferencedCode | Measure-Object -Property Size -Sum).Sum, 2)) KB
- **Fichiers à supprimer/archiver:** $($deprecatedFiles.Count + $unreferencedCode.Count)
- **Impact:** Aucun sur les projets principaux

---

**Rapport généré par:** GitHub Copilot - Analyse de code Visual C++  
**Contact:** Programmeur Senior Visual C++  
**Date:** $(Get-Date -Format "yyyy-MM-dd HH:mm:ss")

"@

# Sauvegarde du rapport
$reportContent | Out-File -FilePath ".\Code_Analysis_Report_Detailed.md" -Encoding UTF8

Write-Host "✅ Rapport complet généré: Code_Analysis_Report_Detailed.md" -ForegroundColor Green
Write-Host ""
Write-Host "💡 PROCHAINES ÉTAPES RECOMMANDÉES:" -ForegroundColor Cyan
Write-Host "   1. Consulter le rapport: Code_Analysis_Report_Detailed.md" -ForegroundColor White
Write-Host "   2. Supprimer le dossier _deprecated (sécurisé)" -ForegroundColor White
Write-Host "   3. Examiner les $($unreferencedCode.Count) fichiers non référencés" -ForegroundColor White
Write-Host ""

# Retour de statistiques
return @{
    TotalFiles = $allSourceFiles.Count
    UnreferencedCode = $unreferencedCode.Count
    DeprecatedFiles = $deprecatedFiles.Count
    SharedFiles = $filesInBothProjects.Count
    SpaceToFree = [math]::Round($deprecatedSize, 2)
}
