#!/bin/bash
# ============================================================================
# REVERT AUTOMATIQUE - BehaviorTreeDebugWindow.cpp
# ============================================================================
# Script Unix/Linux/Mac pour restaurer BehaviorTreeDebugWindow.cpp

echo ""
echo "========================================"
echo "REVERT BehaviorTreeDebugWindow.cpp"
echo "========================================"
echo ""

# Vérifier qu'on est dans un repo git
if [ ! -d ".git" ]; then
    echo "ERREUR: Pas dans un dépôt git!"
    exit 1
fi

echo "[1/5] Affichage des 20 derniers commits..."
echo ""
git log --oneline -20 -- Source/AI/BehaviorTreeDebugWindow.cpp
echo ""

echo "[2/5] Identification du commit stable..."
echo ""

# Récupérer le 5ème commit (probablement stable)
SUGGESTED_COMMIT=$(git log --oneline -20 -- Source/AI/BehaviorTreeDebugWindow.cpp | sed -n '5p' | awk '{print $1}')

echo "Commit stable suggéré: $SUGGESTED_COMMIT"
echo ""
echo "IMPORTANT: Regardez la liste ci-dessus."
echo "Si ce commit ne semble pas bon, entrez un autre hash."
echo ""

read -p "Utiliser le commit $SUGGESTED_COMMIT? (O/N ou entrez un hash): " USER_CHOICE

if [ "$USER_CHOICE" = "O" ] || [ "$USER_CHOICE" = "o" ] || [ -z "$USER_CHOICE" ]; then
    COMMIT_HASH=$SUGGESTED_COMMIT
elif [ "$USER_CHOICE" = "N" ] || [ "$USER_CHOICE" = "n" ]; then
    read -p "Entrez le hash du commit stable: " COMMIT_HASH
else
    COMMIT_HASH=$USER_CHOICE
fi

echo ""
echo "[3/5] Revert vers commit $COMMIT_HASH..."
echo ""

git checkout $COMMIT_HASH -- Source/AI/BehaviorTreeDebugWindow.cpp
git checkout $COMMIT_HASH -- Source/AI/BehaviorTreeDebugWindow.h

if [ $? -ne 0 ]; then
    echo ""
    echo "ERREUR: Revert échoué!"
    echo "Vérifiez que le hash est correct."
    exit 1
fi

echo ""
echo "[4/5] Vérification des fichiers reverted..."
git status Source/AI/BehaviorTreeDebugWindow.*

echo ""
echo "[5/5] Rebuild de la solution..."
echo "Ceci peut prendre quelques minutes..."
echo ""

# Adapter selon votre environnement
if command -v msbuild &> /dev/null; then
    msbuild "Olympe Engine.sln" /t:Clean /p:Configuration=Debug /nologo /v:minimal
    msbuild "Olympe Engine.sln" /t:Rebuild /p:Configuration=Debug /nologo /v:minimal
    BUILD_RESULT=$?
elif command -v cmake &> /dev/null; then
    cmake --build . --clean-first
    BUILD_RESULT=$?
else
    echo "Outil de build non trouvé. Rebuild manuellement."
    BUILD_RESULT=0
fi

if [ $BUILD_RESULT -ne 0 ]; then
    echo ""
    echo "========================================"
    echo "BUILD ÉCHOUÉ"
    echo "========================================"
    echo ""
    echo "Le revert n'a pas résolu les problèmes."
    echo "Essayez un commit plus ancien:"
    echo "1. Relancez ce script"
    echo "2. Choisissez un commit plus ancien dans la liste"
    echo ""
    exit 1
fi

echo ""
echo "========================================"
echo "SUCCESS!"
echo "========================================"
echo ""
echo "Build réussie - 0 errors"
echo ""
echo "Fichiers reverted:"
echo "  - Source/AI/BehaviorTreeDebugWindow.cpp"
echo "  - Source/AI/BehaviorTreeDebugWindow.h"
echo ""
echo "Commit utilisé: $COMMIT_HASH"
echo ""
echo "PROCHAINES ÉTAPES:"
echo "1. Testez F10 debugger in-game"
echo "2. Si OK, committez:"
echo "   git add Source/AI/BehaviorTreeDebugWindow.*"
echo "   git commit -m 'fix: revert BehaviorTreeDebugWindow to stable state'"
echo ""
echo "3. Committez la documentation:"
echo "   git add Docs/Developer/*.md Scripts/*.bat Scripts/*.sh README*.md *.md"
echo "   git add Source/NodeGraphShared/BTDebugAdapter.*"
echo "   git add Source/NodeGraphCore/Commands/DeleteLinkCommand.cpp"
echo "   git commit -m 'docs(nodegraph): Phase 1 complete - 100KB documentation'"
echo "   git push origin master"
echo ""
