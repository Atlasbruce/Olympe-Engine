# Corrections appliquées - EmbeddedBreakpoint Exception

## Problème identifié
**Exception:** `EmbeddedBreakpoint` lors de l'appel à `GetCubicBezier()` dans `ResolveHoveredLink` (imnodes.cpp:1301)

**Cause racine:** Un lien pointait vers des pins dont les positions n'avaient pas été initialisées dans le frame courant, créant des **valeurs indéterminées (garbage memory)** utilisées dans le calcul de la longueur de la courbe de Bézier.

### Chaîne d'appels:
```
ResolveHoveredLink() [1301]
  → GetCubicBezier(start_pin.Pos, end_pin.Pos, ...)
    → ImSqrt(ImLengthSqr(end - start))  // ← Garbage values ici
    → IM_ASSERT(...) fail
```

---

## Corrections apportées

### 1. **imnodes.cpp - ResolveHoveredLink() [L1286-L1320]**

✅ **Ajout de validation des pins avant calcul de Bézier**

```cpp
// Validation: Skip processing the link if either pin hasn't been rendered yet.
// A pin that hasn't been rendered will have an empty AttributeRect and a position of (0, 0).
// This can happen if a node containing a pin is not rendered in the current frame.
// Without this check, uninitialized pin positions (garbage memory) can cause assertion failures
// in GetCubicBezier when computing link_length from corrupted coordinate values.

const bool start_pin_valid = (start_pin.AttributeRect.Max.x > start_pin.AttributeRect.Min.x ||
                               start_pin.AttributeRect.Max.y > start_pin.AttributeRect.Min.y ||
                               start_pin.Pos.x != 0.0f || start_pin.Pos.y != 0.0f);
const bool end_pin_valid = (end_pin.AttributeRect.Max.x > end_pin.AttributeRect.Min.x ||
                             end_pin.AttributeRect.Max.y > end_pin.AttributeRect.Min.y ||
                             end_pin.Pos.x != 0.0f || end_pin.Pos.y != 0.0f);

if (!start_pin_valid || !end_pin_valid)
{
    continue;  // Skip uninitialized pins
}
```

**Logique:** 
- Identique à la validation dans `DrawLink()` (L1996-L2003)
- Détecte les pins non rendus en vérifiant les rectangles et positions
- Empêche le calcul de Bézier sur des données indéterminées

---

### 2. **VisualScriptEditorPanel.cpp - Initialisation des positions**

#### 2.1. **AddNode() - Validation des paramètres d'entrée**

✅ **Ajout de vérification de finitude et de plage**

```cpp
int VisualScriptEditorPanel::AddNode(TaskNodeType type, float x, float y)
{
    // Validate incoming position parameters to prevent garbage values
    if (!std::isfinite(x) || !std::isfinite(y))
    {
        SYSTEM_LOG << "[VSEditor] AddNode: warning - non-finite position provided\n";
        x = 0.0f;
        y = 0.0f;
    }

    // Clamp to a reasonable range to prevent extreme coordinate values
    if (x < -100000.0f || x > 100000.0f) x = 0.0f;
    if (y < -100000.0f || y > 100000.0f) y = 0.0f;

    // ... rest of AddNode
}
```

**Bénéfices:**
- Détecte les valeurs NaN/Inf
- Clamp aux limites raisonnables
- Enregistre les anomalies pour diagnostique

#### 2.2. **SyncCanvasFromTemplate() - Validation des positions chargées**

✅ **Vérification avant restauration depuis le fichier**

```cpp
if (def.HasEditorPos)
{
    // Validate loaded position to prevent garbage values from corrupted JSON
    if (std::isfinite(def.EditorPosX) && std::isfinite(def.EditorPosY) &&
        def.EditorPosX >= -100000.0f && def.EditorPosX <= 100000.0f &&
        def.EditorPosY >= -100000.0f && def.EditorPosY <= 100000.0f)
    {
        eNode.posX = def.EditorPosX;
        eNode.posY = def.EditorPosY;
    }
    else
    {
        // Fall back to auto-layout if corrupted
        eNode.posX = 200.0f * static_cast<float>(i);
        eNode.posY = 100.0f;
    }
}
```

#### 2.3. **SyncEditorNodesFromTemplate() - Protection multi-niveaux**

✅ **Cascade de validation avec fallback** (appliquée à 3 sources de positions):

1. **Paramètres du template** (`__posX`, `__posY`)
2. **Positions sauvegardées** en mémoire
3. **Positions du fichier** (`EditorPosX`, `EditorPosY`)
4. **Auto-layout par défaut** si tout échoue

Pour chacun: vérification `std::isfinite()` + clamp `[-100000, 100000]`

#### 2.4. **Drag-and-drop - Validation des positions reçues**

✅ **Vérification dans PHASE 2 du traitement des nœuds**

```cpp
if (m_pendingNodeDrop)
{
    // Ensure positions are not garbage values
    float safeX = m_pendingNodeX;
    float safeY = m_pendingNodeY;
    if (!std::isfinite(safeX) || !std::isfinite(safeY) || 
        safeX < -100000.0f || safeX > 100000.0f || 
        safeY < -100000.0f || safeY > 100000.0f)
    {
        safeX = 0.0f;
        safeY = 0.0f;
        SYSTEM_LOG << "[VSEditor] Warning: pending node position was garbage\n";
    }

    int newNodeID = AddNode(m_pendingNodeType, safeX, safeY);
    // ...
}
```

---

### 3. **Dépendances - Header ajouté**

✅ **Inclusion de `<cstdlib>` pour `std::isfinite()`**

```cpp
#include <cstdlib>  // For isfinite() validation
```

---

## Points clés de la solution

| Aspect | Avant | Après |
|--------|-------|-------|
| **Liens avec pins non-rendus** | Crash (garbage memory) | Skipés silencieusement |
| **Positions invalides** | Acceptées, causent des bugs | Rejetées et loggées |
| **Finitude des floats** | Non vérifiée | Vérifiée partout |
| **Range des positions** | Illimité | Clamped [-100k, 100k] |
| **Diagnostic** | Aucun | SYSTEM_LOG complet |

---

## Résultat

✅ **L'exception `EmbeddedBreakpoint` ne devrait plus survenir** car:
1. Les pins non-rendus sont détectés et évités dans `ResolveHoveredLink`
2. Les positions indéterminées sont filtrées à la source
3. Chaque entrée de position est validée et sanitisée
4. Les anomalies sont enregistrées pour futur diagnostique

---

## Recommandations supplémentaires

1. **Ajouter des assertions** pour detecter les positions invalides en debug:
   ```cpp
   IM_ASSERT(std::isfinite(eNode.posX) && std::isfinite(eNode.posY));
   ```

2. **Monitorer les logs** pour déterminer si d'autres sources produisent des garbage values

3. **Test de régression**: Charger/créer des graphes avec beaucoup de liens pour confirmer la stabilité
