# Switch Case Editor Modal - UX Improvements

## Executive Summary

Le modal actuel fonctionne techniquement, mais manque de **contexte et de validation** pour guider l'utilisateur. Les améliorations proposées réduiront les erreurs et amélioreront la clarté.

---

## 🎯 Problèmes UX Identifiés

### 1. **Ambiguïté des Champs**
**Problème**: L'utilisateur ne sait pas clairement quel champ remplit quel rôle.
- Quel est le champ "Value" ? (comparaison numérique ? nom ?)
- Quel est le champ "Label" ? (affichage ? ou autre ?)
- Pourquoi deux champs ?

**Votre cas d'étude**: Vous aviez inversé value/label parce que ce n'était pas clair.

### 2. **Pas de Contexte sur la Variable de Comparaison**
**Problème**: Le modal n'affiche pas sur quoi on fait la comparaison.
- Vous switchez sur "mHealth" (de 0-256)
- Mais le modal n'affiche nulle part "Vous switchez sur: mHealth"
- Comment l'utilisateur sait qu'il doit entrer 0, 1, 2, 3 et pas "Idle", "Probe" ?

### 3. **Pas de Validation en Temps Réel**
**Problème**: Les erreurs ne sont détectées qu'au chargement du graphe.
- Valeurs dupliquées? Pas de warning.
- Valeurs vides? Pas de warning.
- Format invalide? Pas de warning.

### 4. **Pin Names Auto-Générés Masqués**
**Problème**: L'utilisateur ne voit pas les noms de pins générés (Case_0, Case_1, etc.)
- Les pins sont générés automatiquement mais on ne les affiche pas
- Si l'utilisateur édite manuellement le JSON, il ne sait pas quoi mettre dans "pin"
- Provoque des déconnexions de graphe si l'utilisateur les change

### 5. **"Add Case" Crée des Valeurs Par Défaut Confuses**
**Problème**: Ajouter un cas avec "NewCase" comme valeur crée de la confusion.
- Devrait-on utiliser "NewCase" comme comparaison ? Non !
- L'utilisateur doit éditer immédiatement

### 6. **Pas d'Aide Contextuelle**
**Problème**: Pas de tooltip, pas de hint, pas d'exemple.
- Comment formater la valeur ?
- Puis-je utiliser des nombres ? Des strings ?
- Quelles sont les règles de validation ?

---

## 💡 Recommandations UX

### **Recommandation 1: En-têtes Explicites avec Icônes**

**Avant**:
```
[Index] | [Value Field] | [Label Field] | [Actions]
```

**Après**:
```
[#] | [⚙️ Match Value (for "mHealth")] | [👁️ Display Name (optional)] | [Actions]
```

**Bénéfice**: Contexte immédiat sur quoi matcher et quoi afficher.

---

### **Recommandation 2: Afficher le Context Variable**

Ajouter une **barre d'en-tête** avant la liste:
```
┌─────────────────────────────────────────┐
│ Switching on: mHealth (Integer)         │
│ Current Value: 10 (Health)              │
│ Possible Matches: 0-255                 │
└─────────────────────────────────────────┘
```

**Code ImGui**:
```cpp
ImGui::BeginChild("SwitchContext");
ImGui::TextColored(ImVec4(0.2f, 0.8f, 1.0f, 1.0f), 
    "Switching on: %s (%s)", switchVarName.c_str(), switchVarType.c_str());

if (!currentValue.empty())
    ImGui::TextDisabled("Current Value: %s", currentValue.c_str());

ImGui::TextDisabled("Add cases for each value to match.");
ImGui::EndChild();
```

**Bénéfice**: L'utilisateur sait immédiatement sur quoi il travaille.

---

### **Recommandation 3: Colorer les Colonnes par Fonction**

```cpp
// Column 1: Index (gris, lecture seule)
ImGui::TextDisabled("#%zu", caseIndex);

// Column 2: Match Value (cyan, critique)
ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.1f, 0.3f, 0.5f, 1.0f)); // Bleu
ImGui::InputText("##value", valueBuf, ...);
ImGui::PopStyleColor();

// Column 3: Display Label (vert clair, optionnel)
ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.1f, 0.4f, 0.2f, 1.0f)); // Vert
ImGui::InputText("##label", labelBuf, ...);
ImGui::PopStyleColor();

// Column 4: Pin Name (gris, lecture seule, copié)
ImGui::TextDisabled("Case_%zu", caseIndex);
ImGui::SameLine();
if (ImGui::SmallButton("Copy##pin")) 
    ImGui::SetClipboardText(pinName.c_str());
```

