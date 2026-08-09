Cahier des charges et spécification — Animation Graph Blueprint pour Olympe
1. Contexte et objectif
Le moteur Olympe dispose déjà d’un Blueprint Editor structuré autour d’un framework de graphes réutilisable, de tabs, d’un asset browser à gauche, d’un canvas central, d’un inspecteur à droite, et d’un panneau de logs / vérification en bas de la colonne gauche.
L’objectif de ce document est de définir l’intégration d’un nouveau type de graphe dédié à l’animation, sans réinventer les systèmes ou l’architecture UI existants.

Le système cible doit permettre :

la création d’un Animation Graph
le chargement d’une banque d’animations TSX
la génération automatique de nodes d’animation à partir de cette banque
la construction d’un graphe de states et transitions
l’édition de propriétés au niveau :
graphe global
node d’animation
transition
événement / marker
la prévisualisation de l’animation
la simulation de son exécution
la vérification de cohérence
la sauvegarde dans un fichier *.ani.json
2. Références d’architecture existante à respecter
Le design doit s’appuyer sur l’existant du Blueprint Editor :

GraphEditorBase
IGraphRenderer
TabManager
BlueprintEditorGUI::RenderFixedLayout()
colonne gauche = asset browser + output / verification
zone centrale = canvas de graphe
zone droite = inspector contextuel
modals framework centralisés
logique de chargement / sauvegarde déjà présente sur les graphes existants
Contraintes générales
Il est interdit de :

créer une nouvelle fenêtre principale dédiée
recréer un file browser indépendant
introduire un second framework de panels
dupliquer le système de logs / verification
contourner TabManager
casser le fonctionnement des autres graphes
Le nouveau graphe doit être une extension du système existant, pas un système parallèle.

3. Intégration UI / layout existant
3.1 Layout de référence
Le layout existant observé dans l’éditeur Blueprint est structuré ainsi :

colonne gauche
asset browser / file loader
panneau de verification / output
zone centrale
canvas du graphe
colonne droite
inspector / propriétés / palettes contextuelles
zone basse optionnelle
panels spécifiques au graphe courant, si nécessaires
3.2 Règles d’intégration
Asset browser
Le graphe Animation doit être visible dans le file loader au même titre que les autres graphes.

Il doit afficher :

les fichiers *.ani.json
les banques TSX associées
éventuellement les assets de preview si nécessaires
Le mécanisme de scan doit :

réutiliser le backend de découverte d’assets déjà en place
scanner récursivement Gamedata/Animation
exposer les fichiers dans le file picker / asset browser existant
Output / verification panel
Le panneau de gauche doit afficher :

les logs de Verify
les logs de Run
les erreurs de chargement
les warnings de configuration
les messages d’événements simulés avec timestamp
Aucun second panneau de logs ne doit être introduit.

Canvas central
Le canvas central doit conserver le même modèle :

pan
zoom
sélection
drag & drop
liens entre nodes
contexte ImGui / ImNodes existant
Le graphe Animation doit être un nouveau renderer compatible avec le framework actuel.

Inspector de droite
Le panneau droit doit être utilisé pour afficher :

propriétés globales du graphe
propriétés du node sélectionné
propriétés de la transition sélectionnée
propriétés de l’event sélectionné
paramètres de preview
paramètres de timeline
Ce panneau doit être contextuel et réutiliser les patterns déjà présents dans les autres graphes.

Timeline panel
Un Animation Timeline panel doit être ajouté en complément du canvas.

Il doit :

s’afficher en bas ou dans une zone dédiée du renderer Animation
rester intégré au cycle de rendu du tab courant
être synchronisé avec la sélection de node
permettre l’édition des markers d’événements
être compatible avec le preview viewer
4. Modèle fonctionnel
4.1 Sources de données
Le système repose sur 3 couches :

TSX Tiled
banque visuelle de clips / frames
Animation Graph Blueprint
logique états / transitions / événements
Prefab / Runtime
liaison d’une entité à un graphe d’animation
4.2 Fichier de graphe
Le graphe Animation est sauvegardé dans :

