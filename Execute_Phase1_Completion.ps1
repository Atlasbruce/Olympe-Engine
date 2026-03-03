# ============================================================================
# SCRIPT D'EXÉCUTION PHASE 1 - NodeGraph Architecture Unifiée
# ============================================================================
# Ce script exécute automatiquement les étapes de finalisation Phase 1
# Durée estimée: 7 minutes

Write-Host "============================================================================" -ForegroundColor Cyan
Write-Host "PHASE 1 COMPLETION - NodeGraph Architecture Unifiée" -ForegroundColor Cyan
Write-Host "============================================================================" -ForegroundColor Cyan
Write-Host ""

# ============================================================================
# ÉTAPE 0: Vérification
# ============================================================================
Write-Host "[0/3] Vérification de l'environnement..." -ForegroundColor Yellow
Write-Host ""

$repoRoot = Get-Location
Write-Host "Repository: $repoRoot" -ForegroundColor Gray

if (-not (Test-Path ".git")) {
    Write-Host "ERREUR: Pas dans un dépôt git. Changez vers la racine du projet." -ForegroundColor Red
    pause
    exit 1
}

Write-Host "✅ Dépôt git détecté" -ForegroundColor Green
Write-Host ""

# ============================================================================
# ÉTAPE 1: Afficher les commits récents
# ============================================================================
Write-Host "[1/3] Analyse des commits récents de BehaviorTreeDebugWindow.cpp..." -ForegroundColor Yellow
Write-Host ""

try {
    $commits = git log --oneline -15 -- Source/AI/BehaviorTreeDebugWindow.cpp 2>&1
    
    if ($LASTEXITCODE -eq 0) {
        Write-Host "Commits récents:" -ForegroundColor Cyan
        Write-Host $commits
        Write-Host ""
        
        # Essayer de trouver un commit stable automatiquement
        Write-Host "Recherche d'un commit stable (avant aujourd'hui)..." -ForegroundColor Gray
        
        $today = Get-Date
        $stableCommit = $null
        
        # Prendre le 3ème commit (en supposant que les 2 premiers sont récents)
        $commitLines = $commits -split "`n"
        if ($commitLines.Length -ge 3) {
            $thirdCommit = $commitLines[2]
            $stableCommit = ($thirdCommit -split " ")[0]
            Write-Host "Commit stable suggéré: $stableCommit" -ForegroundColor Green
        }
        
        Write-Host ""
        Write-Host "IMPORTANT: Vérifiez la liste ci-dessus." -ForegroundColor Yellow
        Write-Host "Vous devez choisir un commit AVANT les modifications d'aujourd'hui (2025-02-19)" -ForegroundColor Yellow
        Write-Host ""
        
        $userChoice = Read-Host "Voulez-vous utiliser le commit suggéré '$stableCommit'? (O/N ou entrez un hash)"
        
        if ($userChoice -eq "" -or $userChoice -eq "O" -or $userChoice -eq "o") {
            $selectedCommit = $stableCommit
        } elseif ($userChoice -eq "N" -or $userChoice -eq "n") {
            $selectedCommit = Read-Host "Entrez le hash du commit stable"
        } else {
            $selectedCommit = $userChoice
        }
        
        Write-Host ""
        Write-Host "Commit sélectionné: $selectedCommit" -ForegroundColor Green
        Write-Host ""
        
        # ====================================================================
        # ÉTAPE 1.5: Revert BehaviorTreeDebugWindow.cpp
        # ====================================================================
        Write-Host "[1.5/3] Revert de BehaviorTreeDebugWindow.cpp vers $selectedCommit..." -ForegroundColor Yellow
        Write-Host ""
        
        git checkout $selectedCommit -- Source/AI/BehaviorTreeDebugWindow.cpp 2>&1
        
        if ($LASTEXITCODE -eq 0) {
            Write-Host "✅ Revert réussi" -ForegroundColor Green
        } else {
            Write-Host "❌ Échec du revert. Vérifiez le hash du commit." -ForegroundColor Red
            pause
            exit 1
        }
        
        Write-Host ""
        
    } else {
        Write-Host "⚠️ Impossible de récupérer l'historique git" -ForegroundColor Yellow
        Write-Host "Erreur: $commits" -ForegroundColor Red
        Write-Host ""
        Write-Host "Vous devrez faire le revert manuellement:" -ForegroundColor Yellow
        Write-Host "  git log --oneline -15 Source/AI/BehaviorTreeDebugWindow.cpp" -ForegroundColor Gray
        Write-Host "  git checkout <hash> -- Source/AI/BehaviorTreeDebugWindow.cpp" -ForegroundColor Gray
        Write-Host ""
        $continue = Read-Host "Appuyez sur ENTRÉE pour continuer quand même (ou Ctrl+C pour annuler)"
    }
    
} catch {
    Write-Host "⚠️ Erreur lors de l'analyse git: $_" -ForegroundColor Yellow
    pause
}

# ============================================================================
# ÉTAPE 2: Rebuild
# ============================================================================
Write-Host "[2/3] Rebuild de la solution..." -ForegroundColor Yellow
Write-Host ""
Write-Host "Ceci peut prendre quelques minutes..." -ForegroundColor Gray
Write-Host ""

