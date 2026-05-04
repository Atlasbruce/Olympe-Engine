# DocumentVersionManager - Architecture Professionnelle avec Fallback

## 🎯 Objectif

Créer un système **professionnel, scalable et résilient** pour gérer les versions de documents (Legacy vs Framework) avec:
- ✅ Routage centralisé des créations
- ✅ Fallback automatique à Legacy en cas d'erreur
- ✅ Logging complet des erreurs et des fallbacks
- ✅ Diagnostic en temps réel
- ✅ Audit trail complet

---

## 📋 Architecture

### Pattern utilisé: **Strategy + Registry + Factory + Fallback**

```
Client Code
    ↓
DocumentVersionManager::CreateNewDocument("EntityPrefab")
    ↓
GetEffectiveVersion("EntityPrefab") → Retourne version active
    ↓
GetStrategy(graphType, version) → Récupère la stratégie
    ↓
TryCreateWithStrategy() → Essaie de créer
    ├─ SUCCESS → Retourne document
    └─ FAILURE → 
        ├─ Log l'erreur avec contexte
        ├─ Si FallbackEnabled:
        │   ├─ Log le fallback
        │   ├─ Essaie Legacy version
        │   ├─ SUCCESS → Retourne document (avec fallback marker)
        │   └─ FAILURE → Log erreur critique, retourne nullptr
        └─ Si FallbackDisabled:
            └─ Retourne nullptr directement
```

---

## 🔧 Composants Clés

### 1. **GraphTypeVersion Enum**
```cpp
enum class GraphTypeVersion {
    Legacy = 1,      // V1: Implémentation originale (stable)
    Framework = 2,   // V2: Conforme framework (nouvelle)
    Next = 3,        // V3: Réservé pour améliorations futures
};
```

### 2. **DocumentCreationStrategy Struct**
```cpp
struct DocumentCreationStrategy {
    std::function<IGraphDocument*()> createNewDocument;
    std::function<IGraphDocument*(const std::string& filePath)> loadDocumentFromFile;
    std::function<IGraphRenderer*(IGraphDocument*)> createRenderer;
    std::string strategyName;      // "Legacy EntityPrefab", "Framework V2", etc.
    GraphTypeVersion version;      // Version this strategy represents
};
```

### 3. **DocumentVersionManager (Singleton)**

**Data Members:**
```cpp
// Stratégies enregistrées: graphType → version → strategy
std::map<std::string, std::map<GraphTypeVersion, DocumentCreationStrategy>> m_strategies;

// Versions actives: graphType → version active
std::map<std::string, GraphTypeVersion> m_activeVersions;

// Versions forcées (testing): graphType → version forcée
std::map<std::string, GraphTypeVersion> m_forcedVersions;

// État du fallback: graphType → enabled/disabled
std::map<std::string, bool> m_fallbackEnabled;

// Compteurs de fallback: graphType → nombre de fallbacks
std::map<std::string, size_t> m_fallbackCount;

// Logs de fallback: graphType → vector<log entries>
std::map<std::string, std::vector<std::string>> m_fallbackLogs;

// Diagnostics
std::string m_lastRoutingDecision;
std::string m_lastErrorMessage;
```

---

## 🚀 Flux de Création Détaillé

### Scénario 1: Création réussie
```
CreateNewDocument("EntityPrefab")
  ↓
Active Version = Framework v2
  ↓
GetStrategy("EntityPrefab", Framework) → ✓ trouvée
  ↓
TryCreateWithStrategy()
  ├─ strategy->createNewDocument() → ✓ retourne document
  ├─ Log: "[SUCCESS] EntityPrefab v2 created via Framework strategy"
  └─ return document ✅
```

