# CONCEPTION ATS — Node Types, Variables & Blackboard
## Olympe Engine — ATS (Action Tree System)
**Version :** 1.0  
**Date :** 2026-03-08  
**Auteur :** Atlasbruce / GitHub Copilot

---

## 1. Vue d'ensemble du système ATS

L'ATS (Action Tree System) est l'éditeur de graphes de comportement d'Olympe Engine. Il s'inspire d'Unreal Engine Blueprint et des Behavior Trees pour offrir une interface visuelle de programmation des comportements IA et système.

Architecture :
- `NodeTypeCatalogue` : référentiel des types de nodes disponibles
- `GlobalBlackboard` : variables globales partagées entre tous les graphes
- `BlackboardSystem` (local) : variables locales à un graphe (dans `GraphDocument`)
- `GraphDocument` : document d'un graphe (nodes, links, blackboard local)

---

## 2. Types de Nodes

### 2.1 Composites

| Type | Couleur | Rôle |
|------|---------|------|
| Sequence | #3A7D44 | Exécute les enfants en ordre — s'arrête au 1er Failure (AND ordonné) |
| Selector | #1E6091 | Exécute les enfants en ordre — s'arrête au 1er Success (OR ordonné) |
| Parallel | #7D3A7D | Exécute tous les enfants simultanément |
| RandomSequence | #2D6B3A | Sequence avec ordre aléatoire |
| RandomSelector | #154F7A | Selector avec ordre aléatoire |

