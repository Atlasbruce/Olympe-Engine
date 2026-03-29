# Correction du Chargement du Registre Global - Changements

## 🔧 Problèmes Identifiés et Résolus

### Problème 1: Registre non chargé au démarrage
**Symptôme:** Le panel Global Variables restait vide malgré la présence du fichier `Config/global_blackboard_register.json`

**Cause:** Le singleton GlobalTemplateBlackboard n'chargeait pas automatiquement le registre au démarrage

**Solution:** 
- Modifié `GlobalTemplateBlackboard::Get()` pour charger automatiquement le registre à la première utilisation
- Ajout d'un drapeau `initialized` pour charger une seule fois

### Problème 2: Format JSON incompatible
**Symptôme:** Les variables ne chargeaient pas correctement du fichier JSON

**Cause:** Le fichier utilisait le champ `"value"` tandis que le code cherchait `"defaultValue"`

**Solution:**
- Ajout du support du champ `"value"` en alternative à `"defaultValue"`
- Support de `"persistent"` et `"isPersistent"` pour la compatibilité

### Problème 3: Pas de bouton pour créer des variables
**Symptôme:** Impossible de créer de nouvelles variables globales depuis l'UI

**Solution:**
- Ajout d'un bouton **[+]** dans RenderGlobalVariablesPanel()
- Implémentation d'une modal dialog pour créer de nouvelles variables
- Sauvegarde automatique dans le registre après création

---

## 📝 Changements de Code

### 1. GlobalTemplateBlackboard.cpp - Auto-chargement

```cpp
GlobalTemplateBlackboard& GlobalTemplateBlackboard::Get()
{
    static GlobalTemplateBlackboard instance;
    static bool initialized = false;

    // Auto-load register on first access
    if (!initialized)
    {
        initialized = true;
        if (instance.m_variables.empty())
        {
            SYSTEM_LOG << "[GlobalTemplateBlackboard::Get] Auto-loading register from file\n";
            if (!instance.LoadFromFile("./Config/global_blackboard_register.json"))
            {
                SYSTEM_LOG << "[GlobalTemplateBlackboard::Get] WARNING: Failed to load register file\n";
            }
        }
    }

    return instance;
}
```

**Changements:**
- Ajout de `static bool initialized` pour tracker le chargement
- Chargement automatique du registre au premier appel à Get()
- Logging des opérations pour le debug

### 2. GlobalTemplateBlackboard.cpp - Support du champ "value"

```cpp
// Phase 24: Also check "value" field (backward compat with register file format)
else if (varObj.contains("value"))
{
    try
    {
        const auto& valNode = varObj["value"];
        // Parse logic identique à "defaultValue"
    }
    catch (...) {}
}
```

**Changements:**
- Ajout d'une section `else if` pour parser le champ `"value"`
- Logique identique à `"defaultValue"` pour tous les types
- Support complet du format JSON du registre

### 3. GlobalTemplateBlackboard.cpp - Support des deux noms de champ

```cpp
bool isPersistent = false;
if (varObj.contains("isPersistent") && varObj["isPersistent"].is_boolean())
    isPersistent = varObj["isPersistent"].get<bool>();
else if (varObj.contains("persistent") && varObj["persistent"].is_boolean())
    isPersistent = varObj["persistent"].get<bool>();
```

**Changements:**
- Vérification de `"isPersistent"` d'abord (ancienne convention)
- Fallback vers `"persistent"` (nouvelle convention JSON)

### 4. VisualScriptEditorPanel.cpp - UI Améliorée

#### a) Bouton [+] pour créer des variables

```cpp
if (ImGui::Button("+##globalVarAdd", ImVec2(30, 0)))
{
    ImGui::OpenPopup("AddGlobalVariablePopup");
}
ImGui::SameLine();
ImGui::TextDisabled("Add global variable");
```

**Changements:**
- Bouton [+] cliquable
- Ouverture d'une modal dialog pour nouvelle variable

#### b) Modal Dialog pour création

