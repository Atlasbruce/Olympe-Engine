# Font Awesome Integration - Guide d'utilisation

Ce document explique comment utiliser les icônes Font Awesome dans l'éditeur Blueprint d'Olympe Engine.

## Configuration

### 1. Prérequis
- Police Font Awesome 6 Free Solid installée dans `Assets/Fonts/fa-solid-900.otf`
- Voir `Assets/Fonts/README.md` pour les instructions de téléchargement

### 2. Initialisation
Le système de polices est automatiquement initialisé au démarrage de l'éditeur Blueprint :
```cpp
// Dans BlueprintEditorGUI::Initialize()
FontManager::Get().Initialize();
FontManager::Get().LoadFontAwesome("Assets/Fonts/fa-solid-900.otf", 16.0f);
```

## Utilisation dans le code

### 1. Inclure les headers nécessaires
```cpp
#include "../Core/IconsFontAwesome6.h"
#include "../Core/FontManager.h"
```

### 2. Utiliser les icônes dans les menus
```cpp
// Menu avec icône
if (ImGui::BeginMenu(ICON_FA_FILE " File")) {
    // Menu items avec icônes
    if (ImGui::MenuItem(ICON_FA_FOLDER_OPEN " Open", "Ctrl+O")) {
        // Action
    }
    if (ImGui::MenuItem(ICON_FA_FLOPPY_DISK " Save", "Ctrl+S")) {
        // Action
    }
    ImGui::EndMenu();
}
```

### 3. Utiliser les icônes dans les boutons
```cpp
if (ImGui::Button(ICON_FA_PLUS " Add Component")) {
    // Action
}

if (ImGui::Button(ICON_FA_TRASH_CAN " Delete")) {
    // Action
}
```

### 4. Utiliser les icônes dans le texte
```cpp
ImGui::Text(ICON_FA_CIRCLE_INFO " Information importante");
ImGui::TextColored(ImVec4(1,0,0,1), ICON_FA_TRIANGLE_EXCLAMATION " Erreur !");
```

## Icônes disponibles

### Fichiers et dossiers
- `ICON_FA_FILE` - Fichier générique
- `ICON_FA_FILE_CODE` - Fichier de code
- `ICON_FA_FOLDER` - Dossier
- `ICON_FA_FOLDER_OPEN` - Dossier ouvert
- `ICON_FA_FLOPPY_DISK` - Sauvegarder

### Édition
- `ICON_FA_PEN` - Éditer
- `ICON_FA_PEN_TO_SQUARE` - Éditer dans un carré
- `ICON_FA_COPY` - Copier
- `ICON_FA_SCISSORS` - Couper
- `ICON_FA_CLIPBOARD` - Presse-papier
- `ICON_FA_TRASH` / `ICON_FA_TRASH_CAN` - Supprimer
- `ICON_FA_PLUS` - Ajouter
- `ICON_FA_MINUS` - Retirer
- `ICON_FA_XMARK` - Fermer

### Undo/Redo
- `ICON_FA_ROTATE_LEFT` - Annuler
- `ICON_FA_ROTATE_RIGHT` - Refaire

### Vue
- `ICON_FA_EYE` - Voir
- `ICON_FA_EYE_SLASH` - Cacher
- `ICON_FA_MAGNIFYING_GLASS` - Recherche/Zoom
- `ICON_FA_EXPAND` - Agrandir

### Blueprint/Graphes
- `ICON_FA_DIAGRAM_PROJECT` - Diagramme de projet
- `ICON_FA_SITEMAP` - Plan du site
- `ICON_FA_NETWORK_WIRED` - Réseau câblé
- `ICON_FA_CODE_BRANCH` - Branche de code
- `ICON_FA_CODE` - Code
- `ICON_FA_CUBE` - Cube (composant)
- `ICON_FA_CUBES` - Cubes multiples

### IA/Behavior Tree
- `ICON_FA_BRAIN` - Cerveau (IA)
- `ICON_FA_ROBOT` - Robot
- `ICON_FA_MICROCHIP` - Microprocesseur
- `ICON_FA_CIRCLE_NODES` - Nœuds circulaires

### Debug/Contrôles
- `ICON_FA_PLAY` - Lecture
- `ICON_FA_PAUSE` - Pause
- `ICON_FA_STOP` - Stop
- `ICON_FA_BUG` - Bug/Debug
- `ICON_FA_GAUGE` - Jauge/Profiler

### Paramètres
- `ICON_FA_GEAR` - Engrenage (paramètres)
- `ICON_FA_GEARS` - Engrenages multiples
- `ICON_FA_WRENCH` - Clé à molette
- `ICON_FA_SLIDERS` - Curseurs
- `ICON_FA_TOOLBOX` - Boîte à outils

### Aide/Information
- `ICON_FA_CIRCLE_INFO` - Information
- `ICON_FA_CIRCLE_QUESTION` - Aide
- `ICON_FA_BOOK` - Documentation
- `ICON_FA_LIGHTBULB` - Ampoule (idée)

### Status
- `ICON_FA_CHECK` - Validation
- `ICON_FA_CIRCLE_CHECK` - Validation (cercle)
- `ICON_FA_TRIANGLE_EXCLAMATION` - Attention
- `ICON_FA_CIRCLE_EXCLAMATION` - Exclamation
- `ICON_FA_CIRCLE_XMARK` - Erreur

