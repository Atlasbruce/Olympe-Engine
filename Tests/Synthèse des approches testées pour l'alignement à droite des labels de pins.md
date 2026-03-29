# 📋 Synthèse des approches testées pour l'alignement à droite des labels de pins

## Contexte
Tentative d'aligner dynamiquement les labels des pins output (à droite) des nodes dans l'éditeur visual script (ImGui + ImNodes), de manière à ce que les labels restent toujours près des pins même quand le node change de taille.

---

## 1. SetCursorPosX() avec CalcTextSize()

**Approche :** Calculer la largeur du texte avec `ImGui::CalcTextSize()`, puis utiliser `ImGui::SetCursorPosX()` pour repositionner le curseur juste avant le pin.

```cpp
ImVec2 labelSize = ImGui::CalcTextSize(label.c_str());
float availSpace = ImGui::GetContentRegionAvail().x;
float targetCursorX = ImGui::GetCursorPosX() + std::max(0.0f, availSpace - labelSize.x - 25.0f);
ImGui::SetCursorPosX(targetCursorX);
ImGui::Text("%s", label.c_str());
```

**Résultat :** ❌ **FAILURE** - A complètement cassé le rendu du node avec ImNodes. Les pins se sont désalignés et le layout s'est effondré.

**Raison :** `ImGui::SetCursorPosX()` interfère avec le système de positionnement d'ImNodes à l'intérieur de `BeginOutputAttribute/EndOutputAttribute`. ImNodes gère son propre positionnement des pins et le curseur ImGui devient incohérent.

---

## 2. ImGui::Indent() / ImGui::Unindent()

**Approche :** Utiliser l'indentation pour pousser le texte à droite dynamiquement.

```cpp
float offsetX = std::max(0.0f, availWidth - textSize.x - 20.0f);
ImGui::Indent(offsetX);
ImGui::Text("%s", label.c_str());
ImGui::Unindent(offsetX);
```

**Résultat :** ❌ **FAILURE** - N'a pas d'effet visible, le texte reste à la position par défaut.

**Raison :** `Indent/Unindent` affecte le layout global mais pas le positionnement fin du texte. De plus, cela interfère aussi avec ImNodes.

---

## 3. ImGui::Dummy() + ImGui::SameLine()

**Approche :** Ajouter un invisible dummy widget puis pousser le texte à côté avec `SameLine()`.

```cpp
ImGui::Dummy(ImVec2(60.0f, 0.0f));
ImGui::SameLine(0.0f, 0.0f);
ImGui::Text("%s", label.c_str());
```

**Résultat :** ⚠️ **PARTIAL FAILURE** - Les labels se sont décalés mais :
- Offset **fixe** (60px) ne s'adapte pas à la largeur dynamique du node
- Quand le node change de taille (nom plus long), le padding ne change pas et les labels restent mal alignés
- Incohérent selon la taille du contenu du node

**Raison :** Padding statique ne peut pas s'adapter dynamiquement à la largeur du node qui change au runtime.

---

## 4. Padding string (espaces fixes)

**Approche :** Ajouter directement des espaces au début du string du label.

```cpp
std::string paddedLabel = "          " + label;
ImGui::Text("%s", paddedLabel.c_str());
```

**Résultat :** ⚠️ **PARTIAL FAILURE** - Même problème que Dummy()
- Padding fixe de 10 espaces ne s'adapte pas
- Casse quand le node est resizé avec un long nom
- Les espaces ne scalent pas proportionnellement à la fonte

**Raison :** Impossible de calculer dynamiquement le nombre d'espaces nécessaires car dépend de la largeur de la police, largeur du node, et contenu.

---

## 5. ImGui::Columns() → ImGui::Table() avec ImGuiTableColumnFlags_RightAligned

**Approche :** Remplacer `Columns()` (deprecated) par `ImGui::BeginTable()` avec flag `ImGuiTableColumnFlags_RightAligned` pour aligner la colonne droite.

```cpp
ImGui::BeginTable("node_pins_table", 2, ImGuiTableFlags_None);
ImGui::TableSetupColumn("InputPins", ImGuiTableColumnFlags_WidthFixed, 80.0f);
ImGui::TableSetupColumn("OutputPins", ImGuiTableColumnFlags_RightAligned);
```

**Résultat :** ❌ **FLAG DOESN'T EXIST** - Erreur de compilation

```
E0135: espace de noms "ImGui" n'a pas de membre "ImGuiTableColumnFlags_RightAligned"
C2065: 'ImGuiTableColumnFlags_RightAligned' : identificateur non déclaré
```

**Raison :** Le flag `ImGuiTableColumnFlags_RightAligned` n'existe pas dans la version ImGui disponible (ou n'a jamais existé).

---

## 6. ImGui::TextRightAligned()

**Approche :** Utiliser une fonction helper ImGui pour alignement à droite.

```cpp
ImGui::TextRightAligned(label.c_str());
```

**Résultat :** ❌ **FUNCTION DOESN'T EXIST** - Erreur de compilation

```
C3861: 'TextRightAligned' : identificateur introuvable
C2039: 'TextRightAligned' n'est pas membre de 'ImGui'
```

**Raison :** Cette fonction n'existe pas dans ImGui. Il existe `ImGui::TextCentered()` et `ImGui::TextColored()` mais pas `TextRightAligned()`.

---

## 7. ImGui::Table avec ImGuiTableColumnFlags_WidthStretch + ImGui::Selectable()