extension : .ani.json
répertoire par défaut : Gamedata/Animation
4.3 Flux principal
l’utilisateur crée un nouveau graphe Animation
il charge un TSX
le TSX alimente la palette de nodes
il crée des états d’animation
il relie les états par des transitions
il ajoute des markers temporels
il prévisualise et simule
il vérifie le graphe
il sauvegarde le graphe
5. Définition des données éditables
5.1 Propriétés globales du graphe
Propriété	Type	Description / usage attendu	Valeur par défaut	Obligatoire
graph_id	string	Identifiant unique du graphe, utilisé en sérialisation et références internes	généré automatiquement	Oui
graph_name	string	Nom affiché dans l’éditeur et dans les listes d’assets	nom du fichier sans extension	Oui
graph_description	string	Description métier du graphe	vide	Non
version	int	Version de schéma du fichier .ani.json	1	Oui
animation_bank_ref	string	Chemin du TSX source utilisé pour peupler la palette et résoudre les clips	vide	Oui
default_state_id	string	État d’entrée lors du chargement runtime	vide	Oui
default_direction_mode	enum(1/4/8)	Détermine la logique de direction des clips et des transitions	8	Non
default_fps	int	FPS fallback si un clip ou state n’en précise pas	12	Non
default_loop	bool	Comportement de boucle par défaut pour les states	true	Non
allow_any_state	bool	Active les transitions globales AnyState	true	Non
preview_character_id	string	Identité du personnage utilisé pour la prévisualisation	vide	Non
preview_root_motion_mode	enum	Mode de preview du root motion	none	Non
timeline_scale	float	Zoom temporel initial de la timeline	1.0	Non
timeline_snap_enabled	bool	Active l’accrochage à la frame / au temps	true	Non
timeline_snap_value	int	Pas de snap en ms ou en frame selon le mode	1	Non
authoring_notes	string	Notes de production / documentation interne	vide	Non
5.2 Propriétés d’un Animation State Node
Propriété	Type	Description / usage attendu	Valeur par défaut	Obligatoire
state_id	string	Identifiant unique du state pour les transitions et la sérialisation	généré automatiquement	Oui
display_name	string	Nom affiché sur le node	nom du clip	Oui
clip_id	string	Référence au clip issu du TSX	vide	Oui
anim_group	string	Groupe fonctionnel du clip (idle, walk, attack, etc.)	vide	Non
direction_mode	enum(1/4/8)	Nombre de directions supportées par le state	hérité du graphe	Non
direction	enum/string	Direction active du clip (n, ne, e, etc.)	vide	Non
loop	bool	Boucle de lecture du clip	hérité du graphe	Non
fps_override	int	FPS spécifique à ce state	0 = hérité	Non
speed_multiplier	float	Multiplicateur de vitesse de lecture	1.0	Non
exit_time	float	Point temporel à partir duquel une transition peut se déclencher	0.0	Non
can_interrupt	bool	Autorise l’interruption de ce state	true	Non
interrupt_priority	int	Priorité locale si plusieurs transitions sont possibles	0	Non
blend_in_ms	int	Temps d’entrée visuelle	0	Non
blend_out_ms	int	Temps de sortie visuelle	0	Non
root_motion	bool	Active le déplacement porté par l’animation	false	Non
play_on_enter	bool	Lance la lecture dès l’entrée dans l’état	true	Non
stop_on_exit	bool	Stoppe la lecture lors de la sortie	true	Non
tags	string[]	Métadonnées de catégorisation	vide	Non
preview_start_frame	int	Frame initiale pour le preview	0	Non
Usage attendu des propriétés de node
Le node doit rester lisible visuellement avec uniquement :

display_name
clip_id
direction
loop
présence d’événements
état de preview actif
Les autres paramètres doivent être édités dans l’inspector.

5.3 Propriétés d’une Transition Link
Propriété	Type	Description / usage attendu	Valeur par défaut	Obligatoire
transition_id	string	Identifiant unique de transition	généré automatiquement	Oui
from_state	string	État source	source de la liaison	Oui
to_state	string	État cible	cible de la liaison	Oui
condition_expr	string	Expression logique évaluée au runtime	vide	Oui
priority	int	Ordre de résolution de la transition	0	Non
can_interrupt	bool	Transition prioritaire / absolue	false	Non
blend_in_ms	int	Temps de fondu à l’entrée	0	Non
blend_out_ms	int	Temps de fondu à la sortie	0	Non
cooldown_ms	int	Anti-spam de transition	0	Non
exit_time_threshold	float	Seuil temporel minimum pour autoriser la transition	0.0	Non
sync_offset	float	Décalage de synchronisation entre états	0.0	Non
use_time_sync	bool	Active la synchronisation temporelle	false	Non
allow_same_state	bool	Autorise la transition vers le même état	false	Non
debug_label	string	Libellé affiché dans l’éditeur	vide	Non
Usage attendu des propriétés de liaison
La liaison représente une vraie transition.
Elle doit donc porter la logique de passage entre states, et non un simple lien visuel.

5.4 Propriétés d’un Event Marker
Propriété	Type	Description / usage attendu	Valeur par défaut	Obligatoire
event_id	string	Identifiant unique de l’événement	généré automatiquement	Oui
state_id	string	State porteur de l’événement	état courant	Oui
event_name	string	Nom métier de l’événement	vide	Oui
event_type	enum	Type d’événement : audio, gameplay, spawn, custom	custom	Oui
frame_index	int	Position temporelle en frame	0	Oui
time_ms	int	Position temporelle en ms	calculée depuis frame	Non
payload_json	json/string	Paramètres spécifiques à l’événement	vide	Non
target	string	Cible de l’action (message, entité, système)	vide	Non
repeat	bool	Répétition éventuelle de l’événement	false	Non
enabled	bool	Active/désactive le marker	true	Non
color	string/color	Couleur d’affichage dans la timeline	couleur du type	Non
snap_to_frame	bool	Force l’alignement sur la frame	true	Non
Exemples d’usage
audio : jouer un bruitage de pas, d’impact, de tir, etc.
gameplay : envoyer un message logique, activer une action, déclencher un état
spawn : créer une entité ou un effet
custom : extension projet
6. Organisation des panneaux d’édition
6.1 Graph Properties Panel
Contenu :

nom du graphe
description
version
banque TSX associée
état initial
direction par défaut
FPS par défaut
options de preview
options de timeline
authoring notes
6.2 Node Properties Panel
Contenu :

identifiant du state
clip lié
direction
loop
speed multiplier
blend
interruption
tags
paramètres de preview
6.3 Transition Properties Panel
Contenu :

source / cible
condition
priorité
blending
cooldown
synchronisation
mode interruptible
6.4 Event Properties Panel
Contenu :

type
frame / time
payload
target
enabled
couleur
snap
6.5 Timeline Panel
Contenu :

piste d’animation
pistes d’événements
markers repositionnables
sélection des markers
zoom temporel
snap
playback controls
viewer de prévisualisation
logs des événements déclenchés
7. Spécification du panel Timeline
Le panel Timeline est un outil d’édition et de preview contextualisé sur le node sélectionné.

Fonctions attendues
afficher la durée de l’animation
représenter les frames / timecodes
afficher les markers d’événements
permettre le déplacement d’un marker
permettre l’ajout / suppression / duplication de markers
afficher un viewer à droite
synchroniser la lecture avec le graphe sélectionné
afficher les erreurs de configuration liées au state courant
Contrôles de lecture
Le viewer doit proposer :

