# ============================================================================
# REVERT DIRECT - Sans interaction utilisateur
# ============================================================================
# Ce script fait le revert automatiquement vers HEAD~5

Write-Host ""
Write-Host "========================================" -ForegroundColor Cyan
Write-Host "REVERT AUTOMATIQUE - BehaviorTreeDebugWindow.cpp" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan
Write-Host ""

# Chercher git dans les emplacements courants
$gitPaths = @(
    "C:\Program Files\Git\cmd\git.exe",
    "C:\Program Files (x86)\Git\cmd\git.exe",
    "$env:LOCALAPPDATA\Programs\Git\cmd\git.exe",
    "git" # Dans le PATH
)

$gitExe = $null
foreach ($path in $gitPaths) {
    try {
        if (Test-Path $path -ErrorAction SilentlyContinue) {
            $gitExe = $path
            break
        } elseif ($path -eq "git") {
            $testGit = & git --version 2>&1
            if ($LASTEXITCODE -eq 0) {
                $gitExe = "git"
                break
            }
        }
    } catch {
        continue
    }
}

if (-not $gitExe) {
    Write-Host "ERREUR: Git non trouvé!" -ForegroundColor Red
    Write-Host "Installez Git ou ajoutez-le au PATH" -ForegroundColor Yellow
    pause
    exit 1
}

Write-Host "Git trouvé: $gitExe" -ForegroundColor Green
Write-Host ""

# Vérifier qu'on est dans un repo git
if (-not (Test-Path ".git")) {
    Write-Host "ERREUR: Pas dans un dépôt git!" -ForegroundColor Red
    pause
    exit 1
}

Write-Host "[1/4] Revert vers HEAD~5 (5 commits en arrière)..." -ForegroundColor Yellow
Write-Host "Ceci restaure une version probablement stable." -ForegroundColor Gray
Write-Host ""

try {
    # Revert vers 5 commits en arrière
    & $gitExe checkout HEAD~5 -- Source/AI/BehaviorTreeDebugWindow.cpp 2>&1
    & $gitExe checkout HEAD~5 -- Source/AI/BehaviorTreeDebugWindow.h 2>&1
    
    if ($LASTEXITCODE -ne 0) {
        throw "Git checkout failed"
    }
    
    Write-Host "✅ Revert réussi" -ForegroundColor Green
    Write-Host ""
    
} catch {
    Write-Host "❌ Revert échoué: $_" -ForegroundColor Red
    Write-Host ""
    Write-Host "Essayez manuellement:" -ForegroundColor Yellow
    Write-Host "  git checkout HEAD~7 -- Source/AI/BehaviorTreeDebugWindow.*" -ForegroundColor Gray
    pause
    exit 1
}

Write-Host "[2/4] Vérification des fichiers..." -ForegroundColor Yellow
& $gitExe status Source/AI/BehaviorTreeDebugWindow.*
Write-Host ""

Write-Host "[3/4] Rebuild de la solution..." -ForegroundColor Yellow
Write-Host "Ceci peut prendre 3-5 minutes..." -ForegroundColor Gray
Write-Host ""

# Chercher msbuild
$msbuildPaths = @(
    "C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe",
    "C:\Program Files\Microsoft Visual Studio\2022\Professional\MSBuild\Current\Bin\MSBuild.exe",
    "C:\Program Files\Microsoft Visual Studio\2022\Enterprise\MSBuild\Current\Bin\MSBuild.exe",
    "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\MSBuild\Current\Bin\MSBuild.exe",
    "msbuild" # Dans le PATH
)

$msbuildExe = $null
foreach ($path in $msbuildPaths) {
    try {
        if (Test-Path $path -ErrorAction SilentlyContinue) {
            $msbuildExe = $path
            break
        } elseif ($path -eq "msbuild") {
            $testMsbuild = & msbuild /version 2>&1
            if ($LASTEXITCODE -eq 0) {
                $msbuildExe = "msbuild"
                break
            }
        }
    } catch {
        continue
    }
}

if (-not $msbuildExe) {
    Write-Host "⚠️ MSBuild non trouvé. Rebuild manuellement:" -ForegroundColor Yellow
    Write-Host "  msbuild 'Olympe Engine.sln' /t:Rebuild /p:Configuration=Debug" -ForegroundColor Gray
    pause
    exit 1
}

