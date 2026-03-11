# Font Awesome Integration - Installation rapide

## ✅ Ce qui a été fait

### 1. Fichiers créés
- ✅ `Source/Core/IconsFontAwesome6.h` - Définitions des icônes Font Awesome 6
- ✅ `Source/Core/FontManager.h` - Gestionnaire de polices (header)
- ✅ `Source/Core/FontManager.cpp` - Implémentation du chargement de polices
- ✅ `Assets/Fonts/README.md` - Instructions de téléchargement Font Awesome
- ✅ `Docs/FONT_AWESOME_INTEGRATION.md` - Documentation complète d'utilisation

### 2. Fichiers modifiés
- ✅ `Source/BlueprintEditor/BlueprintEditorGUI.h` - Ajout include IconsFontAwesome6.h
- ✅ `Source/BlueprintEditor/BlueprintEditorGUI.cpp` - Initialisation FontManager + icônes dans menus
- ✅ `Source/AI/AIEditor/AIEditorMenus.cpp` - Icônes dans tous les menus AI

### 3. Icônes ajoutées

#### Menus du Blueprint Editor (File, Edit, Tools, View, Help)
- File : 📄 Nouveau, 📁 Ouvrir, 💾 Sauvegarder, ❌ Fermer, etc.
- Edit : ↶ Undo, ↷ Redo, ✂️ Cut, 📋 Copy/Paste, 🗑️ Delete, ⚙️ Preferences
- Tools : ➡️ Migration, ✓ Validation
- View : 👁️ Panels, 🐛 Debugger, 📊 Profiler
- Help : ❓ Aide, ℹ️ About, 📖 Documentation

#### Menus de l'AI Editor (File, Edit, View, Help)
- File : 🧠 Behavior Tree, 📊 HFSM, 📁 Open, 💾 Save
- Edit : ↶ Undo, ↷ Redo, ✂️ Cut, 📋 Copy, 🗑️ Delete
- View : 🧊 Node Palette, 📋 Blackboard, 🔧 Senses, 🐛 Debug
- Help : ❓ Aide, ℹ️ About

## 🚀 Pour utiliser

### Étape 1 : Télécharger Font Awesome
1. Allez sur https://fontawesome.com/download
2. Téléchargez "Font Awesome Free for Desktop"
3. Extrayez et copiez `Font Awesome 6 Free-Solid-900.otf` vers `Assets/Fonts/`
4. Renommez en `fa-solid-900.otf`

Voir `Assets/Fonts/README.md` pour plus de détails.

### Étape 2 : Compiler
1. Fermez l'éditeur Blueprint s'il est en cours d'exécution
2. Compilez le projet
3. Lancez l'éditeur

### Étape 3 : Vérifier
Au démarrage, vous devriez voir dans les logs :
```
[FontManager] Initialized
[FontManager] Font Awesome loaded successfully from: Assets/Fonts/fa-solid-900.otf
```

Les icônes apparaîtront automatiquement dans tous les menus !

## 📖 Documentation complète

Consultez `Docs/FONT_AWESOME_INTEGRATION.md` pour :
- Liste complète des icônes disponibles
- Exemples d'utilisation dans le code
- Comment ajouter des icônes à vos propres menus/boutons
- Personnalisation de la taille des icônes

## 🎨 Exemples d'utilisation

```cpp
// Dans vos propres menus
#include "../Core/IconsFontAwesome6.h"

if (ImGui::BeginMenu(ICON_FA_FILE " File")) {
    if (ImGui::MenuItem(ICON_FA_FOLDER_OPEN " Open")) {
        // Action
    }
    ImGui::EndMenu();
}

// Dans vos boutons
if (ImGui::Button(ICON_FA_PLAY " Start")) {
    StartSimulation();
}
```

## ⚠️ Notes importantes

1. **Encodage UTF-8** : Les fichiers sources doivent être en UTF-8
2. **Police requise** : Les icônes ne s'afficheront pas sans la police Font Awesome
3. **Espacement** : Toujours ajouter un espace après l'icône : `ICON_FA_FILE " Text"`

## 📝 License

Font Awesome Free est sous licence **SIL OFL 1.1** (gratuit pour usage commercial).

## 🐛 Problèmes ?

Si les icônes ne s'affichent pas :
1. Vérifiez que `Assets/Fonts/fa-solid-900.otf` existe
2. Vérifiez les logs de FontManager
3. Vérifiez que votre fichier .cpp est en UTF-8
4. Consultez la documentation complète

---

**✨ Résultat** : Votre éditeur Blueprint a maintenant des icônes professionnelles dans tous les menus !