Play
Pause
Stop
Loop
scrubbing temporel
avancée frame par frame si possible
Logs
Les événements déclenchés doivent être loggés avec timestamp dans l’output panel, par exemple :

[00:01.250] audio: play_sound footstep_grass_01
[00:01.300] gameplay: emit_message activate_attack
[00:01.500] spawn: entity=slash_fx
8. Commande Verify
Objectif
Vérification statique de cohérence du graphe.

Contrôles attendus
banque TSX présente
état initial défini
states liés à un clip valide
transitions avec source et cible valides
conditions syntaxiquement valides
événements avec frame valide
absence de doublons d’identifiants
compatibilité directionnelle
graph non vide si requis
cohérence des liens et des références
Résultat
Le panneau output doit afficher :

statut global
erreurs
warnings
statistiques de graphe
Exemple de message Verify
Graph valid
Missing clip for state walk_ne
Transition idle -> walk has invalid condition_expr
Event footstep at frame 45 is outside clip duration
9. Commande Run
Objectif
Simulation d’exécution du graphe.

Contrôles attendus
chargement du graphe
démarrage au default_state
progression temporelle
évaluation des transitions
déclenchement des événements
logs détaillés dans l’output
visualisation de la lecture dans le viewer
Résultats attendus
état courant
transition prise
event déclenché
erreur runtime si données manquantes
trace de simulation
Différence avec Verify
Verify : cohérence structurelle
Run : simulation fonctionnelle
10. Sauvegarde et chargement
Format de sauvegarde
fichier : .ani.json
dossier par défaut : Gamedata/Animation
Chargement à l’ouverture de l’éditeur
Le file loader doit :

scanner récursivement Gamedata/Animation
afficher les fichiers existants
permettre le filtrage par type
distinguer les graphes Animation des autres graphes
Règle
Le scan et l’affichage doivent réutiliser le mécanisme existant d’asset browsing / loading.

11. Graphe de dépendances fonctionnelles
Mermaid
flowchart LR
    TSX[Tiled TSX Animation Bank]
    IMPORT[TSX Importer]
    BANK[Animation Bank Asset]
    AG[Animation Graph Asset .ani.json]
    GUI[Blueprint Editor GUI]
    LOADER[Asset Browser / File Loader]
    GRAPH[Animation Graph Renderer]
    INSPECT[Properties Inspector]
    TIMELINE[Animation Timeline Panel]
    PREVIEW[Preview Viewer]
    VERIFY[Verify Command]
    RUN[Run Command]
    LOGS[Output / Verification Panel]
    PREFAB[Prefab Binding]
    ECS[Runtime ECS Animation System]
    RENDER[Renderer]

    TSX --> IMPORT
    IMPORT --> BANK
    BANK --> AG
    GUI --> LOADER
    LOADER --> AG
    AG --> GRAPH
    AG --> INSPECT
    AG --> TIMELINE
    AG --> PREVIEW
    AG --> PREFAB
    VERIFY --> LOGS
    RUN --> LOGS
    RUN --> PREVIEW
    PREFAB --> ECS
    ECS --> RENDER
    GRAPH --> INSPECT
    GRAPH --> TIMELINE
    GRAPH --> VERIFY
    GRAPH --> RUN
12. Contraintes d’implémentation
le renderer Animation doit dériver du framework existant
il doit respecter IGraphRenderer
il doit être ouvert / fermé via TabManager
il doit réutiliser la logique de layout actuelle
il doit s’intégrer dans l’asset browser / output panel existant
il ne doit pas introduire de système de propriétés parallèle si l’inspector peut être étendu
il ne doit pas casser les autres graphes déjà supportés
13. Phasage recommandé
Phase 1 — Données et import
définir le format .ani.json
définir le modèle de données
intégrer le scan Gamedata/Animation
lire le TSX et extraire les clips
Phase 2 — Renderer et palette
créer le renderer Animation
afficher les clips en palette
créer les nodes depuis la palette
gérer sélection et inspection
Phase 3 — Transitions et propriétés
créer l’édition des transitions
ajouter les panneaux de propriétés
gérer la sérialisation complète
Phase 4 — Timeline et preview
implémenter la timeline
ajouter les markers
intégrer le viewer
logguer les events
Phase 5 — Verify et Run
ajouter la cohérence statique
ajouter la simulation de parcours
brancher les logs dans l’output panel
Phase 6 — Intégration runtime
liaison prefab
runtime ECS
chargement / exécution en jeu
14. Critères d’acceptation
Le système sera considéré comme conforme si :

un graphe Animation peut être créé dans le Blueprint Editor
un TSX peut être chargé comme banque d’animation
les clips TSX apparaissent dans la palette
les nodes d’animation peuvent être posés et édités
les transitions peuvent être configurées
les events peuvent être créés dans la timeline
le graphe peut être sauvegardé en .ani.json
le file loader retrouve les graphes dans Gamedata/Animation
Verify détecte les erreurs de cohérence
Run simule le comportement et loggue les événements
l’intégration UI respecte le layout existant sans réinvention de système
Si tu veux, je peux maintenant faire le document suivant :
une version “spécification technique d’implémentation” encore plus opérationnelle, avec :

classes à créer,
responsabilités par module,
API attendues,
structure JSON proposée,
et points d’intégration précis dans le code existant.
je veux bien que tu produises le document au format Markdown exportable. et oui, comme tu le proposes je veux bien aussi que tu rédiges le document "Spécifications Techniques d'implémentation (opérationnel) au format Markdown également
Voici les deux documents, en Markdown exportable.