Règles :
- Un Composite peut avoir N enfants (Composites, Decorators, Actions)
- Un Composite peut porter des **Services** (s'exécutent en parallèle du tick)
- La connexion se fait via des **pins Exec** (un pin Exec_in, N pins Exec_out numérotés)

### 2.2 Decorators (Modificateurs)

Les Decorators s'attachent à un node existant (Composite ou Action) et modifient son comportement.

| Type | Rôle |
|------|------|
| Inverter | Inverse le résultat (Success↔Failure) |
| ForceSuccess | Retourne toujours Success |
| ForceFailure | Retourne toujours Failure |
| Cooldown(duration) | Bloque pendant N secondes après exécution |
| Loop(count=-1) | Répète N fois (-1 = infini) |
| TimeLimit(time_limit) | Abandonne après N secondes |
| ConditionalLoop(var, op, val) | Répète tant que condition vraie |
| CheckBlackboard(var_name) | N'exécute que si var_name est truthy |

### 2.3 Actions (Tasks / Feuilles)

Actions pré-codées en dur, accessibles depuis le catalogue :

#### Catégorie Movement
| Node | Signature | Description |
|------|-----------|-------------|
| GotoPosition | `(target: Vector, speed: Float=300)` | Déplace l'entité vers une position |
| GetNextWaypoint | `(graph_ref: GraphRef) → Vector` | Récupère le prochain waypoint |
| Flee | `(target: EntityRef, distance: Float=200)` | Fuit loin d'une cible |

#### Catégorie State
| Node | Signature | Description |
|------|-----------|-------------|
| ChangeState | `(new_state: String)` | Change l'état courant |

#### Catégorie Rendering
| Node | Signature | Description |
|------|-----------|-------------|
| SetSprite | `(sprite_ref: SpriteRef)` | Change le sprite |
| SetAnimation | `(anim_ref: AnimRef, loop: Bool=true)` | Joue une animation |
| PlayAnimation | `(animation_name: String, loop: Bool=false)` | Alias de SetAnimation |

#### Catégorie System
| Node | Signature | Description |
|------|-----------|-------------|
| GetDeltaTime | `() → Float` | Retourne le delta time |
| SetPosition | `(instance: EntityRef, pos: Vector)` | Téléporte une instance |
| GetWorldInstance | `(instance_name: String) → EntityRef` | Récupère une instance par nom |
| LoadLevel | `(level_name: String)` | Charge un niveau |

#### Catégorie Timing
| Node | Signature | Description |
|------|-----------|-------------|
| Wait | `(duration: Float)` | Attend N secondes |

#### Catégorie Combat
| Node | Signature | Description |
|------|-----------|-------------|
| Attack | `(damage: Float=10, range: Float=50)` | Attaque la cible |
| Flee | `(target: EntityRef, distance: Float=200)` | Fuit loin d'une cible |

#### Catégorie Interaction
| Node | Signature | Description |
|------|-----------|-------------|
| PickupItem | `(item_id: String)` | Ramasse un objet |
| UseItem | `(item_id: String)` | Utilise un objet |

### 2.4 Conditions

| Node | Signature | Description |
|------|-----------|-------------|
| CompareVar | `(var_name: String, operator: String, value: Any) → Bool` | Compare une variable blackboard |
| BlackboardCheck | `(var_name: String) → Bool` | Variable truthy ? |
| IsInRange | `(target: EntityRef, range: Float) → Bool` | Dans le rayon ? |
| HasLineOfSight | `(target: EntityRef) → Bool` | Ligne de vue ? |
| IsAnimationFinished | `() → Bool` | Anim terminée ? |
| IsAtPosition | `(pos: Vector, tolerance: Float=5) → Bool` | À la position ? |

### 2.5 Services

Services attachés aux Composites (et éventuellement aux Tasks) — tick en parallèle :

| Service | Paramètres | Description |
|---------|------------|-------------|
| UpdateBlackboard | `(var_name, source, interval=0.1)` | Met à jour une variable régulièrement |
| TickTimer | `(var_name, interval=1.0)` | Incrémente un compteur |
| TrackTarget | `(target_var, interval=0.1)` | Suit la position d'une cible |
| PlayLoopedAnimation | `(anim_ref, interval=0.0)` | Animation en boucle |

---

## 3. Types de Variables (Blackboard & Pins)

| Type | C++ | Description |
|------|-----|-------------|
| `Int` | `int` | Entier 32 bits |
| `Float` | `float` | Flottant 32 bits |
| `Bool` | `bool` | Booléen |
| `String` | `std::string` | Chaîne de caractères |
| `Vector` | `Vector(x,y)` | Vecteur 2D — utilise le constructeur surchargé de `Vector`. Ne pas créer de Vector2 séparé. |
| `EntityRef` | `std::string` (nom) | Référence à une entité du monde |
| `GraphRef` | `std::string` (filepath) | Référence à un fichier graphe externe |
| `AnimRef` | `std::string` (id ou filepath) | Référence à une animation |
| `SpriteRef` | `std::string` (id ou filepath) | Référence à un sprite |
| `Any` | Union | Générique (pour nodes utilitaires) |

> **Règle :** On privilégie la classe `Vector` existante dans Olympe. Le constructeur `Vector(x, y)` se comporte comme un Vector2. Ne jamais créer un type `Vector2` séparé.

---

## 4. Blackboard System

### 4.1 Global Blackboard

- **Fichier :** `Blueprints/global_blackboard.json`
- **Classe :** `GlobalBlackboard` (singleton, `Source/NodeGraphCore/GlobalBlackboard.h`)
- **Rôle :** Variables partagées entre TOUS les graphes ATS
- **Éditable** dans l'éditeur ATS et sauvegardable sur disque
- **Types supportés :** Int, Float, Bool, String, Vector (x, y)

### 4.2 Local Blackboard (par graphe)

- **Accesseur :** `GraphDocument::GetLocalBlackboard()` (instance `BlackboardSystem`)
- **Rôle :** Variables locales à un graphe, non partagées
- **Sérialisé** dans le JSON v2 du graphe sous la clé `"localBlackboard"`
- **Rétrocompatibilité :** si absent dans le JSON, blackboard local initialisé vide. Si la clé `"blackboard"` (format précédent) est trouvée, elle est utilisée.

### 4.3 Règles d'accès dans les nodes

1. Un node peut lire/écrire dans le **local blackboard** du graphe courant via `doc.GetLocalBlackboard()`
2. Un node peut lire/écrire dans le **global blackboard** via `GlobalBlackboard::Get()`
3. Les **Conditions** lisent le blackboard et retournent un Bool
4. Les **Services** mettent à jour le blackboard en arrière-plan

---

## 5. Types de Liaisons (Pins)

| Type | Couleur | Direction | Description |
|------|---------|-----------|-------------|
| `Exec` | Blanc/Gris | In/Out | Flux d'exécution (contrôle) |
| `Int` | Cyan | In/Out | Valeur entière |
| `Float` | Vert | In/Out | Valeur flottante |
| `Bool` | Rouge | In/Out | Valeur booléenne |
| `String` | Rose | In/Out | Chaîne |
| `Vector` | Jaune | In/Out | Vecteur 2D |
| `EntityRef` | Orange | In/Out | Référence entité |
| `GraphRef` | Violet | In/Out | Référence graphe |
| `AnimRef` | Bleu clair | In/Out | Référence animation |
| `SpriteRef` | Turquoise | In/Out | Référence sprite |
| `Any` | Gris | In/Out | Type générique |

**Règle de connexion :** Deux pins ne peuvent être connectés que si leurs types sont compatibles. `Any` est compatible avec tous les types.

---

## 6. Règles de Graphe

1. **Un graphe a exactement un nœud Root** (Sequence ou Selector de plus haut niveau)
2. **Les Decorators** s'attachent à un node existant — connexion via une liaison de type "Decorator" (lien spécial, pas un Exec normal)
3. **Les Services** s'attachent à un Composite (ou Task) — connexion via une liaison de type "Service"
4. **Les graphes externes** sont référencés par filepath — le node `GetNextWaypoint` prend un `GraphRef`
5. **Pas de cycles** — le graphe est un arbre dirigé (DAG). La validation `GraphDocument::HasCycles()` est appelée avant sauvegarde.
6. **Les pins Exec** ne peuvent être connectés qu'à d'autres pins Exec
7. **Les pins de données** (Int, Float, etc.) ne peuvent pas se connecter à des pins Exec

---

## 7. Catalogue JSON — Format de référence

```json
{
  "schema_version": 2,
  "catalogType": "Actions",
  "version": "2.0",
  "types": [
    {
      "id": "GotoPosition",
      "name": "Goto Position",
      "category": "Movement",
      "description": "Déplace l'entité vers une position du monde",
      "tooltip": "Utilise le pathfinder pour atteindre la position cible",
      "color": "#4A90D9",
      "pins": {
        "inputs":  [{ "id": "exec_in",  "label": "",       "type": "Exec"   },
                    { "id": "target",   "label": "Target",  "type": "Vector" },
                    { "id": "speed",    "label": "Speed",   "type": "Float"  }],
        "outputs": [{ "id": "exec_out", "label": "",       "type": "Exec"   }]
      },
      "parameters": [
        { "name": "speed", "type": "Float", "required": false, "default": "300.0", "description": "Vitesse de déplacement" }
      ]
    }
  ]
}
```

---

## 8. Fichiers et classes — Cartographie

| Fichier | Classe / Rôle |
|---------|---------------|
| `Source/NodeGraphCore/NodeTypeCatalogue.h/.cpp` | Chargement et accès aux types de nodes |
| `Source/NodeGraphCore/GlobalBlackboard.h/.cpp` | Singleton des variables globales |
| `Source/NodeGraphCore/BlackboardSystem.h/.cpp` | Variables locales d'un graphe |
| `Source/NodeGraphCore/GraphDocument.h/.cpp` | Document graphe (inclut localBlackboard) |
| `Blueprints/Catalogues/ActionTypes.json` | Catalogue v2 des actions |
| `Blueprints/Catalogues/ConditionTypes.json` | Catalogue v2 des conditions |
| `Blueprints/Catalogues/DecoratorTypes.json` | Catalogue v2 des décorateurs |
| `Blueprints/Catalogues/CompositeTypes.json` | Catalogue v2 des composites |
| `Blueprints/Catalogues/ServiceTypes.json` | Catalogue v2 des services |
| `Blueprints/global_blackboard.json` | Données du blackboard global |

---

## 9. Roadmap des phases suivantes

- **Phase 2 :** Intégration UI dans l'éditeur ATS — panneau catalogue (drag & drop), panneau blackboard (édition variables)
- **Phase 3 :** Runtime ATS — exécution des nodes à partir du catalogue, résolution blackboard
- **Phase 4 :** Nodes utilisateur — définition de nodes custom via JSON ou code C++