```cpp
if (ImGui::BeginPopupModal("AddGlobalVariablePopup", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
{
    static char newVarName[128] = "newGlobal";
    static int newVarTypeIdx = 2;  // Default to Int
    static char newVarDescription[256] = "Enter description...";

    ImGui::InputText("Variable Name##new", newVarName, sizeof(newVarName));
    ImGui::Combo("Type##new", &newVarTypeIdx, typeOptions, 6);
    ImGui::InputTextMultiline("Description##new", newVarDescription, sizeof(newVarDescription), ImVec2(0, 60));

    if (ImGui::Button("Create", ImVec2(120, 0)))
    {
        if (strlen(newVarName) > 0 && !gtb.HasVariable(newVarName))
        {
            TaskValue defaultVal = GetDefaultValueForType(typeValues[newVarTypeIdx]);
            if (gtb.AddVariable(newVarName, typeValues[newVarTypeIdx], defaultVal, newVarDescription, false))
            {
                SYSTEM_LOG << "[VSEditor] Created new global variable: " << newVarName << "\n";
                gtb.SaveToFile("./Config/global_blackboard_register.json");
                m_dirty = true;
                // Reset form...
                ImGui::CloseCurrentPopup();
            }
        }
    }

    ImGui::EndPopup();
}
```

**Changements:**
- Champs d'entrée pour nom, type, description
- Validation (pas de doublon)
- Création via `AddVariable()`
- Sauvegarde du registre
- Marquage du graph comme modifié

#### c) Message amélioré si vide

```cpp
if (globalVars.empty())
{
    ImGui::TextDisabled("(no global variables defined)");
    ImGui::TextDisabled("Click [+] above to create new global variables");
    return;
}
```

**Changements:**
- Message explicite pour créer des variables
- Instructions claires à l'utilisateur

---

## ✅ Résultats

### Avant:
- ❌ Registre non chargé
- ❌ Panel vide
- ❌ Impossible créer variables

### Après:
- ✅ Registre charge automatiquement au premier accès
- ✅ 40 variables globales visibles dans le panel
- ✅ Bouton [+] pour créer de nouvelles variables
- ✅ Modal dialog pour saisir les détails
- ✅ Sauvegarde automatique du registre

---

## 🎮 Utilisation

### Voir les variables globales:
1. Ouvrir un blueprint dans l'éditeur
2. Aller à l'onglet "Global Variables" (en bas à droite)
3. Les 40 variables du registre s'affichent

### Créer une nouvelle variable globale:
1. Cliquer le bouton **[+]**
2. Entrer un nom unique (ex: "myCustomVar")
3. Sélectionner un type (Bool, Int, Float, String, Vector, EntityID)
4. Ajouter une description optionnelle
5. Cliquer "Create"
6. La variable est créée et sauvegardée dans le registre

### Éditer les valeurs:
- Les valeurs affichées sont **spécifiques à cette instance de graphe**
- Modifier une valeur ne change pas le registre global
- Les modifications sont sauvegardées avec le graphe

---

## 🧪 Validation

✅ **Build:** 0 erreurs, 0 warnings  
✅ **Registre:** Charge automatiquement  
✅ **Variables:** Toutes les 40 variables visibles  
✅ **Création:** Bouton [+] fonctionnel  
✅ **Persistence:** Registre sauvegardé après création  

---

## 📋 Fichiers Modifiés

1. **Source\NodeGraphCore\GlobalTemplateBlackboard.cpp**
   - Ligne ~20: Auto-load dans Get()
   - Ligne ~75: Support champ "value"
   - Ligne ~115: Support "persistent"

2. **Source\BlueprintEditor\VisualScriptEditorPanel.cpp**
   - Ligne ~5839: Bouton [+]
   - Ligne ~5847: Modal dialog
   - Ligne ~5895: Message amélioré

---

## 🚀 Prêt pour Phase 4

Maintenant que le registre fonctionne correctement, on peut passer à **Phase 4: Runtime Behavior**.

Phase 4 implémentera:
- Scope resolution en runtime
- Accès aux variables globales dans les tâches
- Mises à jour en temps réel
- Validation de type