1) animation_graph_blueprint_specification.md
Markdown
# Cahier des charges et spécification — Animation Graph Blueprint pour Olympe

## 1. Contexte et objectif

Le moteur Olympe dispose déjà d’un **Blueprint Editor** structuré autour d’un framework de graphes réutilisable, de tabs, d’un asset browser à gauche, d’un canvas central, d’un inspecteur à droite, et d’un panneau de logs / vérification en bas de la colonne gauche.  
L’objectif de ce document est de définir l’intégration d’un **nouveau type de graphe** dédié à l’animation, sans réinventer les systèmes ou l’architecture UI existants.

Le système cible doit permettre :

- la création d’un **Animation Graph**
- le chargement d’une **banque d’animations TSX**
- la génération automatique de nodes d’animation à partir de cette banque
- la construction d’un graphe de **states et transitions**
- l’édition de propriétés au niveau :
  - graphe global
  - node d’animation
  - transition
  - événement / marker
- la prévisualisation de l’animation
- la simulation de son exécution
- la vérification de cohérence
- la sauvegarde dans un fichier `*.ani.json`

---

## 2. Références d’architecture existante à respecter

Le design doit s’appuyer sur l’existant du Blueprint Editor :

- `GraphEditorBase`
- `IGraphRenderer`
- `TabManager`
- `BlueprintEditorGUI::RenderFixedLayout()`
- colonne gauche = asset browser + output / verification
- zone centrale = canvas de graphe
- zone droite = inspector contextuel
- modals framework centralisés
- logique de chargement / sauvegarde déjà présente sur les graphes existants

### Contraintes générales
Il est interdit de :

- créer une nouvelle fenêtre principale dédiée
- recréer un file browser indépendant
- introduire un second framework de panels
- dupliquer le système de logs / verification
- contourner `TabManager`
- casser le fonctionnement des autres graphes

Le nouveau graphe doit être une **extension du système existant**, pas un système parallèle.

---

## 3. Intégration UI / layout existant

### 3.1 Layout de référence
Le layout existant observé dans l’éditeur Blueprint est structuré ainsi :

- **colonne gauche**
  - asset browser / file loader
  - panneau de verification / output
- **zone centrale**
  - canvas du graphe
- **colonne droite**
  - inspector / propriétés / palettes contextuelles
- **zone basse optionnelle**
  - panels spécifiques au graphe courant, si nécessaires

### 3.2 Règles d’intégration

#### Asset browser
Le graphe Animation doit être visible dans le file loader au même titre que les autres graphes.

Il doit afficher :

- les fichiers `*.ani.json`
- les banques TSX associées
- éventuellement les assets de preview si nécessaires

Le mécanisme de scan doit :

- réutiliser le backend de découverte d’assets déjà en place
- scanner récursivement `Gamedata/Animation`
- exposer les fichiers dans le file picker / asset browser existant

#### Output / verification panel
Le panneau de gauche doit afficher :

- les logs de `Verify`
- les logs de `Run`
- les erreurs de chargement
- les warnings de configuration
- les messages d’événements simulés avec timestamp

Aucun second panneau de logs ne doit être introduit.

#### Canvas central
Le canvas central doit conserver le même modèle :

- pan
- zoom
- sélection
- drag & drop
- liens entre nodes
- contexte ImGui / ImNodes existant

Le graphe Animation doit être un nouveau renderer compatible avec le framework actuel.

#### Inspector de droite
Le panneau droit doit être utilisé pour afficher :

- propriétés globales du graphe
- propriétés du node sélectionné
- propriétés de la transition sélectionnée
- propriétés de l’event sélectionné
- paramètres de preview
- paramètres de timeline

Ce panneau doit être contextuel et réutiliser les patterns déjà présents dans les autres graphes.

#### Timeline panel
Un **Animation Timeline panel** doit être ajouté en complément du canvas.

Il doit :

- s’afficher en bas ou dans une zone dédiée du renderer Animation
- rester intégré au cycle de rendu du tab courant
- être synchronisé avec la sélection de node
- permettre l’édition des markers d’événements
- être compatible avec le preview viewer

---

## 4. Modèle fonctionnel

### 4.1 Sources de données
Le système repose sur 3 couches :

1. **TSX Tiled**
   - banque visuelle de clips / frames
2. **Animation Graph Blueprint**
   - logique états / transitions / événements
3. **Prefab / Runtime**
   - liaison d’une entité à un graphe d’animation

### 4.2 Fichier de graphe
Le graphe Animation est sauvegardé dans :

- extension : `.ani.json`
- répertoire par défaut : `Gamedata/Animation`

### 4.3 Flux principal
1. l’utilisateur crée un nouveau graphe Animation
2. il charge un TSX
3. le TSX alimente la palette de nodes
4. il crée des états d’animation
5. il relie les états par des transitions
6. il ajoute des markers temporels
7. il prévisualise et simule
8. il vérifie le graphe
9. il sauvegarde le graphe

---

## 5. Définition des données éditables

## 5.1 Propriétés globales du graphe