**Avantages**:
- Distinction visuelle immédiate
- Pin name visible pour référence JSON
- Bouton Copy pour faciliter édition manuelle

---

### **Recommandation 4: Validation en Temps Réel avec Indicateurs**

```cpp
// Dans RenderCaseRow:
bool RenderCaseRow(size_t caseIndex, const std::string& switchVarName)
{
    bool modified = false;

    // ... input fields ...

    // Validation checks
    ImGui::SameLine();

    // Check 1: Value empty?
    if (m_caseValueBuffers[caseIndex].empty())
    {
        ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), "⚠️");
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Match value cannot be empty");
    }

    // Check 2: Value duplicate?
    bool isDuplicate = false;
    for (size_t j = 0; j < m_editingCases.size(); ++j)
    {
        if (j != caseIndex && 
            m_caseValueBuffers[j] == m_caseValueBuffers[caseIndex] &&
            !m_caseValueBuffers[caseIndex].empty())
        {
            isDuplicate = true;
            break;
        }
    }
    if (isDuplicate)
    {
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "❌");
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Duplicate value - case %zu already uses this", someIndex);
    }

    // Check 3: Label empty (just warning, not error)
    if (m_caseLabelBuffers[caseIndex].empty())
    {
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "ℹ️");
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Display name is empty - will show pin name on canvas");
    }

    return modified;
}
```

**Indicateurs**:
- 🟠 **⚠️** = Avertissement (peut fonctionner mais pas idéal)
- 🔴 **❌** = Erreur (empêcher Apply si présent)
- 🔵 **ℹ️** = Info (juste pour info)

---

### **Recommandation 5: "Add Case" Intelligent**

**Avant**:
```cpp
newCase.value = "NewCase";  // ❌ Confus
newCase.pinName = "Case_" + std::to_string(m_editingCases.size());
newCase.customLabel = "";
```

**Après**:
```cpp
// Suggérer la prochaine valeur basée sur le contexte
SwitchCaseDefinition newCase;

// Si c'est un switch numérique, suggérer le prochain numéro
if (allCasesAreNumeric)
{
    int nextNum = FindHighestNumericValue() + 1;
    newCase.value = std::to_string(nextNum);  // "4" au lieu de "NewCase"
    newCase.customLabel = "State_" + std::to_string(nextNum);  // Suggestion
}
else
{
    newCase.value = "";  // Vide pour que l'utilisateur le remplisse
    newCase.customLabel = "";
}

newCase.pinName = "Case_" + std::to_string(m_editingCases.size());
m_editingCases.push_back(newCase);
m_caseValueBuffers.push_back(newCase.value);
m_caseLabelBuffers.push_back(newCase.customLabel);

// Focus automatically sur le champ value du nouveau cas
ImGui::SetKeyboardFocusHere(-1);
```

**Bénéfice**: Réduit les efforts de l'utilisateur, crée des patterns cohérents.

---

### **Recommandation 6: Tooltips et Help Text**

```cpp
ImGui::SameLine();
ImGui::TextDisabled("(?)");
if (ImGui::IsItemHovered())
{
    ImGui::BeginTooltip();
    ImGui::Text("Match Value:\n");
    ImGui::Bullet(); ImGui::Text("Must be numeric (0, 1, 2...)");
    ImGui::Bullet(); ImGui::Text("No duplicates allowed");
    ImGui::Bullet(); ImGui::Text("Examples: 0, 1, 10, 256\n");
    ImGui::Spacing();
    ImGui::Text("Display Name (optional):\n");
    ImGui::Bullet(); ImGui::Text("Human-readable label");
    ImGui::Bullet(); ImGui::Text("Shows on canvas pins");
    ImGui::Bullet(); ImGui::Text("Example: Idle, Patrol, Combat");
    ImGui::EndTooltip();
}
```

---

### **Recommandation 7: Mode "Compact" vs "Detailed"**

**Ajout d'un Toggle**:
```cpp
static bool detailedMode = false;
ImGui::Checkbox("Detailed Mode", &detailedMode);
ImGui::SameLine();
ImGui::TextDisabled("(?) Show pin names and validation");
```

**Compact Mode** (défaut): Cache les détails techniques
**Detailed Mode**: Affiche noms de pins, validations, indices

---

### **Recommandation 8: Bouton "Test Match" (Futur)**