**Approche :** Utiliser Tables avec stretch column width, puis utiliser `Selectable()` au lieu de `Text()` pour bénéficier d'alignement automatique.

```cpp
ImGui::TableSetupColumn("OutputPins", ImGuiTableColumnFlags_WidthStretch);
ImGui::SetNextItemWidth(-1.0f);
ImGui::Selectable(label.c_str(), false);
```

**Résultat :** ⚠️ **PARTIAL SUCCESS but BROKEN LAYOUT** - 
- Table a pris une **largeur infinie**
- Tous les nodes avec la table se sont étendus infiniment
- Les nodes sans table (comme Branch avec Columns) restaient normaux
- Donc créait une incohérence visuelle majeure

**Raison :** `ImGui::BeginTable()` crée son propre contexte de sizing qui ne se limite pas à la taille du node ImNodes. Sans contrainte de largeur explicite (`ImVec2` avec max width), elle expande sans limite.

---

## 8. ImGui::PushTextWrapPos() / ImGui::PopTextWrapPos()

**Approche :** Utiliser `PushTextWrapPos()` pour forcer le wrapping et alignement.

```cpp
ImGui::PushTextWrapPos(availWidth);
ImGui::Text("%s", label.c_str());
ImGui::PopTextWrapPos();
```

**Résultat :** ❌ **NO EFFECT** - Le texte reste non aligné

**Raison :** `PushTextWrapPos()` contrôle le wrapping (breaking ligne), pas l'alignement horizontal. N'affecte pas la justification left/right/center.

---

## 🎯 Problèmes fondamentaux identifiés

1. **ImNodes + ImGui mismatch** : ImNodes gère son propre système de positionnement des pins qui est incompatible avec les tentatives de repositionnement ImGui manuel

2. **Colonnes statiques** : `ImGui::Columns()` utilise des largeurs fixes qui ne s'adaptent pas à la largeur dynamique du node

3. **Tables sans contrainte** : `ImGui::Table()` n'est pas limité à la taille du node et expande infiniment

4. **Pas de true right-align dans ImGui** : ImGui n'a pas de primitive native pour aligner du texte à droite dans une région

5. **Contexte nested** : `BeginOutputAttribute` crée un contexte ImNodes qui ignore les commandes ImGui de positionnement

---

## 💡 Pistes possibles à explorer

### 1. **Dessiner directement avec ImDrawList**
Bypass ImGui/ImNodes complètement et utiliser `ImGui::GetWindowDrawList()` pour dessiner le texte manuellement à la position correcte.

**Avantages :**
- Contrôle total du rendu
- Pas d'interférence avec ImNodes
- Possibilité de calculer précisément la position

**Inconvénients :**
- Complexité accrue
- Besoin de calculer les positions manuellement
- Maintenance difficile

### 2. **Hooks ou pré-traitement personnalisé**
Modifier le pipeline de rendu d'ImNodes lui-même ou intercepter le rendu avant qu'il ne se fasse.

### 3. **RightToLeft text direction**
Peut-être que forcer RTL rendrait le texte inversé de manière utile ?

### 4. **Custom ImGui backend**
Créer des widgets personnalisés qui respectent mieux ImNodes.

### 5. **Constraint table width explicitement**
Passer `ImVec2(node_width, 0)` dans `BeginTable()` pour limiter la largeur plutôt que de laisser expander infiniment.

**Avantages :**
- Peut résoudre le problème de table infinie
- Utilise l'API ImGui standard

**Inconvénients :**
- Faudrait connaître la largeur du node en temps réel
- Complexité de synchroniser avec ImNodes

### 6. **Utiliser des tabs au lieu de colonnes/tables**
`ImGui::BeginTabBar()` avec alignement personnalisé ?

### 7. **Shader ou canvas externe**
Accepter que ce soit impossible avec ImGui/ImNodes et utiliser un système de rendu parallèle.

### 8. **Approche Right-Padding dynamique**
Plutôt que de tenter un vrai right-align, calculer dynamiquement le padding d'espaces basé sur la largeur disponible **une seule fois au chargement du node**, puis accepter que ce soit statique pour ce node.

---

## 📊 Tableau résumé

| Approche | Résultat | Raison de l'échec |
|----------|----------|-------------------|
| SetCursorPosX() | ❌ Crash layout | Interfère avec ImNodes |
| Indent/Unindent | ❌ Pas d'effet | N'affecte pas le positionnement fin |
| Dummy+SameLine | ⚠️ Padding fixe | Ne s'adapte pas dynamiquement |
| Padding string | ⚠️ Padding fixe | Dépend de trop de facteurs |
| Table+RightAligned | ❌ Flag inexistant | Flag n'existe pas dans ImGui |
| TextRightAligned | ❌ Fonction inexistante | Fonction n'existe pas dans ImGui |
| Table+Selectable | ⚠️ Largeur infinie | Table ne se limite pas à node |
| PushTextWrapPos | ❌ Pas d'effet | Affecte wrapping, pas alignement |

---

## 🚀 Recommandations

- **Court terme** : Accepter l'alignement left actuel ou utiliser une solution de padding dynamique limité
- **Moyen terme** : Explorer l'approche ImDrawList pour un rendu custom
- **Long terme** : Envisager une refonte du système de rendu des nodes si l'alignement à droite est critique pour l'UX

---

**Dernière mise à jour :** Session actuelle  
**Status :** Problème non résolu - en attente d'exploration supplémentaire