| Propriété | Type | Description / usage attendu | Valeur par défaut | Obligatoire |
|---|---:|---|---|---|
| `graph_id` | string | Identifiant unique du graphe, utilisé en sérialisation et références internes | généré automatiquement | Oui |
| `graph_name` | string | Nom affiché dans l’éditeur et dans les listes d’assets | nom du fichier sans extension | Oui |
| `graph_description` | string | Description métier du graphe | vide | Non |
| `version` | int | Version de schéma du fichier `.ani.json` | 1 | Oui |
| `animation_bank_ref` | string | Chemin du TSX source utilisé pour peupler la palette et résoudre les clips | vide | Oui |
| `default_state_id` | string | État d’entrée lors du chargement runtime | vide | Oui |
| `default_direction_mode` | enum(1/4/8) | Détermine la logique de direction des clips et des transitions | 8 | Non |
| `default_fps` | int | FPS fallback si un clip ou state n’en précise pas | 12 | Non |
| `default_loop` | bool | Comportement de boucle par défaut pour les states | true | Non |
| `allow_any_state` | bool | Active les transitions globales `AnyState` | true | Non |
| `preview_character_id` | string | Identité du personnage utilisé pour la prévisualisation | vide | Non |
| `preview_root_motion_mode` | enum | Mode de preview du root motion | `none` | Non |
| `timeline_scale` | float | Zoom temporel initial de la timeline | 1.0 | Non |
| `timeline_snap_enabled` | bool | Active l’accrochage à la frame / au temps | true | Non |
| `timeline_snap_value` | int | Pas de snap en ms ou en frame selon le mode | 1 | Non |
| `authoring_notes` | string | Notes de production / documentation interne | vide | Non |

---

## 5.2 Propriétés d’un Animation State Node

| Propriété | Type | Description / usage attendu | Valeur par défaut | Obligatoire |
|---|---:|---|---|---|
| `state_id` | string | Identifiant unique du state pour les transitions et la sérialisation | généré automatiquement | Oui |
| `display_name` | string | Nom affiché sur le node | nom du clip | Oui |
| `clip_id` | string | Référence au clip issu du TSX | vide | Oui |
| `anim_group` | string | Groupe fonctionnel du clip (`idle`, `walk`, `attack`, etc.) | vide | Non |
| `direction_mode` | enum(1/4/8) | Nombre de directions supportées par le state | hérité du graphe | Non |
| `direction` | enum/string | Direction active du clip (`n`, `ne`, `e`, etc.) | vide | Non |
| `loop` | bool | Boucle de lecture du clip | hérité du graphe | Non |
| `fps_override` | int | FPS spécifique à ce state | 0 = hérité | Non |
| `speed_multiplier` | float | Multiplicateur de vitesse de lecture | 1.0 | Non |
| `exit_time` | float | Point temporel à partir duquel une transition peut se déclencher | 0.0 | Non |
| `can_interrupt` | bool | Autorise l’interruption de ce state | true | Non |
| `interrupt_priority` | int | Priorité locale si plusieurs transitions sont possibles | 0 | Non |
| `blend_in_ms` | int | Temps d’entrée visuelle | 0 | Non |
| `blend_out_ms` | int | Temps de sortie visuelle | 0 | Non |
| `root_motion` | bool | Active le déplacement porté par l’animation | false | Non |
| `play_on_enter` | bool | Lance la lecture dès l’entrée dans l’état | true | Non |
| `stop_on_exit` | bool | Stoppe la lecture lors de la sortie | true | Non |
| `tags` | string[] | Métadonnées de catégorisation | vide | Non |
| `preview_start_frame` | int | Frame initiale pour le preview | 0 | Non |

### Usage attendu des propriétés de node
Le node doit rester lisible visuellement avec uniquement :

- `display_name`
- `clip_id`
- `direction`
- `loop`
- présence d’événements
- état de preview actif

Les autres paramètres doivent être édités dans l’inspector.

---

## 5.3 Propriétés d’une Transition Link

| Propriété | Type | Description / usage attendu | Valeur par défaut | Obligatoire |
|---|---:|---|---|---|
| `transition_id` | string | Identifiant unique de transition | généré automatiquement | Oui |
| `from_state` | string | État source | source de la liaison | Oui |
| `to_state` | string | État cible | cible de la liaison | Oui |
| `condition_expr` | string | Expression logique évaluée au runtime | vide | Oui |
| `priority` | int | Ordre de résolution de la transition | 0 | Non |
| `can_interrupt` | bool | Transition prioritaire / absolue | false | Non |
| `blend_in_ms` | int | Temps de fondu à l’entrée | 0 | Non |
| `blend_out_ms` | int | Temps de fondu à la sortie | 0 | Non |
| `cooldown_ms` | int | Anti-spam de transition | 0 | Non |
| `exit_time_threshold` | float | Seuil temporel minimum pour autoriser la transition | 0.0 | Non |
| `sync_offset` | float | Décalage de synchronisation entre états | 0.0 | Non |
| `use_time_sync` | bool | Active la synchronisation temporelle | false | Non |
| `allow_same_state` | bool | Autorise la transition vers le même état | false | Non |
| `debug_label` | string | Libellé affiché dans l’éditeur | vide | Non |

### Usage attendu des propriétés de liaison
La liaison représente une vraie **transition**.  
Elle doit donc porter la logique de passage entre states, et non un simple lien visuel.

---

## 5.4 Propriétés d’un Event Marker

| Propriété | Type | Description / usage attendu | Valeur par défaut | Obligatoire |
|---|---:|---|---|---|
| `event_id` | string | Identifiant unique de l’événement | généré automatiquement | Oui |
| `state_id` | string | State porteur de l’événement | état courant | Oui |
| `event_name` | string | Nom métier de l’événement | vide | Oui |
| `event_type` | enum | Type d’événement : audio, gameplay, spawn, custom | custom | Oui |
| `frame_index` | int | Position temporelle en frame | 0 | Oui |
| `time_ms` | int | Position temporelle en ms | calculée depuis frame | Non |
| `payload_json` | json/string | Paramètres spécifiques à l’événement | vide | Non |
| `target` | string | Cible de l’action (message, entité, système) | vide | Non |
| `repeat` | bool | Répétition éventuelle de l’événement | false | Non |
| `enabled` | bool | Active/désactive le marker | true | Non |
| `color` | string/color | Couleur d’affichage dans la timeline | couleur du type | Non |
| `snap_to_frame` | bool | Force l’alignement sur la frame | true | Non |