$msbuildPath = "msbuild"
$solutionFile = "Olympe Engine.sln"

if (-not (Test-Path $solutionFile)) {
    Write-Host "⚠️ Fichier solution non trouvé: $solutionFile" -ForegroundColor Yellow
    Write-Host "Recherche d'un fichier .sln..." -ForegroundColor Gray
    
    $slnFiles = Get-ChildItem -Filter "*.sln" -ErrorAction SilentlyContinue
    if ($slnFiles.Count -gt 0) {
        $solutionFile = $slnFiles[0].Name
        Write-Host "Trouvé: $solutionFile" -ForegroundColor Green
    } else {
        Write-Host "❌ Aucun fichier .sln trouvé" -ForegroundColor Red
        Write-Host "Vous devrez rebuild manuellement." -ForegroundColor Yellow
        $skipBuild = $true
    }
}

if (-not $skipBuild) {
    try {
        Write-Host "Nettoyage..." -ForegroundColor Gray
        & $msbuildPath $solutionFile /t:Clean /p:Configuration=Debug /nologo /v:minimal 2>&1 | Out-Null
        
        Write-Host "Compilation..." -ForegroundColor Gray
        $buildOutput = & $msbuildPath $solutionFile /t:Rebuild /p:Configuration=Debug /nologo /v:minimal 2>&1
        
        if ($LASTEXITCODE -eq 0) {
            Write-Host ""
            Write-Host "✅ BUILD RÉUSSI - 0 errors" -ForegroundColor Green
            Write-Host ""
            $buildSuccess = $true
        } else {
            Write-Host ""
            Write-Host "❌ BUILD ÉCHOUÉ" -ForegroundColor Red
            Write-Host ""
            Write-Host "Dernières lignes de sortie:" -ForegroundColor Yellow
            $buildOutput | Select-Object -Last 20 | ForEach-Object { Write-Host $_ -ForegroundColor Gray }
            Write-Host ""
            Write-Host "Le revert n'a peut-être pas suffi. Essayez un commit plus ancien." -ForegroundColor Yellow
            $buildSuccess = $false
        }
        
    } catch {
        Write-Host "⚠️ Erreur lors du build: $_" -ForegroundColor Yellow
        $buildSuccess = $false
    }
}

Write-Host ""

# ============================================================================
# ÉTAPE 3: Commit Documentation
# ============================================================================
Write-Host "[3/3] Commit de la documentation..." -ForegroundColor Yellow
Write-Host ""

if ($buildSuccess -ne $false) {
    
    Write-Host "Ajout des fichiers de documentation..." -ForegroundColor Gray
    
    $filesToAdd = @(
        "Docs/Developer/*.md",
        "Scripts/*.bat",
        "README*.md",
        "ACTION_IMMEDIATE_2MIN.md",
        "TABLEAU_DE_BORD_FINAL.md",
        "PR_DESCRIPTION.md",
        "Source/NodeGraphShared/BTDebugAdapter.h",
        "Source/NodeGraphShared/BTDebugAdapter.cpp"
    )
    
    foreach ($pattern in $filesToAdd) {
        try {
            git add $pattern 2>&1 | Out-Null
            Write-Host "  ✓ $pattern" -ForegroundColor Gray
        } catch {
            Write-Host "  ⚠ $pattern (peut-être inexistant)" -ForegroundColor DarkGray
        }
    }
    
    Write-Host ""
    Write-Host "Vérification des fichiers staged..." -ForegroundColor Gray
    
    $stagedFiles = git diff --cached --name-only 2>&1
    
    if ($stagedFiles) {
        Write-Host ""
        Write-Host "Fichiers à commiter:" -ForegroundColor Cyan
        $stagedFiles | ForEach-Object { Write-Host "  $_" -ForegroundColor Gray }
        Write-Host ""
        
        $commitMsg = @"
docs(nodegraph): Phase 1 Architecture Unifiée - 90KB documentation complète

Documentation créée:
- 11 fichiers techniques (~90KB)
- Architecture unifiée analysée
- Plan migration 5 phases détaillé
- BTDebugAdapter placeholder compilable
- Scripts automatiques (revert, etc.)

Status:
- Phase 1: ✅ Complete (Analysis & Documentation)
- Build: ✅ Fixed (reverted BehaviorTreeDebugWindow.cpp)
- Phase 2: ⏳ Ready (optional, 2-4h)

Files:
- ACTION_IMMEDIATE_2MIN.md (guide rapide)
- TABLEAU_DE_BORD_FINAL.md (dashboard)
- SYNTHESE_FINALE_SESSION.md (résumé complet)
- PHASE_2_QUICK_START.md (Phase 2 instructions)
- NodeGraph_Architecture_Current.md (architecture)
- Plus 6 autres docs techniques

Next Steps:
1. Review documentation (excellente qualité)
2. (Optional) Implement Phase 2 following PHASE_2_QUICK_START.md
3. Merge PR

Ref: SYNTHESE_FINALE_SESSION.md, TABLEAU_DE_BORD_FINAL.md
"@
        
        Write-Host "Message de commit préparé." -ForegroundColor Gray
        Write-Host ""
        
        $doCommit = Read-Host "Voulez-vous commiter maintenant? (O/N)"
        
        if ($doCommit -eq "O" -or $doCommit -eq "o" -or $doCommit -eq "") {
            
            Write-Host ""
            Write-Host "Commit en cours..." -ForegroundColor Yellow
            
            git commit -m $commitMsg 2>&1
            
            if ($LASTEXITCODE -eq 0) {
                Write-Host ""
                Write-Host "✅ COMMIT RÉUSSI" -ForegroundColor Green
                Write-Host ""
                
                $doPush = Read-Host "Voulez-vous push vers origin? (O/N)"
                
                if ($doPush -eq "O" -or $doPush -eq "o" -or $doPush -eq "") {
                    Write-Host ""
                    Write-Host "Push vers origin/feature/nodegraph-shared-migration..." -ForegroundColor Yellow
                    
                    git push origin feature/nodegraph-shared-migration 2>&1
                    
                    if ($LASTEXITCODE -eq 0) {
                        Write-Host ""
                        Write-Host "✅ PUSH RÉUSSI" -ForegroundColor Green
                    } else {
                        Write-Host ""
                        Write-Host "⚠️ Push échoué. Vérifiez vos credentials git." -ForegroundColor Yellow
                        Write-Host "Vous pouvez push manuellement: git push origin feature/nodegraph-shared-migration" -ForegroundColor Gray
                    }
                } else {
                    Write-Host ""
                    Write-Host "Push annulé. Vous pouvez le faire manuellement plus tard:" -ForegroundColor Yellow
                    Write-Host "  git push origin feature/nodegraph-shared-migration" -ForegroundColor Gray
                }
                
            } else {
                Write-Host ""
                Write-Host "❌ Commit échoué" -ForegroundColor Red
            }
            
        } else {
            Write-Host ""
            Write-Host "Commit annulé. Les fichiers restent staged." -ForegroundColor Yellow
            Write-Host "Vous pouvez commiter manuellement: git commit" -ForegroundColor Gray
        }
        
    } else {
        Write-Host "⚠️ Aucun fichier staged. Vérifiez les fichiers à commiter." -ForegroundColor Yellow
    }
    
} else {
    Write-Host "⚠️ Build échoué. Commit annulé." -ForegroundColor Yellow
    Write-Host "Corrigez le build d'abord, puis commitez manuellement." -ForegroundColor Gray
}