Write-Host "MSBuild trouvé: $msbuildExe" -ForegroundColor Green
Write-Host ""

try {
    # Clean
    Write-Host "Cleaning..." -ForegroundColor Gray
    & $msbuildExe "Olympe Engine.sln" /t:Clean /p:Configuration=Debug /nologo /v:minimal 2>&1 | Out-Null
    
    # Rebuild
    Write-Host "Building..." -ForegroundColor Gray
    $buildOutput = & $msbuildExe "Olympe Engine.sln" /t:Rebuild /p:Configuration=Debug /nologo /v:minimal 2>&1
    
    if ($LASTEXITCODE -eq 0) {
        Write-Host ""
        Write-Host "========================================" -ForegroundColor Green
        Write-Host "✅ BUILD RÉUSSI - 0 ERRORS" -ForegroundColor Green
        Write-Host "========================================" -ForegroundColor Green
        Write-Host ""
        
        Write-Host "Fichiers restaurés:" -ForegroundColor Cyan
        Write-Host "  ✓ Source/AI/BehaviorTreeDebugWindow.cpp" -ForegroundColor Gray
        Write-Host "  ✓ Source/AI/BehaviorTreeDebugWindow.h" -ForegroundColor Gray
        Write-Host ""
        
        Write-Host "PROCHAINES ÉTAPES:" -ForegroundColor Cyan
        Write-Host ""
        Write-Host "1. Testez F10 debugger in-game" -ForegroundColor White
        Write-Host "   - Lancez le jeu" -ForegroundColor Gray
        Write-Host "   - Appuyez F10" -ForegroundColor Gray
        Write-Host "   - Vérifiez que le debugger s'ouvre" -ForegroundColor Gray
        Write-Host ""
        
        Write-Host "2. Committez le revert:" -ForegroundColor White
        Write-Host "   git add Source/AI/BehaviorTreeDebugWindow.*" -ForegroundColor Gray
        Write-Host "   git commit -m 'fix: revert BehaviorTreeDebugWindow to stable state'" -ForegroundColor Gray
        Write-Host ""
        
        Write-Host "3. Committez la documentation:" -ForegroundColor White
        Write-Host "   git add Docs/Developer/*.md Scripts/*.bat Scripts/*.ps1 Scripts/*.sh *.md" -ForegroundColor Gray
        Write-Host "   git add Source/NodeGraphShared/BTDebugAdapter.*" -ForegroundColor Gray
        Write-Host "   git add Source/NodeGraphCore/Commands/DeleteLinkCommand.cpp" -ForegroundColor Gray
        Write-Host "   git commit -m 'docs(nodegraph): Phase 1 complete - 100KB documentation'" -ForegroundColor Gray
        Write-Host ""
        
        Write-Host "4. Push:" -ForegroundColor White
        Write-Host "   git push origin master" -ForegroundColor Gray
        Write-Host ""
        
        Write-Host "========================================" -ForegroundColor Cyan
        Write-Host "🎉 PHASE 1 COMPLÈTE!" -ForegroundColor Cyan
        Write-Host "========================================" -ForegroundColor Cyan
        Write-Host ""
        
    } else {
        Write-Host ""
        Write-Host "========================================" -ForegroundColor Red
        Write-Host "❌ BUILD ÉCHOUÉ" -ForegroundColor Red
        Write-Host "========================================" -ForegroundColor Red
        Write-Host ""
        Write-Host "Dernières lignes de sortie:" -ForegroundColor Yellow
        $buildOutput | Select-Object -Last 30 | ForEach-Object { Write-Host $_ -ForegroundColor Gray }
        Write-Host ""
        Write-Host "Le commit HEAD~5 n'était pas assez ancien." -ForegroundColor Yellow
        Write-Host "Essayez un commit plus ancien:" -ForegroundColor Yellow
        Write-Host "  git checkout HEAD~7 -- Source/AI/BehaviorTreeDebugWindow.*" -ForegroundColor Gray
        Write-Host "  git checkout HEAD~10 -- Source/AI/BehaviorTreeDebugWindow.*" -ForegroundColor Gray
        Write-Host ""
    }
    
} catch {
    Write-Host "❌ Erreur lors du build: $_" -ForegroundColor Red
}

Write-Host ""
Write-Host "[4/4] Terminé" -ForegroundColor Yellow
Write-Host ""
pause