### Scénario 2: Création échoue, fallback réussit
```
CreateNewDocument("EntityPrefab")
  ↓
Active Version = Framework v2
  ↓
GetStrategy("EntityPrefab", Framework) → ✓ trouvée
  ↓
TryCreateWithStrategy()
  ├─ strategy->createNewDocument() → ✗ retourne nullptr
  ├─ Log ERROR: "[ERROR] EntityPrefab v2 failed: File not found (caller: TabManager.cpp:187)"
  ├─ FallbackEnabled("EntityPrefab") → ✓ true
  ├─ Log FALLBACK: "[FALLBACK] EntityPrefab v2 → v1 (reason: File not found) [2024-01-15 14:23:45.123]"
  ├─ m_fallbackCount["EntityPrefab"]++ → 1
  ├─ GetStrategy("EntityPrefab", Legacy) → ✓ trouvée
  ├─ TryCreateWithStrategy() → ✓ Legacy réussit
  ├─ Log: "[SUCCESS] EntityPrefab v1 created via Legacy strategy (fallback)"
  └─ return document ✅ (mais fallback s'est produit)
```

### Scénario 3: Les deux échouent
```
CreateNewDocument("EntityPrefab")
  ↓
Active Version = Framework v2
  ↓
[Framework échoue]
  ├─ Log ERROR: "[ERROR] EntityPrefab v2 failed: Parse error (caller: ...)"
  ├─ FallbackEnabled → ✓ true
  ├─ Log FALLBACK: "[FALLBACK] EntityPrefab v2 → v1 ..."
  ├─ [Legacy échoue aussi]
  ├─ Log ERROR: "[ERROR] EntityPrefab v1 failed: File corrupted (caller: ...)"
  ├─ Log CRITICAL: "[CRITICAL] Both v2 and v1 failed - cannot recover"
  └─ return nullptr ❌
```

### Scénario 4: Fallback désactivé
```
CreateNewDocument("EntityPrefab")
  ↓
Active Version = Framework v2
  ↓
[Framework échoue]
  ├─ Log ERROR: "[ERROR] EntityPrefab v2 failed: ..."
  ├─ FallbackEnabled("EntityPrefab") → ✗ false
  ├─ Log: "[INFO] Fallback disabled for EntityPrefab - no retry with Legacy"
  └─ return nullptr ❌ (sans essayer Legacy)
```

---

## 📊 Logging et Diagnostics

### Types de Logs

**1. ROUTING (Info)**
```
[DocumentVersionManager] CreateNewDocument: EntityPrefab (v2)
[DocumentVersionManager] LoadDocument: EntityPrefab from './data/entity.json' (v2)
[DocumentVersionManager] Active version changed: EntityPrefab -> v2
```

**2. ERROR (Warning/Error)**
```
[DocumentVersionManager] ERROR: EntityPrefab v2 failed: Parse error (caller: TabManager.cpp:187)
[DocumentVersionManager] ERROR: Strategy not found for EntityPrefab v3
[DocumentVersionManager] ERROR: Cannot set active version: version not registered
```

**3. FALLBACK (Warning)**
```
[DocumentVersionManager] FALLBACK: EntityPrefab v2 → v1 (reason: File not found) [2024-01-15 14:23:45.123]
[DocumentVersionManager] FALLBACK: VisualScript v2 → v1 (reason: Corrupt graph) [2024-01-15 14:23:46.456]
```

**4. SUCCESS (Info)**
```
[DocumentVersionManager] SUCCESS: create completed for EntityPrefab v2 using strategy 'Framework V2'
[DocumentVersionManager] SUCCESS: load completed for EntityPrefab v1 using strategy 'Legacy'
```

### Méthodes de Diagnostic