```cpp
ImGui::SameLine(ImGui::GetWindowWidth() - 250);
if (ImGui::Button("Test Match"))
{
    // Open small popup:
    // "Enter value to test: [input] [Check]"
    // Shows which cases would match this value
}
```

---

## 📊 Résumé des Changements

| Problème | Solution | Priorité |
|----------|----------|----------|
| Ambiguïté champs | En-têtes + couleurs + icônes | 🔴 Critique |
| Contexte manquant | Afficher switch variable/type/valeur | 🔴 Critique |
| Pas de validation | Indicateurs d'erreur en temps réel | 🟠 Haute |
| Pin names cachés | Afficher + bouton Copy | 🟠 Haute |
| Add Case confus | Auto-complétion intelligente | 🟡 Moyenne |
| Pas d'aide | Tooltips + Help text | 🟡 Moyenne |
| UX compacte | Toggle Detailed Mode | 🟢 Basse |

---

## 🚀 Phase d'Implémentation Recommandée

**Phase 26-A** (Immédiat - Haute Priorité):
1. En-têtes explicites avec contexte
2. Coloration des colonnes
3. Affichage du pin name + Copy button
4. Validation basique (erreur, pas Apply si erreur)

**Phase 26-B** (Futur - Moyenne Priorité):
1. Validation avancée avec indicateurs
2. Add Case intelligent
3. Tooltips contextuels
4. Mode Detailed

**Phase 26-C** (Futur - Basse Priorité):
1. Test Match feature
2. Import/Export cases
3. Templates prédéfinis

---

## Fichiers à Modifier

Pour implémenter Phase 26-A:
- `Source/Editor/Modals/SwitchCaseEditorModal.cpp`
  - `RenderCaseList()`: Ajouter en-tête contextuel
  - `RenderCaseRow()`: Couleurs + Pin name display + validation
  - `RenderActionButtons()`: Meilleur "Add Case"

- `Source/Editor/Modals/SwitchCaseEditorModal.h`
  - Ajouter param `switchVarName` et `switchVarType` à `Open()`

- `Source/BlueprintEditor/VisualScriptEditorPanel_Properties.cpp`
  - Passer `switchVarName` en appelant `m_switchCaseModal->Open()`

---

## Exemple Before/After

### BEFORE
```
┌──────────────────────────────────────────────────────────┐
│ Edit Switch Cases                       [X]              │
├──────────────────────────────────────────────────────────┤
│ # | Value  | Label   | Actions                          │
├────────────────────────────────────────────────────────────┤
│ 0 | [Idle] | [0]     | ^ v X                            │
│ 1 | [Probe]| [1]     | ^ v X                            │
│ 2 | [Walk] | [2]     | ^ v X                            │
│ 3 | [Run]  | [3]     | ^ v X                            │
├──────────────────────────────────────────────────────────┤
│  [Add Case]  [Apply]  [Cancel]                          │
└──────────────────────────────────────────────────────────┘
```

### AFTER
```
┌──────────────────────────────────────────────────────────┐
│ Edit Switch Cases                       [X]              │
├──────────────────────────────────────────────────────────┤
│ Switching on: mHealth (Integer)                          │
│ ℹ️ Add cases for each value to match                     │
├────────────────────────────────────────────────────────────┤
│ # │ ⚙️ Match Value  │ 👁️ Display Name │ Pin Name     │
├────────────────────────────────────────────────────────────┤
│ 0 │ [0      ]  ✓    │ [Idle      ]     │ Case_0  [Copy] │
│ 1 │ [1      ]  ✓    │ [Probe     ]     │ Case_1  [Copy] │
│ 2 │ [2      ]  ✓    │ [Walk      ]  ℹ️ │ Case_2  [Copy] │
│ 3 │ [3      ]  ✓    │ [Run       ]     │ Case_8  [Copy] │
├──────────────────────────────────────────────────────────┤
│  [Add Case] [Detailed Mode]  [Apply]  [Cancel]         │
└──────────────────────────────────────────────────────────┘
```

---

## Conclusion

Ces améliorations transformeront le modal de "techniquement fonctionnel" à "clairement intuitif". L'utilisateur comprendra immédiatement:

✅ **Quoi**: Je crée des cas de switch
✅ **Pourquoi**: Pour matcher différentes valeurs de mHealth
✅ **Comment**: Value = numéro à matcher, Label = nom pour affichage
✅ **Validation**: Les erreurs sont visibles immédiatement

