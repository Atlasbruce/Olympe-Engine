# 📋 Specification Fonctionnelle - Refonte Blackboard Dynamique Global Externe

**Document:** Refonte Blackboard Dynamique Global Externe  
**Date:** 2026-03-26  
**Version:** 1.0  
**Status:** 🟢 VALIDÉ - PRÊT POUR IMPLÉMENTATION  
**Auteur:** Équipe Engine  
**Scope:** Editor UI + Runtime System + Sérialisation  

---

## 📑 Table des Matières

1. [Vue d'ensemble](#vue-densemble)
2. [Architecture Système](#architecture-système)
3. [Spécifications Fonctionnelles](#spécifications-fonctionnelles)
4. [Interface Utilisateur](#interface-utilisateur)
5. [Sérialisation & Persistence](#sérialisation--persistence)
6. [Runtime Behavior](#runtime-behavior)
7. [Cas d'Usage & Exemples](#cas-dusage--exemples)
8. [Acceptance Criteria](#acceptance-criteria)

---

## 🎯 Vue d'Ensemble

### Objectif Principal

Refactoriser le système de variables blackboard en séparant complètement:
- **Variables LOCALES:** Spécifiques au graphe, par-instance
- **Variables GLOBALES:** Définies via un registre externe, partagées au niveau du projet, une structure commune pour toutes les entités

### Concept Clé

Les variables globales ne sont **PAS partagées** entre les entités (contrairement au premier design GlobalBlackboard singleton). À la place, elles constituent un **modèle de structure** (template) qui est appliqué individuellement à chaque instance d'entité avec des valeurs propres.

```
GlobalTemplateBlackboard Registre (Structure)
    ├─ Position (Vector, default: 0,0,0)
    ├─ Velocity (Vector, default: 0,0,0)
    ├─ Speed (Float, default: 0.0)
    └─ Health (Float, default: 100.0)

Entity Instance 1 Blackboard        Entity Instance 2 Blackboard
├─ Position: (100, 0, 0)            ├─ Position: (50, 0, 0)
├─ Velocity: (1, 0, 0)              ├─ Velocity: (-1, 0, 0)
├─ Speed: 5.0                       ├─ Speed: 3.0
└─ Health: 100.0                    └─ Health: 75.0
```

---

## 🏗️ Architecture Système

### 1. GlobalTemplateBlackboard Registry

**Localisation:** `./Config/global_blackboard_register.json`

**Responsabilités:**
- Stocker la **définition structurelle** des variables globales
- Être chargé au démarrage de l'application
- Être le point unique de vérité pour toutes les variables globales du projet
- Ne PAS contenir les valeurs d'instances (seulement defaults + overrides)

**Chargement:**
- Chargé UNE FOIS au boot via `GlobalTemplateBlackboard::Get().LoadFromFile()`
- Conservé en mémoire pendant toute la durée de l'application
- **FIGÉ** pendant l'exécution (pas de hot-reload à runtime)

### 2. LocalBlackboard (Existing - Refactorisé)

**Champ:** `m_localVariables` (existant - inchangé)

**Comportement:**
- Contient UNIQUEMENT les variables LOCAL (`IsGlobal = false`)
- Initialisé depuis `TaskGraphTemplate.Blackboard` (entries avec `IsGlobal = false`)
- Scope per-graph-execution
- Méthodes: `GetValue()`, `SetValue()`, `HasVariable()`

### 3. EntityBlackboard (NEW)

**Localisation:** À créer - tbd sur la structure existante

**Responsabilités:**
- Contient UNIQUEMENT les variables GLOBALES par-entity
- Initialisé depuis le registre GlobalTemplateBlackboard
- Valeurs spécifiques à chaque instance d'entité
- Peut être modifié à runtime (SetValue avec type-checking)

**Interface:**
```cpp
class EntityBlackboard {
public:
    // Initialize from template + override values
    void InitializeFromTemplate(
        const GlobalTemplateBlackboard& globalTemplate,
        const std::map<std::string, TaskValue>& overrides
    );

    TaskValue GetValue(const std::string& key) const;
    void SetValue(const std::string& key, const TaskValue& value);
    bool HasVariable(const std::string& key) const;

private:
    std::unordered_map<std::string, TaskValue> m_values;
    std::unordered_map<std::string, VariableType> m_types;
};
```

### 4. BBVariableRegistry (Existing - Refactorisé)

**Modifications:**
- Ajouter méthode `GetGlobalVariables()` → retourne uniquement les globales
- Ajouter méthode `GetLocalVariables()` → retourne uniquement les locales
- Ajouter support du prefix (L)/(G) dans `FormatDisplayLabel()`
- Gérer l'affichage des conflits de noms

---

## 📋 Spécifications Fonctionnelles

### 1. Registre Externe - Format JSON

**Fichier:** `./Config/global_blackboard_register.json`

**Structure:**
```json
{
  "version": "1.0",
  "description": "Global Template Blackboard - Defines shared variable structure across all entities",
  "variables": [
    {
      "key": "Position",
      "type": "Vector",
      "defaultValue": {
        "x": 0.0,
        "y": 0.0,
        "z": 0.0
      },
      "description": "Entity world position"
    },
    {
      "key": "Velocity",
      "type": "Vector",
      "defaultValue": {
        "x": 0.0,
        "y": 0.0,
        "z": 0.0
      },
      "description": "Entity velocity vector"
    },
    {
      "key": "Speed",
      "type": "Float",
      "defaultValue": 0.0,
      "description": "Entity movement speed"
    },
    {
      "key": "Health",
      "type": "Float",
      "defaultValue": 100.0,
      "description": "Entity health points"
    },
    {
      "key": "IsAlive",
      "type": "Bool",
      "defaultValue": true,
      "description": "Entity alive state"
    }
  ]
}
```

### 2. Template Blackboard Structure (Existant - Inchangé)

**Fichier:** `Blueprint.ats` (JSON v4 schema)

**Structure - LOCALES uniquement:**
```json
{
  "schema_version": 4,
  "name": "AI_Combat",
  "blackboard": [
    {
      "key": "CurrentTarget",
      "type": "EntityID",
      "value": "0",
      "isGlobal": false
    },
    {
      "key": "DistanceToPlayer",
      "type": "Float",
      "value": "0.0",
      "isGlobal": false
    }
  ],
  "globalVariableOverrides": [
    {
      "key": "Health",
      "value": 150.0
    },
    {
      "key": "Speed",
      "value": 7.5
    }
  ]
}
```

**Comportement Sérialisation:**
- Seules les entrées `blackboard` avec `isGlobal: false` sont stockées
- Les variables globales ne sont **PAS** stockées dans le blueprint
- UNIQUEMENT les **overrides** de variables globales (si valeur != default)
- Sérialisation optimisée: ne sauvegarder que les différences

### 3. Panel UI - Refactorisé

**Nouvelle structure:** Right Panel avec Tabs (Global / Local)

```
┌─────────────────────────────────────────────┐
│ Blueprint Editor - Right Panel              │
├─────────────────────────────────────────────┤
│                                             │
│ Node Properties                             │
│ ─────────────────────                       │
│ [Node properties...]                        │
│                                             │
│ ═════════════════════════════════════════════│ Splitter 1
│                                             │
│ ┌─ [GLOBAL] [LOCAL] ────────────────────┐  │
│ │ Tabbed interface                       │  │
│ │                                        │  │
│ │ TAB: GLOBAL (default selected)       │  │
│ │ ─────────────────────────────────────  │  │
│ │ [+] Add global variable                │  │
│ │ [!] Loaded from: ./Config/...register │  │
│ │                                        │  │
│ │ ┌────────────────────────────────────┐ │  │
│ │ │ Name    │ Type   │ Default │  X    │ │  │
│ │ ├────────────────────────────────────┤ │  │
│ │ │ Position│ Vector │ 0,0,0   │ [DEL] │ │  │
│ │ │ Velocity│ Vector │ 0,0,0   │ [DEL] │ │  │
│ │ │ Speed   │ Float  │ 0.0     │ [DEL] │ │  │
│ │ │ Health  │ Float  │ 100.0   │ [DEL] │ │  │
│ │ └────────────────────────────────────┘ │  │
│ │                                        │  │
│ │ TAB: LOCAL                            │  │
│ │ ─────────────────────────────────────  │  │
│ │ [+] Add local variable                 │  │
│ │                                        │  │
│ │ ┌────────────────────────────────────┐ │  │
│ │ │ Name      │ Type  │ Default │  X   │ │  │
│ │ ├────────────────────────────────────┤ │  │
│ │ │ CurTarget │Entity │ 0       │ [DEL]│ │  │
│ │ │ PatrolIdx │ Int   │ 0       │ [DEL]│ │  │
│ │ └────────────────────────────────────┘ │  │
│ └────────────────────────────────────────┘  │
│                                             │
└─────────────────────────────────────────────┘
```

### 4. Prefixes pour Identification

**Convention:** (L) pour Local, (G) pour Global

**Affichage dans Dropdowns:**
```
GetBBValue / SetBBValue Selection:
├─ (L) CurrentTarget
├─ (L) PatrolIndex
├─ (L) DistanceToPlayer
├─ (G) Position
├─ (G) Velocity
├─ (G) Speed
└─ (G) Health
```

**Cas de Conflit de Noms:**
```
Si variable "Health" existe dans Local ET Global:
├─ (L) Health    ← Local variable
└─ (G) Health    ← Global variable
```

---

## 🎨 Interface Utilisateur

### 1. Global Variables Tab

**Comportement:**
- Affiché par défaut (TAB sélectionné au premier accès)
- Affiche la liste des variables chargées depuis le registre
- Info: "Loaded from: `./Config/global_blackboard_register.json`"

**Operations:**
- [+] Add Global Variable (NOUVEAU)
- Edit name / type / default value
- [x] Delete variable

**Add Global Variable Dialog:**
```
┌─ Add Global Variable ──────────────────┐
│                                        │
│ Variable Name: [________________]      │
│ Variable Type: [Dropdown: Types]       │
│ Default Value: [________________]      │
│                                        │
│ [!] Errors:                           │
│ - Name cannot be empty                │
│ - Name must be unique (no duplicates)  │
│ - Type mismatch with existing variable│
│                                        │
│ [Create]  [Cancel]                    │
└────────────────────────────────────────┘
```

**Validation:**
- ✅ Name non-vide
- ✅ Name unique (pas de duplicate même scope)
- ✅ Type valide (Bool, Int, Float, String, Vector, EntityID)
- ✅ Pas d'override name déjà existant dans blueprint

**Action Create:**
1. Valider l'entrée
2. Ajouter à GlobalTemplateBlackboard en mémoire
3. Sauvegarder dans `./Config/global_blackboard_register.json`
4. Rafraîchir la UI (ajouter la ligne dans le tableau)
5. Marquer le blueprint comme dirty si overrides affectés

### 2. Local Variables Tab

**Comportement:** Identique à la structure existante

**Operations:**
- [+] Add Local Variable (existant)
- Edit name / type / default value
- [x] Delete variable

**Changes from Previous:**
- ❌ SUPPRESSION: Checkbox [G] (Global) - pas nécessaire
- Toutes les variables ajoutées ici sont implicitement `IsGlobal = false`

### 3. Conflit Resolution

**Scenario:** Utilisateur essaie d'ajouter "Health" localement alors qu'elle existe globalement

**Behavior:**
```
Utilisateur tape: "Health" dans le panel Local
      ↓
Validation détecte conflit (Health existe en Global)
      ↓
Dialog d'avertissement:
┌─ Naming Conflict ──────────────────────────┐
│                                            │
│ ⚠ Variable "Health" already exists:        │
│                                            │
│ (G) Health (Float, Global)                 │
│                                            │
│ Choose action:                             │
│ ○ Use existing (G) Health                  │
│ ○ Rename to "Health_Local"                 │
│ ○ Cancel                                   │
│                                            │
│ [OK]  [Cancel]                             │
└────────────────────────────────────────────┘
```

**Resolution Logic:**
- Option A: Utiliser la variable globale existante (ne pas créer duplicate)
- Option B: Permettre renommer pour éviter le conflit
- Option C: Annuler l'opération

---

## 💾 Sérialisation & Persistence

### 1. Registre Global - Sauvegarde

**Localisation:** `./Config/global_blackboard_register.json`

**Quand Sauvegarder:**
- ✅ Après ajout d'une variable globale ([+] Add)
- ✅ Après modification (name, type, default value)
- ✅ Après suppression ([x] Delete)
- ⚠️ Sauvegarder dans l'éditeur Blueprint (menu ou hotkey)
- ⚠️ Sauvegarder en quittant l'éditeur

**Format JSON - Complet:**
```json
{
  "version": "1.0",
  "lastModified": "2026-03-26T14:35:22Z",
  "variables": [
    {
      "key": "Position",
      "type": "Vector",
      "defaultValue": { "x": 0.0, "y": 0.0, "z": 0.0 },
      "description": "Entity world position"
    }
  ]
}
```

### 2. Blueprint - Sérialisation

**Localisation:** `Blueprint.ats` (JSON v4 schema)

**Contenu Blackboard:**
```json
{
  "blackboard": [
    {
      "key": "CurrentTarget",
      "type": "EntityID",
      "value": "0",
      "isGlobal": false
    }
  ],
  "globalVariableOverrides": [
    {
      "key": "Health",
      "value": 150.0
    }
  ]
}
```

**Règles de Sérialisation:**
- ✅ Variables LOCALES: toujours sauvegarder (isGlobal: false)
- ✅ Variables GLOBALES: sauvegarder UNIQUEMENT les overrides
- ✅ Optimisation: si global == default, ne pas inclure dans overrides
- ✅ Format: array `globalVariableOverrides` distinct du `blackboard`

### 3. Chargement - Desérialisation

**Registre Global (Boot):**
```cpp
// Au démarrage
GlobalTemplateBlackboard::Get().LoadFromFile("./Config/global_blackboard_register.json");
```

**Blueprint - Local Variables:**
```cpp
// À l'ouverture du blueprint dans l'éditeur
LocalBlackboard.InitializeFromEntries(blueprint.blackboard);  // Filtre isGlobal: false
```

**Blueprint - Global Variable Overrides:**
```cpp
// À l'ouverture du blueprint dans l'éditeur
std::map<std::string, TaskValue> overrides;
for (const auto& entry : blueprint.globalVariableOverrides) {
    overrides[entry.key] = entry.value;
}
```

**Entity Instance (Runtime):**
```cpp
// Lors de la création d'une instance d'entité
EntityBlackboard eb;
eb.InitializeFromTemplate(
    GlobalTemplateBlackboard::Get(),
    blueprint_overrides
);
```

### 4. Backward Compatibility

**Migration de l'Ancien Système:**

Ancien blueprint avec variables globales:
```json
{
  "blackboard": [
    {
      "key": "GamePhase",
      "isGlobal": true,
      "value": "Combat"
    }
  ]
}
```

Migration vers nouveau système:
```json
{
  "blackboard": [
    // ❌ Ancien format supprimé de cette section
  ],
  "globalVariableOverrides": [
    // ✅ Variables globales migrées ici
    { "key": "GamePhase", "value": "Combat" }
  ]
}
```

**Logique de Migration:**
1. Détecter ancien format (isGlobal: true dans blackboard)
2. Extraire les variables globales
3. Ajouter au registre global (si n'existe pas)
4. Convertir les valeurs en overrides
5. Nettoyer l'ancien format

---

## ⚙️ Runtime Behavior

### 1. Initialisation (Boot)

```
Sequence:
1. Charger GlobalTemplateBlackboard.LoadFromFile("./Config/global_blackboard_register.json")
2. Registre en mémoire, immutable jusqu'au prochain restart
3. À chaque instance d'entité créée:
   - Charger le blueprint
   - InitializeFromTemplate(registre, blueprint_overrides)
   - Entity a maintenant:
     - LocalBlackboard (graph-specific)
     - EntityBlackboard (global template values)
```

### 2. Accès Variable - Runtime

**GetBBValue Node:**
```cpp
TaskValue value = localBlackboard.GetValueScoped("VariableName");
// Recherche:
// 1. Dans LocalBlackboard si "local:VariableName" ou default "VariableName"
// 2. Dans EntityBlackboard si "(G) VariableName"
// 3. Retourner default si not found
```

**SetBBValue Node:**
```cpp
localBlackboard.SetValueScoped("VariableName", newValue);
// Écrit:
// 1. Dans LocalBlackboard si "local:VariableName" ou default
// 2. Dans EntityBlackboard si "(G) VariableName"
// 3. Type-checking enforced
```

### 3. Scope Resolution Priority

```
When accessing "Position":
1. Check if starts with "local:" → LocalBlackboard
2. Check if starts with "global:" → EntityBlackboard
3. No prefix (default):
   - Try LocalBlackboard first
   - If not found, try EntityBlackboard
   - If not found, return default TaskValue()
```

### 4. Type Safety

**Validation à Runtime:**
- ✅ SetValue() valide que le type de la nouvelle valeur correspond au type déclaré
- ✅ Throw exception ou log warning si type mismatch
- ✅ GetValue() retourne la valeur stockée ou default TaskValue() si not found

---

## 📚 Cas d'Usage & Exemples

### Use Case 1: Position Shared Across Graph

**Scenario:**
```
Entity 1 et Entity 2 exécutent le même graphe
Chaque entité a sa propre position (Global template variable)
```

**Blueprint Graph:**
```
GetBBValue("Position") → Affiche la position de CETTE entité
GetBBValue("Velocity") → Affiche la vélocité de CETTE entité
```

**Runtime:**
```
Entity 1: GetBBValue("Position") = (100, 0, 0)
Entity 2: GetBBValue("Position") = (50, 0, 0)

Chaque entité voit sa propre valeur ✅
```

### Use Case 2: Local Health Override

**Scenario:**
```
Registre global définit Health (Float, default: 100.0)
Blueprint AI_Boss override: Health = 500.0
```

**Sérialisation:**
```json
// global_blackboard_register.json
{ "key": "Health", "type": "Float", "defaultValue": 100.0 }

// Blueprint.ats
{
  "globalVariableOverrides": [
    { "key": "Health", "value": 500.0 }
  ]
}
```

**Runtime:**
```
Entity (AI_Boss): GetBBValue("Health") = 500.0 ✅ (from override)
Entity (AI_Scout): GetBBValue("Health") = 100.0 ✅ (from default)
```

### Use Case 3: Local Variable Independence

**Scenario:**
```
Deux entités exécutent le même graphe
Chacune a son propre "CurrentTarget" local (non-global)
```

**Blueprint:**
```json
{
  "blackboard": [
    {
      "key": "CurrentTarget",
      "type": "EntityID",
      "isGlobal": false,
      "value": "0"
    }
  ]
}
```

**Runtime:**
```
Entity 1: GetBBValue("CurrentTarget") = EntityID(42)
Entity 2: GetBBValue("CurrentTarget") = EntityID(0)

Chaque entité maintient sa propre cible ✅
```

### Use Case 4: Conflicting Names Resolution

**Scenario:**
```
Registre global a "Speed" (Float)
Blueprint local définit aussi "Speed" (Int)
```

**Resolution:**
- ✅ Sauvegarder localement comme: "(L) Speed" ou renommer
- ✅ Afficher dans dropdowns avec prefixes pour clarifier
- ⚠️ Runtime: résoudre le conflit (préférer Global par défaut)

---

## ✅ Acceptance Criteria

### UI / Editor

- [ ] **Tab Interface Implémentée**
  - [ ] Global tab créé et sélectionné par défaut
  - [ ] Local tab créé
  - [ ] Splitter visible et fonctionnel

- [ ] **Global Variables Panel**
  - [ ] Affiche liste des variables du registre
  - [ ] [+] Add Global Variable fonctionne
  - [ ] Edit name/type/default value fonctionne
  - [ ] [x] Delete variable fonctionne
  - [ ] Validation: pas de duplicates

- [ ] **Local Variables Panel**
  - [ ] ❌ Checkbox [G] supprimée
  - [ ] Toutes les variables implicitement `IsGlobal = false`
  - [ ] [+] Add Local Variable fonctionne
  - [ ] Edit/Delete fonctionne
  - [ ] Conflits détectés et gérés

- [ ] **Dropdowns (GetBBValue, SetBBValue, etc.)**
  - [ ] Affichent prefix (L) pour locals
  - [ ] Affichent prefix (G) pour globals
  - [ ] Tous les deux types disponibles
  - [ ] Sélection fonctionne correctement

### Sérialisation

- [ ] **Registre Global**
  - [ ] `./Config/global_blackboard_register.json` créé/chargé
  - [ ] Variables du registre loadées au boot
  - [ ] Nouvelles variables sauvegardées dans le fichier
  - [ ] Format JSON valide

- [ ] **Blueprint Serialization**
  - [ ] Variables locales stockées dans `blackboard[]`
  - [ ] Global overrides stockés dans `globalVariableOverrides[]`
  - [ ] Optimisation: defaults non-sauvegarder
  - [ ] Backward compatibility: migration ancien format

### Runtime

- [ ] **EntityBlackboard Initialization**
  - [ ] Instances initialisées depuis registre
  - [ ] Overrides appliquées correctement
  - [ ] Valeurs per-entity indépendantes

- [ ] **Access & Modification**
  - [ ] GetBBValue() accède aux deux scopes
  - [ ] SetBBValue() modifie le bon scope
  - [ ] Type-checking enforced
  - [ ] Scope resolution correct

### Validation & Logging

- [ ] **Verification Panel**
  - [ ] Warnings loggés pour conflits de noms
  - [ ] Errors loggés pour type mismatches
  - [ ] Informations utiles pour debugging

- [ ] **Edge Cases**
  - [ ] Variable ajoutée au registre pendant éditeur ouvert
  - [ ] Variable modifiée/supprimée
  - [ ] Registre corrompu → error handling
  - [ ] Empty registre → comportement défini

---

## 🔧 Spécifications Techniques

### Scope de l'Implémentation

**À Implémenter:**
1. ✅ GlobalTemplateBlackboard Registry (NEW)
2. ✅ EntityBlackboard (NEW)
3. ✅ Refactor VisualScriptEditorPanel UI (Global/Local tabs)
4. ✅ Refactor BBVariableRegistry (prefixes + filtering)
5. ✅ JSON Serialization/Deserialization (registre + overrides)
6. ✅ Validation & Error Handling
7. ✅ Runtime Initialization & Access

**À Préserver:**
1. ✅ Existing LocalBlackboard (inchangé)
2. ✅ Existing GetBBValue/SetBBValue nodes (adapté scope resolution)
3. ✅ Existing Branch/Switch/While conditions (adapté scope)
4. ✅ JSON v4 schema (étendu avec globalVariableOverrides)

### Dependencies

**Existantes:**
- `LocalBlackboard.h/cpp`
- `BBVariableRegistry.h/cpp`
- `VisualScriptEditorPanel.cpp`
- `TaskGraphTemplate.h`
- `json_helper.h`

**À Créer:**
- `GlobalTemplateBlackboard.h/cpp`
- `EntityBlackboard.h/cpp`

### Configuration Files

**Nouveau Fichier:**
- `./Config/global_blackboard_register.json` (à générer)

---

## 📊 Risk Assessment

### Low Risk
- ✅ Serialization (bien défini, testé)
- ✅ UI Layout (structure simple, tab interface standard)
- ✅ Local Blackboard (préservé, inchangé)

### Medium Risk
- ⚠️ Scope Resolution (bien défini mais multi-branches)
- ⚠️ Conflict Resolution (needs careful validation)
- ⚠️ Backward Compatibility (migration required)

### Mitigation
- ✅ Comprehensive logging
- ✅ Unit tests pour scope resolution
- ✅ Migration script pour ancien format
- ✅ Validation stricte en editor

---

## 📝 Notes d'Implémentation

### Priority Order

1. **Phase 1: Infrastructure**
   - GlobalTemplateBlackboard registry
   - EntityBlackboard structure
   - Load/Save registre

2. **Phase 2: UI Editor**
   - Tab interface (Global/Local)
   - Global Variables panel
   - Local Variables panel (refactor)

3. **Phase 3: Integration**
   - BBVariableRegistry updates
   - Dropdowns with prefixes
   - Conflict resolution

4. **Phase 4: Runtime**
   - EntityBlackboard initialization
   - Scope resolution
   - Type validation

5. **Phase 5: Polish**
   - Error handling
   - Logging
   - Unit tests

---

## ✅ Validation Finale

**Document Validé Par:** [À remplir]  
**Date Validation:** [À remplir]  
**Statut:** 🟢 PRÊT POUR IMPLÉMENTATION  

**Points Confirmés:**
- ✅ Q1: Option C (hot-reload pas requis)
- ✅ Q2: Option C (registre figé jusqu'au restart)
- ✅ Q3: Option A (toutes les entités accès immédiat)
- ✅ Q4: Option B (JSON edition hors-editeur)
- ✅ Q5: C/C++ style (pas de duplicate same-type)
- ✅ Q6: Option A (scope au niveau du projet)

---

**Document Version:** 1.0  
**Date:** 2026-03-26  
**Status:** ✅ VALIDÉ - PRÊT POUR DÉVELOPPEMENT