### Exemples d’usage
- `audio` : jouer un bruitage de pas, d’impact, de tir, etc.
- `gameplay` : envoyer un message logique, activer une action, déclencher un état
- `spawn` : créer une entité ou un effet
- `custom` : extension projet

---

## 6. Organisation des panneaux d’édition

## 6.1 Graph Properties Panel
Contenu :

- nom du graphe
- description
- version
- banque TSX associée
- état initial
- direction par défaut
- FPS par défaut
- options de preview
- options de timeline
- authoring notes

## 6.2 Node Properties Panel
Contenu :

- identifiant du state
- clip lié
- direction
- loop
- speed multiplier
- blend
- interruption
- tags
- paramètres de preview

## 6.3 Transition Properties Panel
Contenu :

- source / cible
- condition
- priorité
- blending
- cooldown
- synchronisation
- mode interruptible

## 6.4 Event Properties Panel
Contenu :

- type
- frame / time
- payload
- target
- enabled
- couleur
- snap

## 6.5 Timeline Panel
Contenu :

- piste d’animation
- pistes d’événements
- markers repositionnables
- sélection des markers
- zoom temporel
- snap
- playback controls
- viewer de prévisualisation
- logs des événements déclenchés

---

## 7. Spécification du panel Timeline

Le panel Timeline est un outil d’édition et de preview contextualisé sur le node sélectionné.

### Fonctions attendues
- afficher la durée de l’animation
- représenter les frames / timecodes
- afficher les markers d’événements
- permettre le déplacement d’un marker
- permettre l’ajout / suppression / duplication de markers
- afficher un viewer à droite
- synchroniser la lecture avec le graphe sélectionné
- afficher les erreurs de configuration liées au state courant

### Contrôles de lecture
Le viewer doit proposer :

- `Play`
- `Pause`
- `Stop`
- `Loop`
- scrubbing temporel
- avancée frame par frame si possible

### Logs
Les événements déclenchés doivent être loggés avec timestamp dans l’output panel, par exemple :

- `[00:01.250] audio: play_sound footstep_grass_01`
- `[00:01.300] gameplay: emit_message activate_attack`
- `[00:01.500] spawn: entity=slash_fx`

---

## 8. Commande Verify

### Objectif
Vérification statique de cohérence du graphe.

### Contrôles attendus
- banque TSX présente
- état initial défini
- states liés à un clip valide
- transitions avec source et cible valides
- conditions syntaxiquement valides
- événements avec frame valide
- absence de doublons d’identifiants
- compatibilité directionnelle
- graph non vide si requis
- cohérence des liens et des références

### Résultat
Le panneau output doit afficher :

- statut global
- erreurs
- warnings
- statistiques de graphe

### Exemple de message Verify
- `Graph valid`
- `Missing clip for state walk_ne`
- `Transition idle -> walk has invalid condition_expr`
- `Event footstep at frame 45 is outside clip duration`

---

## 9. Commande Run

### Objectif
Simulation d’exécution du graphe.

### Contrôles attendus
- chargement du graphe
- démarrage au `default_state`
- progression temporelle
- évaluation des transitions
- déclenchement des événements
- logs détaillés dans l’output
- visualisation de la lecture dans le viewer

### Résultats attendus
- état courant
- transition prise
- event déclenché
- erreur runtime si données manquantes
- trace de simulation

### Différence avec Verify
- **Verify** : cohérence structurelle
- **Run** : simulation fonctionnelle

---

## 10. Sauvegarde et chargement

### Format de sauvegarde
- fichier : `.ani.json`
- dossier par défaut : `Gamedata/Animation`

### Chargement à l’ouverture de l’éditeur
Le file loader doit :

- scanner récursivement `Gamedata/Animation`
- afficher les fichiers existants
- permettre le filtrage par type
- distinguer les graphes Animation des autres graphes

### Règle
Le scan et l’affichage doivent réutiliser le mécanisme existant d’asset browsing / loading.

---

## 11. Graphe de dépendances fonctionnelles

