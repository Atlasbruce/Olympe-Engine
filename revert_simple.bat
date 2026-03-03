@echo off
REM Revert simple vers HEAD~5
echo Revert vers HEAD~5...
git checkout HEAD~5 -- Source/AI/BehaviorTreeDebugWindow.cpp
git checkout HEAD~5 -- Source/AI/BehaviorTreeDebugWindow.h
echo.
echo Fichiers reverted. Verifiez avec: git status
pause