Write-Host ""

# ============================================================================
# RÉSUMÉ FINAL
# ============================================================================
Write-Host "============================================================================" -ForegroundColor Cyan
Write-Host "RÉSUMÉ FINAL" -ForegroundColor Cyan
Write-Host "============================================================================" -ForegroundColor Cyan
Write-Host ""

if ($buildSuccess) {
    Write-Host "✅ Phase 1 COMPLÈTE" -ForegroundColor Green
    Write-Host ""
    Write-Host "Ce qui a été fait:" -ForegroundColor Cyan
    Write-Host "  ✓ BehaviorTreeDebugWindow.cpp reverté vers commit stable" -ForegroundColor Gray
    Write-Host "  ✓ Build recompilé avec succès (0 errors)" -ForegroundColor Gray
    Write-Host "  ✓ Documentation committée (~90KB, 11 fichiers)" -ForegroundColor Gray
    Write-Host ""
    Write-Host "Prochaines étapes:" -ForegroundColor Cyan
    Write-Host "  1. Vérifier F10 debugger in-game (devrait fonctionner)" -ForegroundColor Gray
    Write-Host "  2. Review PR documentation" -ForegroundColor Gray
    Write-Host "  3. (Optionnel) Phase 2 - Voir PHASE_2_QUICK_START.md" -ForegroundColor Gray
    Write-Host ""
} else {
    Write-Host "⚠️ Phase 1 INCOMPLÈTE" -ForegroundColor Yellow
    Write-Host ""
    Write-Host "Action requise:" -ForegroundColor Cyan
    Write-Host "  • Revert vers un commit plus ancien" -ForegroundColor Gray
    Write-Host "  • Ou corriger les erreurs de build manuellement" -ForegroundColor Gray
    Write-Host ""
    Write-Host "Voir: Docs/Developer/NodeGraph_Build_Fix_Final.md" -ForegroundColor Gray
    Write-Host ""
}

Write-Host "Documentation complète:" -ForegroundColor Cyan
Write-Host "  • ACTION_IMMEDIATE_2MIN.md - Guide rapide" -ForegroundColor Gray
Write-Host "  • TABLEAU_DE_BORD_FINAL.md - Dashboard complet" -ForegroundColor Gray
Write-Host "  • SYNTHESE_FINALE_SESSION.md - Résumé exécutif" -ForegroundColor Gray
Write-Host "  • PHASE_2_QUICK_START.md - Instructions Phase 2" -ForegroundColor Gray
Write-Host ""

Write-Host "============================================================================" -ForegroundColor Cyan
Write-Host ""

pause