```cpp
// Voir état complet
std::string info = manager.GetDiagnosticInfo();
/*
========== DocumentVersionManager Diagnostic Info ==========

[Registered Graph Types]
  - EntityPrefab
    v1 [ACTIVE]
    v2
  - VisualScript
    v1 [ACTIVE]
    v2 [FALLBACKS: 3]

[Last Routing Decision]
  LoadDocument: EntityPrefab from './data/entity.json' (v1)

[Last Error]
  EntityPrefab v2 failed: Parse error (caller: TabManager.cpp:187)

==========================================================
*/

// Voir fallbacks pour un type
std::string fallbackLog = manager.GetFallbackLog("EntityPrefab");
/*
========== Fallback Log for 'EntityPrefab' ==========
Total Fallbacks: 3

[1] [DocumentVersionManager] FALLBACK: EntityPrefab v2 → v1 (reason: File not found) [2024-01-15 14:23:45.123]
[2] [DocumentVersionManager] FALLBACK: EntityPrefab v2 → v1 (reason: JSON parse error) [2024-01-15 14:23:47.456]
[3] [DocumentVersionManager] FALLBACK: EntityPrefab v2 → v1 (reason: Missing schema) [2024-01-15 14:23:49.789]

=========================================================
*/

// Nombre de fallbacks
size_t count = manager.GetFallbackCount("EntityPrefab"); // 3

// Dernier message d'erreur
std::string lastError = manager.GetLastErrorMessage();
```

---

## 🎮 Usage - Initialisation

### Phase 1: Register Strategies (au démarrage)

```cpp
// Dans une fonction initialize() appelée au démarrage:

DocumentVersionManager& manager = DocumentVersionManager::Get();

// Enregistrer EntityPrefab Legacy
DocumentCreationStrategy legacyEntityStrategy = {
    []() { 
        return new EntityPrefabGraphDocument(); 
    },
    [](const std::string& path) { 
        auto doc = new EntityPrefabGraphDocument();
        if (doc->Load(path)) return doc;
        delete doc;
        return nullptr;
    },
    [](IGraphDocument* doc) { 
        return new EntityPrefabRenderer(static_cast<EntityPrefabGraphDocument*>(doc)); 
    },
    "Legacy EntityPrefab",
    GraphTypeVersion::Legacy
};

manager.RegisterStrategy("EntityPrefab", GraphTypeVersion::Legacy, legacyEntityStrategy);

// Enregistrer EntityPrefab Framework V2
DocumentCreationStrategy frameworkEntityStrategy = {
    []() { 
        return new EntityPrefabGraphDocumentV2(); 
    },
    [](const std::string& path) { 
        auto doc = new EntityPrefabGraphDocumentV2();
        if (doc->Load(path)) return doc;
        delete doc;
        return nullptr;
    },
    [](IGraphDocument* doc) { 
        return new EntityPrefabEditorV2(static_cast<EntityPrefabGraphDocumentV2*>(doc)); 
    },
    "Framework EntityPrefab V2",
    GraphTypeVersion::Framework
};

manager.RegisterStrategy("EntityPrefab", GraphTypeVersion::Framework, frameworkEntityStrategy);

// Par défaut, Framework est actif (si enregistré)
manager.SetActiveVersion("EntityPrefab", GraphTypeVersion::Framework);

// Fallback est activé par défaut (mais peut être changé)
manager.SetFallbackEnabled("EntityPrefab", true);
```

### Phase 2: Use - Création transparente

```cpp
// Dans TabManager::CreateNewTab():
IGraphDocument* doc = DocumentVersionManager::Get()
    .CreateNewDocument("EntityPrefab");
    
// Routage automatique:
// - Essaie Framework v2
// - Si échoue: Essaie Legacy v1 automatiquement
// - Si les deux échouent: retourne nullptr

if (!doc)
{
    SYSTEM_LOG("Failed to create EntityPrefab: " + 
               DocumentVersionManager::Get().GetLastErrorMessage());
    return;
}
```

### Phase 3: Advanced - Testing et Debug

```cpp
// A/B Testing: Forcer Legacy pour certains tests
manager.ForceVersion("EntityPrefab", GraphTypeVersion::Legacy);
IGraphDocument* doc = manager.CreateNewDocument("EntityPrefab"); // ← Uses Legacy forcément

// Revenir à la configuration normale
manager.ClearForcedVersions();

// Désactiver fallback pour vérifier la robustesse de Framework
manager.SetFallbackEnabled("EntityPrefab", false);
doc = manager.CreateNewDocument("EntityPrefab"); // ← Échoue s'il y a problème

// Réactiver fallback
manager.SetFallbackEnabled("EntityPrefab", true);

// Analyser les problèmes
if (manager.GetFallbackCount("EntityPrefab") > 0)
{
    SYSTEM_LOG("EntityPrefab a eu problèmes:");
    SYSTEM_LOG(manager.GetFallbackLog("EntityPrefab"));
}
```

