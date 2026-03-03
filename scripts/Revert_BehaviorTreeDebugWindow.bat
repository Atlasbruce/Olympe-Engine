@echo off
REM ========================================
REM REVERT SCRIPT - BehaviorTreeDebugWindow
REM ========================================
REM
REM Ce script restaure BehaviorTreeDebugWindow.cpp à un état stable
REM pour permettre la Phase 2 de l'intégration NodeGraph

echo.
echo ================================================
echo REVERT BehaviorTreeDebugWindow to Stable State
echo ================================================
echo.

REM Trouver les commits récents
echo [1/5] Finding recent commits...
git log --oneline -15 Source/AI/BehaviorTreeDebugWindow.cpp

echo.
echo ================================================
echo IMPORTANT: Identifier le commit stable
echo ================================================
echo.
echo Regardez la liste ci-dessus et identifiez un commit AVANT
echo les modifications d'aujourd'hui qui semblait stable.
echo.
echo Par exemple, cherchez:
echo   - "feat: BT debug working"
echo   - "fix: stable version"
echo   - Ou un commit date d'hier/avant-hier
echo.
set /p COMMIT_HASH="Entrez le hash du commit stable (ex: abc1234): "

if "%COMMIT_HASH%"=="" (
    echo.
    echo ERREUR: Aucun hash fourni. Abandon.
    pause
    exit /b 1
)

echo.
echo [2/5] Reverting to commit %COMMIT_HASH%...
git checkout %COMMIT_HASH% -- Source/AI/BehaviorTreeDebugWindow.cpp
git checkout %COMMIT_HASH% -- Source/AI/BehaviorTreeDebugWindow.h

if errorlevel 1 (
    echo.
    echo ERREUR: Revert echoue. Verifiez le hash du commit.
    pause
    exit /b 1
)

echo.
echo [3/5] Cleaning build...
msbuild "Olympe Engine.sln" /t:Clean /p:Configuration=Debug /nologo /v:m

echo.
echo [4/5] Rebuilding solution...
msbuild "Olympe Engine.sln" /t:Rebuild /p:Configuration=Debug /nologo /v:m

if errorlevel 1 (
    echo.
    echo ================================================
    echo BUILD FAILED
    echo ================================================
    echo.
    echo Le revert n'a pas resolu les problemes.
    echo Essayez un commit plus ancien.
    echo.
    pause
    exit /b 1
)

echo.
echo [5/5] Verifying git status...
git status Source/AI/BehaviorTreeDebugWindow.*

echo.
echo ================================================
echo SUCCESS - Build Restored
echo ================================================
echo.
echo Le fichier BehaviorTreeDebugWindow.cpp a ete restore.
echo La compilation a reussi.
echo.
echo NEXT STEPS:
echo 1. Testez F10 debugger in-game
echo 2. Si OK, committez: git add Source/AI/BehaviorTreeDebugWindow.*
echo 3. Puis procedez a Phase 2
echo.
echo Documentation Phase 2:
echo   - Docs/Developer/SESSION_FINAL_SUMMARY_NodeGraph.md
echo   - README_NodeGraph_QuickStart.md
echo.
pause
