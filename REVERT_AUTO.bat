@echo off
REM ============================================================================
REM REVERT AUTOMATIQUE - BehaviorTreeDebugWindow.cpp
REM ============================================================================
REM Ce script identifie automatiquement un commit stable et fait le revert

echo.
echo ========================================
echo REVERT BehaviorTreeDebugWindow.cpp
echo ========================================
echo.

REM Vérifier qu'on est dans un repo git
if not exist ".git" (
    echo ERREUR: Pas dans un depot git!
    pause
    exit /b 1
)

echo [1/5] Affichage des 20 derniers commits...
echo.
git log --oneline -20 -- Source/AI/BehaviorTreeDebugWindow.cpp
echo.

REM Suggérer automatiquement le 5ème commit (probablement stable)
echo [2/5] Identification du commit stable...
echo.

REM Récupérer le 5ème commit (index 4 car 0-based)
for /f "skip=4 tokens=1" %%i in ('git log --oneline -20 -- Source/AI/BehaviorTreeDebugWindow.cpp') do (
    set SUGGESTED_COMMIT=%%i
    goto :found_commit
)

:found_commit
echo Commit stable suggere: %SUGGESTED_COMMIT%
echo.
echo IMPORTANT: Regardez la liste ci-dessus.
echo Si ce commit ne semble pas bon, vous pourrez choisir un autre.
echo.

set /p USE_SUGGESTED="Utiliser le commit %SUGGESTED_COMMIT%? (O/N ou entrez un hash): "

if "%USE_SUGGESTED%"=="O" goto :do_revert
if "%USE_SUGGESTED%"=="o" goto :do_revert
if "%USE_SUGGESTED%"=="N" goto :ask_commit
if "%USE_SUGGESTED%"=="n" goto :ask_commit

REM L'utilisateur a entré un hash
set COMMIT_HASH=%USE_SUGGESTED%
goto :do_revert

:ask_commit
set /p COMMIT_HASH="Entrez le hash du commit stable: "
goto :do_revert

:do_revert
if "%COMMIT_HASH%"=="" set COMMIT_HASH=%SUGGESTED_COMMIT%

echo.
echo [3/5] Revert vers commit %COMMIT_HASH%...
echo.

git checkout %COMMIT_HASH% -- Source/AI/BehaviorTreeDebugWindow.cpp
git checkout %COMMIT_HASH% -- Source/AI/BehaviorTreeDebugWindow.h

if errorlevel 1 (
    echo.
    echo ERREUR: Revert echoue!
    echo Verifiez que le hash est correct.
    pause
    exit /b 1
)

echo.
echo [4/5] Verification des fichiers reverted...
git status Source/AI/BehaviorTreeDebugWindow.*

echo.
echo [5/5] Rebuild de la solution...
echo Ceci peut prendre quelques minutes...
echo.

msbuild "Olympe Engine.sln" /t:Clean /p:Configuration=Debug /nologo /v:minimal
msbuild "Olympe Engine.sln" /t:Rebuild /p:Configuration=Debug /nologo /v:minimal

if errorlevel 1 (
    echo.
    echo ========================================
    echo BUILD ECHOUE
    echo ========================================
    echo.
    echo Le revert n'a pas resolu les problemes.
    echo Essayez un commit plus ancien:
    echo 1. Relancez ce script
    echo 2. Choisissez un commit plus ancien dans la liste
    echo.
    pause
    exit /b 1
)

echo.
echo ========================================
echo SUCCESS!
echo ========================================
echo.
echo Build reussie - 0 errors
echo.
echo Fichiers reverted:
echo   - Source/AI/BehaviorTreeDebugWindow.cpp
echo   - Source/AI/BehaviorTreeDebugWindow.h
echo.
echo Commit utilise: %COMMIT_HASH%
echo.
echo PROCHAINES ETAPES:
echo 1. Testez F10 debugger in-game
echo 2. Si OK, committez:
echo    git add Source/AI/BehaviorTreeDebugWindow.*
echo    git commit -m "fix: revert BehaviorTreeDebugWindow to stable state"
echo.
echo 3. Committez la documentation:
echo    git add Docs/Developer/*.md Scripts/*.bat README*.md
echo    git add ACTION_IMMEDIATE_2MIN.md TABLEAU_DE_BORD_FINAL.md
echo    git add ETAT_BUILD_ACTUEL.md BILAN_COMPLET_SESSION.md
echo    git add START_HERE.md EXECUTION_AUTOMATIQUE.md
echo    git add PR_DESCRIPTION.md
echo    git add Source/NodeGraphShared/BTDebugAdapter.*
echo    git add Source/NodeGraphCore/Commands/DeleteLinkCommand.cpp
echo    git commit -m "docs(nodegraph): Phase 1 complete - 95KB documentation + fixes"
echo    git push origin master
echo.
pause