```mermaid
flowchart LR
    TSX[Tiled TSX Animation Bank]
    IMPORT[TSX Importer]
    BANK[Animation Bank Asset]
    AG[Animation Graph Asset .ani.json]
    GUI[Blueprint Editor GUI]
    LOADER[Asset Browser / File Loader]
    GRAPH[Animation Graph Renderer]
    INSPECT[Properties Inspector]
    TIMELINE[Animation Timeline Panel]
    PREVIEW[Preview Viewer]
    VERIFY[Verify Command]
    RUN[Run Command]
    LOGS[Output / Verification Panel]
    PREFAB[Prefab Binding]
    ECS[Runtime ECS Animation System]
    RENDER[Renderer]

    TSX --> IMPORT
    IMPORT --> BANK
    BANK --> AG
    GUI --> LOADER
    LOADER --> AG
    AG --> GRAPH
    AG --> INSPECT
    AG --> TIMELINE
    AG --> PREVIEW
    AG --> PREFAB
    VERIFY --> LOGS
    RUN --> LOGS
    RUN --> PREVIEW
    PREFAB --> ECS
    ECS --> RENDER
    GRAPH --> INSPECT
    GRAPH --> TIMELINE
    GRAPH --> VERIFY
    GRAPH --> RUN
12. Contraintes d’implémentation
le renderer Animation doit dériver du framework existant
il doit respecter IGraphRenderer
il doit être ouvert / fermé via TabManager
il doit réutiliser la logique de layout actuelle
il doit s’intégrer dans l’asset browser / output panel existant
il ne doit pas introduire de système de propriétés parallèle si l’inspector peut être étendu
il ne doit pas casser les autres graphes déjà supportés
13. Phasage recommandé
Phase 1 — Données et import
définir le format .ani.json
définir le modèle de données
intégrer le scan Gamedata/Animation
lire le TSX et extraire les clips
Phase 2 — Renderer et palette
créer le renderer Animation
afficher les clips en palette
créer les nodes depuis la palette
gérer sélection et inspection
Phase 3 — Transitions et propriétés
créer l’édition des transitions
ajouter les panneaux de propriétés
gérer la sérialisation complète
Phase 4 — Timeline et preview
implémenter la timeline
ajouter les markers
intégrer le viewer
logguer les events
Phase 5 — Verify et Run
ajouter la cohérence statique
ajouter la simulation de parcours
brancher les logs dans l’output panel
Phase 6 — Intégration runtime
liaison prefab
runtime ECS
chargement / exécution en jeu
14. Critères d’acceptation
Le système sera considéré comme conforme si :

un graphe Animation peut être créé dans le Blueprint Editor
un TSX peut être chargé comme banque d’animation
les clips TSX apparaissent dans la palette
les nodes d’animation peuvent être posés et édités
les transitions peuvent être configurées
les events peuvent être créés dans la timeline
le graphe peut être sauvegardé en .ani.json
le file loader retrouve les graphes dans Gamedata/Animation
Verify détecte les erreurs de cohérence
Run simule le comportement et loggue les événements
l’intégration UI respecte le layout existant sans réinvention de système
Code

-------------------------------------------------------------------------------------

# Spécifications techniques d’implémentation — Animation Graph Blueprint pour Olympe

## 1. Objectif technique

Implémenter un nouveau type de graphe Blueprint dédié à l’animation, en réutilisant au maximum :

- le framework de graph editor existant
- le système de tabs existant
- le layout ImGui existant
- le système de logs / verification existant
- les patterns de document / renderer déjà en place

Le but n’est pas de créer une nouvelle architecture, mais de **brancher un renderer Animation** sur les composants déjà disponibles.

---

## 2. Référentiel existant à réutiliser

Les éléments déjà en place à exploiter sont notamment :

- `GraphEditorBase`
- `IGraphRenderer`
- `BlueprintEditorGUI`
- `TabManager`
- `DataManager`
- `BlueprintEditor`
- `BlueprintEditorGUI::RenderFixedLayout()`
- la colonne gauche des outputs / verification
- les modals framework
- les patterns de `Load()`, `Save()`, `Render()`, `RenderFrameworkModals()`

### Règle
Toute nouvelle fonctionnalité doit d’abord vérifier si un point d’extension existant peut être utilisé.

---

## 3. Architecture cible

### 3.1 Nouveaux éléments à ajouter
Créer les briques suivantes :

- `AnimationGraphDocument`
- `AnimationGraphRenderer`
- `AnimationTimelinePanel`
- `AnimationPreviewPanel` ou composant de preview interne
- `AnimationGraphVerifier`
- `AnimationGraphRunner`
- `AnimationTSXImporter`
- `AnimationAssetFilter` / extension du loader existant

### 3.2 Éléments à étendre
Étendre si nécessaire :

- `TabManager`
- asset browser / file picker
- panneau de verification output
- panneau inspector
- système de menu “New graph”
- système de scan des dossiers de gamedata

---

## 4. Intégration dans le framework de graphes

### 4.1 Contrat `IGraphRenderer`
Le renderer Animation doit respecter le contrat suivant :

- `Render()`
- `Load(const std::string& path)`
- `Save(const std::string& path)`
- `IsDirty() const`
- `GetGraphType() const`
- `GetCurrentPath() const`
- `RenderFrameworkModals()`
- `SaveCanvasState()`
- `RestoreCanvasState()`

### 4.2 Base recommandée
Le renderer Animation doit idéalement dériver de `GraphEditorBase` pour bénéficier de :

- sélection
- pan / zoom
- modals
- toolbar
- contexte de rendu
- structure commune des interactions

### 4.3 Principe d’extension
Le renderer doit implémenter :

- la palette de clips générée depuis le TSX
- la création de `AnimationState` nodes
- les liens de transition
- l’inspector contextuel
- l’affichage de la timeline
- la simulation `Run`
- la vérification `Verify`

---

## 5. Structure des classes recommandée

## 5.1 `AnimationGraphDocument`
Responsabilités :

- stocker les données du graphe
- sérialiser / désérialiser `.ani.json`
- exposer les nodes, transitions, events
- stocker les propriétés globales
- gérer dirty state
- maintenir le lien avec la banque TSX

### API attendue
- `Load(path)`
- `Save(path)`
- `Clear()`
- `IsDirty()`
- `SetDirty(bool)`
- `GetAllStates()`
- `GetAllTransitions()`
- `GetAllEvents()`
- `GetGraphSettings()`
- `SetGraphSettings(...)`

---

## 5.2 `AnimationGraphRenderer`
Responsabilités :

- afficher le graph canvas
- recevoir les interactions utilisateur
- afficher la palette TSX
- synchroniser la sélection
- piloter l’inspector
- piloter la timeline
- exposer les actions `Verify` et `Run`

### Méthodes recommandées
- `Load()`
- `Save()`
- `RenderGraphContent()`
- `RenderTypePanels()`
- `RenderTypeSpecificToolbar()`
- `RenderVerificationLogsPanel()`
- `RenderTimelinePanel()`
- `VerifyGraph()`
- `RunGraph()`

---

## 5.3 `AnimationTimelinePanel`
Responsabilités :

- afficher la timeline
- rendre les markers d’événements
- gérer les éditions temporelles
- piloter le preview
- envoyer les logs de preview / simulation

### Fonctions nécessaires
- ajout / suppression de marker
- déplacement par drag
- snap
- playback controls
- scrubbing
- affichage de frame et timecode

---

## 5.4 `AnimationGraphVerifier`
Responsabilités :

- vérifier la cohérence statique
- retourner erreurs / warnings
- produire un rapport exploitable par l’output panel

### Vérifications recommandées
- TSX référencé valide
- clips existants
- state d’entrée présent
- transitions valides
- conditions valides
- événements valides
- absence de doublons
- cohérence de durée / frames

---

## 5.5 `AnimationGraphRunner`
Responsabilités :

- simuler un parcours d’exécution
- évaluer les conditions de transition
- déclencher les événements
- émettre des logs horodatés
- alimenter le preview

### Note
Le runner n’est pas le runtime final jeu, mais une simulation éditeur.

---

## 5.6 `AnimationTSXImporter`
Responsabilités :

- parser le TSX
- extraire les animations / clips
- lire les propriétés des tiles
- construire une banque exploitable par l’éditeur

### Données à extraire
- `clip_id`
- `anim_group`
- `direction`
- `loop`
- `fps`
- `priority`
- `interruptible`
- `blend_in_ms`
- `blend_out_ms`
- `root_motion`
- frames et durations

---

## 6. Structures de données recommandées

## 6.1 `AnimationGraphSettings`
Contient :
- identifiant
- nom
- description
- TSX source
- default state
- settings de preview
- settings de timeline
- mode de direction
- version

## 6.2 `AnimationStateData`
Contient :
- id
- clip id
- direction
- playback params
- blend params
- interruptibility
- root motion
- tags
- preview frame

## 6.3 `AnimationTransitionData`
Contient :
- source
- cible
- condition
- priorité
- cooldown
- blend
- sync params

## 6.4 `AnimationEventData`
Contient :
- id
- state id
- frame index
- type
- target
- payload
- enabled

---

## 7. Intégration dans `BlueprintEditorGUI`

### 7.1 Menu File
Ajouter une entrée :
- `New Animation Graph`

### 7.2 Ouverture de fichier
Le loader doit reconnaître :
- `*.ani.json`

et ouvrir le bon renderer.

### 7.3 Scan automatique
Au lancement de l’éditeur :

- scanner récursivement `Gamedata/Animation`
- injecter les fichiers dans le file browser existant

### 7.4 Output panel
Le panneau de gauche doit pouvoir afficher :
- logs de validation
- logs de simulation
- messages de chargement
- erreurs de preview

### 7.5 Tab management
Le graphe Animation doit être ouvert dans un tab géré par `TabManager`, comme les autres graphes.

---

## 8. Intégration dans le layout existant

### Colonne gauche
Doit continuer à afficher :
- asset browser
- output / verification

### Zone centrale
Doit afficher :
- canvas du graphe
- éventuellement la timeline si intégrée au renderer

### Colonne droite
Doit afficher :
- properties inspector
- palette contextuelle
- preview settings

### Zone basse
Peut contenir :
- timeline
- preview viewer
- logs détaillés si le panneau gauche n’est pas suffisant

### Règle d’implémentation
Aucune nouvelle architecture de docking ne doit être introduite si le layout courant couvre les besoins.

---

## 9. Vérification `Verify`

### Entrées
- graphe courant
- TSX lié
- states
- transitions
- events

### Sortie
- liste des erreurs
- liste des warnings
- résumé global
- statistiques

### Vérifications minimales
- `animation_bank_ref` non vide
- fichier TSX accessible
- `default_state_id` résolu
- chaque `clip_id` existe
- chaque transition relie deux états existants
- les conditions sont non vides si requises
- les frames d’events sont valides
- pas d’IDs dupliqués

---

## 10. Simulation `Run`

### Entrées
- graphe courant
- état initial
- conditions de simulation
- paramètres runtime de preview

### Traitement
- entrer dans l’état initial
- avancer le temps
- évaluer les transitions
- déclencher les événements
- logguer les résultats

### Sortie
- trace horodatée
- état courant
- transitions prises
- événements émis
- erreurs runtime

---

## 11. Format de sérialisation `.ani.json`

### Contenu minimal
- metadata du graphe
- référence TSX
- settings globaux
- liste des states
- liste des transitions
- liste des events
- settings preview / timeline

### Règles
- les IDs doivent être stables
- le format doit être versionné
- le chargement doit supporter les champs manquants via valeurs par défaut
- les assets référencés doivent rester résolus via chemins stables

---

## 12. Plan de mise en œuvre

### Étape 1
Créer le modèle de données et le parseur TSX.

### Étape 2
Créer le renderer Animation basé sur le framework existant.

### Étape 3
Brancher la palette TSX, les nodes et le panneau de propriétés.

### Étape 4
Ajouter la timeline et le preview.

### Étape 5
Implémenter `Verify` et `Run`.

### Étape 6
Brancher le loader de fichiers et les chemins par défaut.

### Étape 7
Préparer la liaison prefab / runtime.

---

## 13. Points d’attention

- ne pas mélanger modèle éditeur et runtime
- ne pas ajouter de logique de transition dans la vue uniquement
- ne pas dupliquer les logs de verification
- ne pas créer une UI timeline indépendante du renderer
- ne pas créer un loader parallèle au mécanisme existant
- garder la compatibilité avec les autres graphes
- privilégier les extensions réutilisables

---

## 14. Critères techniques de validation

L’implémentation est correcte si :

- le type `Animation` apparaît dans les nouveaux graphs
- le renderer charge un `.ani.json`
- le TSX alimente la palette
- les nodes et transitions sont éditables
- la timeline fonctionne
- `Verify` affiche un rapport exploitable
- `Run` simule et loggue correctement
- le file browser voit les fichiers sous `Gamedata/Animation`
- le layout reste cohérent avec l’éditeur actuel