### UI
- `ICON_FA_BARS` - Menu hamburger
- `ICON_FA_LIST` - Liste
- `ICON_FA_TABLE` - Tableau
- `ICON_FA_WINDOW_MAXIMIZE` - Maximiser
- `ICON_FA_WINDOW_MINIMIZE` - Minimiser
- `ICON_FA_WINDOW_RESTORE` - Restaurer

### Autres
- `ICON_FA_STAR` - Étoile (favoris)
- `ICON_FA_BOOKMARK` - Signet
- `ICON_FA_TAG` / `ICON_FA_TAGS` - Étiquette(s)
- `ICON_FA_FILTER` - Filtre
- `ICON_FA_CLOCK` - Horloge
- `ICON_FA_CALENDAR` - Calendrier

## Exemples d'intégration

### Exemple 1 : Menu contextuel
```cpp
if (ImGui::BeginPopupContextItem()) {
    if (ImGui::MenuItem(ICON_FA_PEN " Edit")) {
        EditItem();
    }
    if (ImGui::MenuItem(ICON_FA_COPY " Duplicate")) {
        DuplicateItem();
    }
    if (ImGui::MenuItem(ICON_FA_TRASH " Delete")) {
        DeleteItem();
    }
    ImGui::EndPopup();
}
```

### Exemple 2 : Toolbar
```cpp
if (ImGui::Button(ICON_FA_PLAY)) {
    StartSimulation();
}
ImGui::SameLine();
if (ImGui::Button(ICON_FA_PAUSE)) {
    PauseSimulation();
}
ImGui::SameLine();
if (ImGui::Button(ICON_FA_STOP)) {
    StopSimulation();
}
```

### Exemple 3 : Status avec icône
```cpp
ImGui::Text(ICON_FA_CHECK " Blueprint valid");
ImGui::TextColored(ImVec4(1,0.7f,0,1), ICON_FA_TRIANGLE_EXCLAMATION " 3 warnings");
ImGui::TextColored(ImVec4(1,0,0,1), ICON_FA_CIRCLE_EXCLAMATION " 1 error");
```

### Exemple 4 : Types de nodes
```cpp
// Dans un système de type de nodes
const char* GetNodeIcon(NodeType type) {
    switch(type) {
        case NodeType::Event: return ICON_FA_PLAY;
        case NodeType::Action: return ICON_FA_GEAR;
        case NodeType::Condition: return ICON_FA_CIRCLE_QUESTION;
        case NodeType::Decorator: return ICON_FA_STAR;
        case NodeType::Variable: return ICON_FA_CUBE;
        default: return ICON_FA_CIRCLE;
    }
}
```

## Personnalisation

### Changer la taille de police
```cpp
// Dans BlueprintEditorGUI::Initialize()
FontManager::Get().LoadFontAwesome("Assets/Fonts/fa-solid-900.otf", 20.0f); // Plus grande
```

### Utiliser plusieurs polices
```cpp
FontManager& fontMgr = FontManager::Get();
ImFont* iconFont = fontMgr.GetFontAwesome();
ImFont* defaultFont = fontMgr.GetDefaultFont();

// Utiliser la police d'icônes
ImGui::PushFont(iconFont);
ImGui::Text(ICON_FA_STAR);
ImGui::PopFont();
```

## Notes importantes

1. **Encodage UTF-8** : Les icônes sont des caractères Unicode encodés en UTF-8. Assurez-vous que vos fichiers sources sont en UTF-8.

2. **Police requise** : Les icônes ne s'afficheront pas si la police Font Awesome n'est pas chargée. Vérifiez les logs :
   ```
   [FontManager] Font Awesome loaded successfully from: Assets/Fonts/fa-solid-900.otf
   ```

3. **Espacement** : Ajoutez toujours un espace après l'icône pour la lisibilité :
   ```cpp
   ICON_FA_FILE " Save"  // ✓ Bon
   ICON_FA_FILE "Save"   // ✗ Mauvais (pas d'espace)
   ```

4. **Performance** : Les icônes sont des caractères de texte standard, elles n'ont pas d'impact significatif sur les performances.

## Ajouter de nouvelles icônes

Pour ajouter des icônes non listées dans `IconsFontAwesome6.h` :

1. Trouvez le code Unicode de l'icône sur https://fontawesome.com/icons
2. Convertissez le code en UTF-8 (ex: U+F015 → `\xef\x80\x95`)
3. Ajoutez la définition dans `Source/Core/IconsFontAwesome6.h` :
   ```cpp
   #define ICON_FA_NOUVELLE_ICONE "\xef\x80\x95"
   ```

## Ressources

- **Site officiel** : https://fontawesome.com
- **Documentation des icônes** : https://fontawesome.com/icons
- **License** : SIL OFL 1.1 (gratuit pour usage commercial)
- **GitHub** : https://github.com/FortAwesome/Font-Awesome

## Support

Pour toute question ou problème d'affichage des icônes, vérifiez :
1. Que la police est bien présente dans `Assets/Fonts/fa-solid-900.otf`
2. Que les logs confirment le chargement de la police
3. Que votre fichier source est encodé en UTF-8
4. Que vous avez inclus `IconsFontAwesome6.h`
