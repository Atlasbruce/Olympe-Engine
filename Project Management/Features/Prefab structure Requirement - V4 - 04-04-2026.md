📋 STRUCTURE JSON CORRECTE - EntityPrefab v4

🔑 CHAMPS OBLIGATOIRES PAR NŒUD

Champ	Type	Obligatoire	Description
nodeId	uint	✅ Oui	Identifiant unique du nœud (>0)
componentType	string	✅ Oui	Type de composant (ex: "Identity_data")
componentName	string	✅ Oui	Nom lisible du composant (ex: "Identity")
position	{x, y, z}	✅ Oui	Position dans le canvas (en pixels)
size	{x, y, z}	✅ Oui	Taille du nœud visuel (120x60 standard)
enabled	bool	✅ Oui	Nœud actif/inactif
selected	bool	✅ Oui	État de sélection (false au chargement)
properties	object	✅ Oui	Propriétés du composant (map clé-valeur)

---
📊 STRUCTURE GLOBALE


{
  "schema_version": 4,           ← IMPORTANT : v4 pour Phase 3+
  "type": "<NomType>",           ← Doit correspondre à blueprintType
  "blueprintType": "EntityPrefab", ← CRITIQUE : détection de type
  "name": "<Nom>",
  "description": "<Description>",
  "metadata": { ... },
  "data": {
    "prefabName": "<Nom>",
    "nodes": [ ... ],            ← Array de nœuds
    "connections": [ ... ],      ← Array de connexions
    "canvasState": { ... }       ← État du canvas
  }
}

Exemple guard.json
{
  "schema_version": 4,
  "type": "Guard",
  "blueprintType": "EntityPrefab",
  "name": "Guard",
  "description": "Guard NPC with full AI combat capabilities",
  "metadata": {
    "author": "Atlasbruce",
    "created": "2026-01-08T10:00:00Z",
    "lastModified": "2026-01-08T10:00:00Z",
    "tags": ["Guard", "NPC", "Combat", "AI"]
  },
  "data": {
    "prefabName": "Guard",
    "nodes": [
      {
        "nodeId": 1,
        "componentType": "Identity_data",
        "componentName": "Identity",
        "position": { "x": 0.0, "y": 0.0, "z": 0.0 },
        "size": { "x": 120.0, "y": 60.0, "z": 0.0 },
        "enabled": true,
        "selected": false,
        "properties": {
          "name": "Guard_{id}",
          "tag": "Guard",
          "entityType": "Guard"
        }
      },
      {
        "nodeId": 2,
        "componentType": "Movement_data",
        "componentName": "Movement",
        "position": { "x": 140.0, "y": 0.0, "z": 0.0 },
        "size": { "x": 120.0, "y": 60.0, "z": 0.0 },
        "enabled": true,
        "selected": false,
        "properties": {
          "speed": 90.0,
          "acceleration": 450.0
        }
      },
      {
        "nodeId": 3,
        "componentType": "VisualSprite_data",
        "componentName": "Sprite",
        "position": { "x": 280.0, "y": 0.0, "z": 0.0 },
        "size": { "x": 120.0, "y": 60.0, "z": 0.0 },
        "enabled": true,
        "selected": false,
        "properties": {
          "spritePath": "./Resources/Sprites/guard.png",
          "width": 32,
          "height": 32,
          "layer": 1
        }
      },
      {
        "nodeId": 4,
        "componentType": "Health_data",
        "componentName": "Health",
        "position": { "x": 420.0, "y": 0.0, "z": 0.0 },
        "size": { "x": 120.0, "y": 60.0, "z": 0.0 },
        "enabled": true,
        "selected": false,
        "properties": {
          "maxHealth": 80,
          "currentHealth": 80,
          "invulnerable": false
        }
      },
      {
        "nodeId": 5,
        "componentType": "AIBlackboard_data",
        "componentName": "AIBlackboard",
        "position": { "x": 560.0, "y": 0.0, "z": 0.0 },
        "size": { "x": 120.0, "y": 60.0, "z": 0.0 },
        "enabled": true,
        "selected": false,
        "properties": {
          "initialized": true,
          "patrolRoute": "GuardRoute_A"
        }
      },
      {
        "nodeId": 6,
        "componentType": "BehaviorTreeRuntime_data",
        "componentName": "BehaviorTree",
        "position": { "x": 700.0, "y": 0.0, "z": 0.0 },
        "size": { "x": 120.0, "y": 60.0, "z": 0.0 },
        "enabled": true,
        "selected": false,
        "properties": {
          "AITreePath": "Blueprints/AI/guardV2_ai.json",
          "active": true
        }
      }
    ],
    "connections": [
      {
        "sourceNodeId": 1,
        "targetNodeId": 2
      },
      {
        "sourceNodeId": 2,
        "targetNodeId": 3
      },
      {
        "sourceNodeId": 1,
        "targetNodeId": 4
      },
      {
        "sourceNodeId": 1,
        "targetNodeId": 5
      },
      {
        "sourceNodeId": 5,
        "targetNodeId": 6
      }
    ],
    "canvasState": {
      "zoom": 1.0,
      "offsetX": 0.0,
      "offsetY": 0.0
    }
  }
}

🎯 VALIDATION CHECKLIST
•	✅ schema_version: 4
•	✅ blueprintType: "EntityPrefab" (pas "EntityPrefab")
•	✅ Chaque nœud a nodeId unique et > 0
•	✅ Chaque nœud a componentType (type composant)
•	✅ Chaque nœud a position et size (pour rendu canvas)
•	✅ Chaque nœud a properties objet (peut être vide {})
•	✅ connections est un array de pairs source→target
•	✅ Pas de "components" plat (ancien format BT)