---

## 🛡️ Garanties de Robustesse

### 1. **Aucun Appel Accidentel du Legacy**
- Legacy n'est appelé que si:
  - Framework n'est pas enregistré
  - Framework retourne nullptr/exception
  - Fallback est activé
- Tous les appels loggés avec contexte

### 2. **Audit Trail Complet**
- Chaque création loggée: version tentée, résultat
- Chaque erreur loggée: raison, contexte (caller)
- Chaque fallback loggé: versions, raison, timestamp

### 3. **Pas de Surprises**
- Diagnostic accessible: `GetDiagnosticInfo()` montre tout
- Fallback counters: Détecte les instabilités
- Forced versions: Peut tester une version spécifique

### 4. **Dégradation Gracieuse**
- Framework échoue → Fallback automatique
- Legacy échoue → Retourne nullptr (pas de crash)
- Client gère nullptr

---

## 📈 Migration Progressive

### Phase 1: EntityPrefab (Actuellement)
- Register v1 (Legacy) et v2 (Framework)
- Actif: Framework v2
- Fallback: Activé
- Si v2 échoue: Retourne v1 automatiquement

### Phase 2: VisualScript (Après EP stable)
- Même pattern pour VisualScript
- Register v1 (imnodes) et v2 (Framework)
- Actif: Framework v2
- Fallback: Activé

### Phase 3: BehaviorTree (Après VS stable)
- Même pattern pour BehaviorTree
- Register v1 (NodeGraphPanel) et v2 (Framework)
- Actif: Framework v2
- Fallback: Activé

### Avantage: Zéro code nouveau pour routing

```cpp
// Même code pour tous les types:
IGraphDocument* doc = DocumentVersionManager::Get()
    .CreateNewDocument("EntityPrefab");  // ou "VisualScript", "BehaviorTree"

// Routage automatique via stratégie enregistrée
```

---

## 🔍 Détection d'Instabilité

### Scenario: Framework V2 a des bugs intermittents

```cpp
// Créer 100 documents
for (int i = 0; i < 100; ++i)
{
    IGraphDocument* doc = manager.CreateNewDocument("EntityPrefab");
    if (doc)
    {
        // Utiliser le document
        // ...
        delete doc;
    }
}

// Après 100 créations, analyser:
size_t fallbacks = manager.GetFallbackCount("EntityPrefab");
if (fallbacks > 5)
{
    SYSTEM_LOG("⚠️  EntityPrefab V2 is unstable!");
    SYSTEM_LOG("Fallbacks: " + std::to_string(fallbacks));
    SYSTEM_LOG(manager.GetFallbackLog("EntityPrefab"));
    
    // Décision: Revenir à Legacy ou investiguer
    manager.SetActiveVersion("EntityPrefab", GraphTypeVersion::Legacy);
}
```

---

## 📝 Notes de Conception

### Pourquoi pas de statique GetCallerContext()?
- Actuellement retourne "unknown:0"
- Peut être implémenté avec:
  - Windows: StackWalk64 + dbghelp.dll
  - Linux: backtrace() + backtrace_symbols()
- Pas critique pour Phase 2.1 mais possible ajout futur

### Pourquoi TryCreateWithStrategy()?
- Centralise la gestion d'erreur
- Exception handling
- Logging cohérent
- Facilite le fallback

### Pourquoi pas d'auto-detection du type?
- Routing décidé par caller (TabManager), pas par contenu fichier
- Plus rapide
- Plus explicite
- Évite les surprises

---

## ✅ Checklist Implémentation

- ✅ DocumentVersionManager.h (architecture)
- ✅ DocumentVersionManager.cpp (implementation)
- ⏳ Chunk 2: Rendering & Canvas
- ⏳ Chunk 3: TabManager Integration
- ⏳ Chunk 4-8: Features & Documentation

---

**Status: READY FOR INTEGRATION** 🚀
