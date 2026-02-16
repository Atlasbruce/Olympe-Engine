
🎬 ANIMATION EDITOR V1 - BRIEF
Olympe Engine - Animation Editor (F9)
Version: 1.0 (Phase 1 - MVP)
Date: 2026-02-16
Auteur: Atlasbruce
Standard: C++14 Strict (voir Source/COPILOT_CODING_RULES.md)


📋 TABLE DES MATIÈRES

Vue d'ensemble
Architecture du système
Analyse des spritesheets fournis
Structures de données
Format JSON unifié
Interface utilisateur
Implémentation technique
Intégration système
Workflow utilisateur
Plan d'implémentation
Validation et tests
Roadmap Phase 2
Annexes


🎯 VUE D'ENSEMBLE
Objectif
Créer un Animation Editor (touche F9) pour Olympe Engine permettant de:

Créer et éditer des Animation Banks (collections de séquences d'animation)
Gérer des spritesheets multiples par bank (idle, walk, run, attack, special...)
Prévisualiser les animations en temps réel avec contrôles playback
Sauvegarder au format JSON unifié avec rétrocompatibilité
Intégrer avec les systèmes Prefab, AI/BT, et Animation Runtime
Principes de Conception

Une Bank = Plusieurs SpritesheetsExemple: thesee bank contient:thesee_idle.png (128 frames, 8 cols × 16 rows)
thesee_walk.png (128 frames, 12 cols × ~11 rows)
thesee_run.png (128 frames, 8 cols × 16 rows)
thesee_hit.png (112 frames, 7 cols × 16 rows)
Séquences = Ranges de FramesChaque séquence référence un spritesheet spécifique
Définit un range de frames (start, count)
Propriétés: loop, speed, frameDuration, nextAnimation
Prefabs référencent Banks par ID"bankId": "thesee" dans VisualAnimation_data
Séquence initiale: "currentAnimName": "idle"
Graph FSM optionnel: "animGraphPath": "..."
Séparation Bank ↔ GraphBank: Définit QUOI animer (frames, timing)
Graph: Définit QUAND animer (transitions, conditions)
Graph peut être absent (animations manuelles)
Architecture ECS StricteComponents = Pure Data (pas de méthodes, sauf constructeurs)
Helpers = Utility Functions (manipulation data)
Systems = Game Logic (update components)
Managers = Asset Management (load/cache)


🏗️ ARCHITECTURE DU SYSTÈME
Diagramme de Flux Complet
Code


┌──────────────────────────────────────────────────────────────────────────────┐
│                          ANIMATION EDITOR (F9)                               │
│  ┌────────────────────────────────────────────────────────────────────────┐  │
│  │ 1. SCAN ASSETS                                                         │  │
│  │    ├─ GameData/Animations/Banks/*.json                                 │  │
│  │    ├─ GameData/Animations/Graphs/*.json                                │  │
│  │    └─ GameData/Animations/**/*.{png,jpg} (spritesheets)                │  │
│  │                                                                          │  │
│  │ 2. CRUD ANIMATION BANK                                                 │  │
│  │    ├─ Create: New Bank + Add Spritesheets                             │  │
│  │    ├─ Read: Load Bank JSON + Load Textures                            │  │
│  │    ├─ Update: Edit Sequences + Preview Real-Time                      │  │
│  │    └─ Delete: Remove Sequence/Spritesheet                             │  │
│  │                                                                          │  │
│  │ 3. SPRITESHEET VIEWER                                                  │  │
│  │    ├─ Multi-spritesheet tabs (idle, walk, run...)                     │  │
│  │    ├─ Grid overlay (auto-detect columns/rows)                         │  │
│  │    ├─ Zoom/Pan controls (mouse wheel + drag)                          │  │
│  │    └─ Frame selection tool (drag rectangle)                           │  │
│  │                                                                          │  │
│  │ 4. ANIMATION PREVIEW                                                   │  │
│  │    ├─ Play/Pause/Stop controls                                         │  │
│  │    ├─ Frame scrubber (timeline)                                        │  │
│  │    ├─ Loop toggle                                                      │  │
│  │    └─ Speed multiplier (0.1x to 5x)                                   │  │
│  │                                                                          │  │
│  │ 5. SAVE JSON                                                           │  │
│  │    ├─ Format: Unified Schema v2                                        │  │
│  │    ├─ Location: GameData/Animations/Banks/{bankId}.json               │  │
│  │    └─ Validation: Check required fields                               │  │
│  └────────────────────────────────────────────────────────────────────────┘  │
└──────────────────────────────────────────────────────────────────────────────┘
                                    ↓ PRODUCES
                                    ↓
┌──────────────────────────────────────────────────────────────────────────────┐
│                     ANIMATION BANK JSON (Unified Schema v2)                  │
│  {                                                                            │
│    "schema_version": 2,                                                       │
│    "type": "AnimationBank",                                                   │
│    "bankId": "thesee",                                                        │
│    "description": "Thésée character animations",                             │
│    "metadata": {                                                              │
│      "author": "Atlasbruce",                                                  │
│      "created": "2026-02-16T10:00:00Z",                                      │
│      "lastModified": "2026-02-16T15:30:00Z",                                 │
│      "tags": ["character", "hero", "minotaur"]                               │
│    },                                                                         │
│    "spritesheets": [                                                          │
│      {                                                                        │
│        "id": "thesee_idle",                                                  │
│        "path": "./Gamedata/Minotaurus/texture/thesee_idle.png",             │
│        "frameWidth": 46, "frameHeight": 81,                                  │
│        "columns": 8, "rows": 16, "totalFrames": 128,                        │
│        "spacing": 0, "margin": 0,                                            │
│        "hotspot": { "x": 23, "y": 70 }                                       │
│      },                                                                       │
│      {                                                                        │
│        "id": "thesee_walk",                                                  │
│        "path": "./Gamedata/Minotaurus/texture/thesee_walk.png",             │
│        "frameWidth": 46, "frameHeight": 81,                                  │
│        "columns": 12, "rows": 11, "totalFrames": 128,                       │
│        "spacing": 0, "margin": 0,                                            │
│        "hotspot": { "x": 23, "y": 70 }                                       │
│      },                                                                       │
│      { ... }  // run, hit, etc.                                              │
│    ],                                                                         │
│    "sequences": [                                                             │
│      {                                                                        │
│        "name": "idle",                                                       │
│        "spritesheetId": "thesee_idle",                                      │
│        "frames": { "start": 0, "count": 128 },                              │
│        "frameDuration": 0.08,                                                │
│        "loop": true,                                                         │
│        "speed": 1.0,                                                         │
│        "nextAnimation": "",                                                  │
│        "events": []                                                          │
│      },                                                                       │
│      {                                                                        │
│        "name": "walk",                                                       │
│        "spritesheetId": "thesee_walk",                                      │
│        "frames": { "start": 0, "count": 128 },                              │
│        "frameDuration": 0.06,                                                │
│        "loop": true,                                                         │
│        "speed": 1.0                                                          │
│      },                                                                       │
│      { ... }  // run, hit, etc.                                              │
│    ]                                                                          │
│  }                                                                            │
└──────────────────────────────────────────────────────────────────────────────┘
                                    ↓ REFERENCED BY
                                    ↓
┌──────────────────────────────────────────────────────────────────────────────┐
│                          ENTITY PREFAB JSON                                  │
│  {                                                                            │
│    "schema_version": 2,                                                       │
│    "type": "Character",                                                       │
│    "name": "Thesee",                                                         │
│    "components": [                                                            │
│      {                                                                        │
│        "type": "Identity_data",                                              │
│        "properties": {                                                        │
│          "name": "Thésée",                                                   │
│          "tag": "Player",                                                    │
│          "entityType": "Player"                                              │
│        }                                                                      │
│      },                                                                       │
│      {                                                                        │
│        "type": "Position_data",                                              │
│        "properties": {                                                        │
│          "position": { "x": 100, "y": 100, "z": 0 }                         │
│        }                                                                      │
│      },                                                                       │
│      {                                                                        │
│        "type": "VisualAnimation_data",                                       │
│        "properties": {                                                        │
���          "bankId": "thesee",              ← References animation bank        │
│          "currentAnimName": "idle",       ← Initial sequence                 │
│          "animGraphPath": "./Gamedata/Animations/Graphs/thesee_animgraph.json", │
│          "playbackSpeed": 1.0,                                               │
│          "isPlaying": true,                                                  │
│          "loop": true                                                        │
│        }                                                                      │
│      }                                                                        │
│    ]                                                                          │
│  }                                                                            │
└──────────────────────────────────────────────────────────────────────────────┘
                                    ↓ LOADED AT RUNTIME
                                    ↓
┌──────────────────────────────────────────────────────────────────────────────┐
│                       RUNTIME ANIMATION SYSTEM                               │
│  ┌────────────────────────────────────────────────────────────────────────┐  │
│  │ AnimationManager::LoadAnimationBanks()                                 │  │
│  │ ├─ Parse JSON → Olympe::AnimationBank                                  │  │
│  │ ├─ Load textures → DataManager cache                                   │  │
│  │ └─ Store in m_banks map                                                │  │
│  └────────────────────────────────────────────────────────────────────────┘  │
│  ┌────────────────────────────────────────────────────────────────────────┐  │
│  │ AnimationSystem::Update(deltaTime)                                     │  │
│  │ ├─ For each VisualAnimation_data component                            │  │
│  │ ├─ Get bank via bankId                                                 │  │
│  │ ├─ Get sequence via currentAnimName                                    │  │
│  │ ├─ Update frame timer                                                  │  │
│  │ ├─ Calculate current frame index                                       │  │
│  │ └─ Update VisualSprite_data.srcRect                                    │  │
│  └──────────────────────────────────────────────────���─────────────────────┘  │
│  ┌────────────────────────────────────────────────────────────────────────┐  │
│  │ AI/BT → Animation Bridge (via AnimationHelpers)                       │  │
│  │ ├─ BehaviorTree executes action (e.g., MoveToGoal)                    │  │
│  │ ├─ Calls AnimationHelpers::SetParameter(animData, "isMoving", true)   │  │
│  │ ├─ AnimationSystem evaluates FSM graph transitions                    │  │
│  │ └─ Changes animation if conditions met (e.g., idle → walk)            │  │
│  └────────────────────────────────────────────────────────────────────────┘  │
└──────────────────────────────────────────────────────────────────────────────┘


Architecture ECS: Séparation Data/Logic
Code


┌────────────────────────────────────────────────────────────────┐
│                    COMPONENTS (Pure Data)                      │
├────────────────────────────────────────────────────────────────┤
│ VisualAnimation_data (struct, no methods)                      │
│ ├─ bankId, currentAnimName, animGraphPath                      │
│ ├─ playbackSpeed, isPlaying, loop                              │
│ ├─ currentFrameIndex, frameTimer                               │
│ └─ floatParams, boolParams, intParams (FSM parameters)         │
└────────────────────────────────────────────────────────────────┘
                            ↓ MANIPULATED BY
                            ↓
┌────────────────────────────────────────────────────────────────┐
│                 HELPERS (Utility Functions)                     │
├────────────────────────────────────────────────────────────────┤
│ namespace Olympe::AnimationHelpers {                           │
│   void SetParameter(VisualAnimation_data&, name, value);       │
│   float GetFloatParameter(const VisualAnimation_data&, ...);   │
│   bool GetBoolParameter(const VisualAnimation_data&, ...);     │
│   int GetIntParameter(const VisualAnimation_data&, ...);       │
│   bool HasParameter(const VisualAnimation_data&, name);        │
│   void ClearParameters(VisualAnimation_data&);                 │
│ }                                                               │
└────────────────────────────────────────────────────────────────┘
                            ↓ USED BY
                            ↓
┌────────────────────────────────────────────────────────────────┐
│                   SYSTEMS (Game Logic)                          │
├────────────────────────────────────────────────────────────────┤
│ AnimationSystem::Update(deltaTime)                             │
│ ├─ Update frame timers                                          │
│ ├─ Evaluate FSM transitions (via GetParameter)                 │
│ ├─ Change animations (via SetParameter)                        │
│ └─ Update VisualSprite_data.srcRect                            │
│                                                                 │
│ BehaviorTreeSystem::ExecuteAction(...)                         │
│ ├─ Set FSM parameters (via SetParameter)                       │
│ └─ Trigger animation changes indirectly                        │
│                                                                 │
│ PlayerControllerSystem::Update(deltaTime)                      │
│ ├─ Read input                                                   │
│ ├─ Set FSM parameters (speed, isMoving, etc.)                  │
│ └─ AnimationSystem reacts next frame                           │
└────────────────────────────────────────────────────────────────┘
                            ↓ LOADS DATA FROM
                            ↓
┌────────────────────────────────────────────────────────────────┐
│                 MANAGERS (Asset Management)                     │
├────────────────────────────────────────────────────────────────┤
│ AnimationManager::LoadAnimationBanks(directory)                │
│ ├─ Scan JSON files                                              │
│ ├─ Parse JSON → AnimationBank structures                       │
│ ├─ Load textures via DataManager                               │
│ └─ Cache banks in std::unordered_map<bankId, bank>             │
│                                                                 │
│ DataManager::GetOrLoadTexture(path)                            │
│ ├─ Check texture cache                                          │
│ ├─ Load with SDL_IMG if not cached                             │
│ ├─ Fallback: recursive scan GameData/ if file not found       │
│ └─ Return SDL_Texture*                                          │
└────────────────────────────────────────────────────────────────┘




🖼️ ANALYSE DES SPRITESHEETS FOURNIS
Contexte: Spritesheets Thésée
Les images fournies montrent 4 spritesheets pour le personnage "Thésée" (héros du jeu Minotaurus):

thesee_idle.png - Animation d'attente
thesee_walk.png - Animation de marche
thesee_run.png - Animation de course
thesee_hit.png - Animation de réaction aux dégâts
Caractéristiques Communes

Propriété	Valeur
Frame Size	46px × 81px
Hotspot	(23, 70) - centre-bas du personnage
Spacing	0px (pas d'espace entre frames)
Margin	0px (pas de marge autour du spritesheet)

Image 1: thesee_idle.png
Dimensions estimées: 368px (width) × 1296px (height)
Frame size: 46px × 81px
Layout: 8 colonnes × 16 lignes = 128 frames
Observations:

✅ Grid régulier, pas de spacing
✅ Animation Idle complète (128 frames pour ~10 sec à 12 FPS)
✅ Frames identiques répétées (idle breathing animation)
Métadonnées JSON:
JSON


{
  "id": "thesee_idle",
  "path": "./Gamedata/Minotaurus/texture/thesee_idle.png",
  "frameWidth": 46,
  "frameHeight": 81,
  "columns": 8,
  "rows": 16,
  "totalFrames": 128,
  "spacing": 0,
  "margin": 0,
  "hotspot": { "x": 23, "y": 70 },
  "description": "Idle breathing animation"
}




Image 2: thesee_walk.png
Dimensions estimées: 552px (width) × 891px (height)
Frame size: 46px × 81px
Layout: 12 colonnes × 11 lignes = 132 frames (mais 128 utilisées)
Observations:

✅ Grid régulier
✅ Animation Walk complète (cycle de marche)
✅ Légèrement plus rapide que idle (plus de mouvement)
⚠️ Plus de colonnes (12 vs 8) pour accommoder cycle de marche
Métadonnées JSON:
JSON


{
  "id": "thesee_walk",
  "path": "./Gamedata/Minotaurus/texture/thesee_walk.png",
  "frameWidth": 46,
  "frameHeight": 81,
  "columns": 12,
  "rows": 11,
  "totalFrames": 128,
  "spacing": 0,
  "margin": 0,
  "hotspot": { "x": 23, "y": 70 },
  "description": "Walk cycle animation"
}




Image 3: thesee_run.png
Dimensions estimées: 368px (width) × 1296px (height)
Frame size: 46px × 81px
Layout: 8 colonnes × 16 lignes = 128 frames
Observations:

✅ Grid identique à idle
✅ Animation Run (course rapide)
✅ Cycle plus court que walk (mouvement plus vif)
✅ Framerate plus élevé (0.05s vs 0.06s pour walk)
Métadonnées JSON:
JSON


{
  "id": "thesee_run",
  "path": "./Gamedata/Minotaurus/texture/thesee_run.png",
  "frameWidth": 46,
  "frameHeight": 81,
  "columns": 8,
  "rows": 16,
  "totalFrames": 128,
  "spacing": 0,
  "margin": 0,
  "hotspot": { "x": 23, "y": 70 },
  "description": "Run cycle animation"
}




Image 4: thesee_hit.png
Dimensions estimées: 322px (width) × 1296px (height)
Frame size: 46px × 81px
Layout: 7 colonnes × 16 lignes = 112 frames
Observations:

✅ Grid régulier (7 colonnes au lieu de 8)
✅ Animation Hit/Hurt (réaction aux dégâts)
✅ Animation non-looping (one-shot)
✅ Moins de frames (112 vs 128)
⚠️ Doit transitionner vers "idle" après complétion
Métadonnées JSON:
JSON


{
  "id": "thesee_hit",
  "path": "./Gamedata/Minotaurus/texture/thesee_hit.png",
  "frameWidth": 46,
  "frameHeight": 81,
  "columns": 7,
  "rows": 16,
  "totalFrames": 112,
  "spacing": 0,
  "margin": 0,
  "hotspot": { "x": 23, "y": 70 },
  "description": "Hit reaction animation"
}




Synthèse: Structure d'une Bank Multi-Spritesheets
Bank ID: thesee
Spritesheets: 4 (idle, walk, run, hit)
Total frames: 480 frames
Frame size: Uniforme (46×81px)
Hotspot: Uniforme (23, 70) - centre-bas du personnage
Tableau récapitulatif:

Spritesheet	Columns	Rows	Total Frames	Duration (0.08s/frame)	FPS
thesee_idle	8	16	128	10.24s	12.5
thesee_walk	12	11	128	7.68s (0.06s/f)	16.67
thesee_run	8	16	128	6.40s (0.05s/f)	20
thesee_hit	7	16	112	4.48s (0.04s/f)	25

Constat Important:

✅ Même frame size pour tous les spritesheets d'une bank
✅ Columns/Rows variables selon le spritesheet
✅ Hotspot identique (cohérence visuelle)
✅ Paths relatifs depuis GameData/


📊 STRUCTURES DE DONNÉES
Principe ECS: Data-Only Components

⚠️ RÈGLE FONDAMENTALE: Dans une architecture ECS pure, les components sont des structures de données pures (POD - Plain Old Data) sans logique métier. Les fonctions doivent être dans les Systems ou Managers ou Helpers.




1. SpritesheetInfo (Metadata par Spritesheet)
C++


// Dans Animation/AnimationTypes.h
namespace Olympe
{
    /**
     * @struct SpritesheetInfo
     * @brief Metadata for a single spritesheet within an animation bank
     * 
     * Defines the physical layout and properties of a spritesheet image.
     * One bank can contain multiple spritesheets (idle, walk, run, etc.).
     */
    struct SpritesheetInfo
    {
        std::string id;                 ///< Unique identifier within bank (e.g., "thesee_idle")
        std::string path;               ///< Relative path to spritesheet image
        std::string description;        ///< Optional description
        
        // Frame dimensions
        int frameWidth = 0;             ///< Width of each frame in pixels
        int frameHeight = 0;            ///< Height of each frame in pixels
        
        // Grid layout
        int columns = 0;                ///< Number of columns in spritesheet
        int rows = 0;                   ///< Number of rows in spritesheet
        int totalFrames = 0;            ///< Total number of frames available
        int spacing = 0;                ///< Spacing between frames in pixels
        int margin = 0;                 ///< Margin around spritesheet edges in pixels
        
        // Rendering properties
        SDL_FPoint hotspot = {0, 0};    ///< Default hotspot/pivot point (e.g., center-bottom)
        
        // ✅ NO METHODS (pure data)
        SpritesheetInfo() = default;
        SpritesheetInfo(const SpritesheetInfo&) = default;
        SpritesheetInfo& operator=(const SpritesheetInfo&) = default;
    };
    
} // namespace Olympe


Usage:
C++


SpritesheetInfo sheet;
sheet.id = "thesee_idle";
sheet.path = "./Gamedata/Minotaurus/texture/thesee_idle.png";
sheet.frameWidth = 46;
sheet.frameHeight = 81;
sheet.columns = 8;
sheet.rows = 16;
sheet.totalFrames = 128;
sheet.hotspot = {23.0f, 70.0f};




2. AnimationSequence (Logical Frame Range)
C++


// Dans Animation/AnimationTypes.h
namespace Olympe
{
    /**
     * @struct AnimationSequence
     * @brief Defines a single animation sequence (e.g., "idle", "walk")
     * 
     * References a spritesheet and defines which frames to use for this animation.
     * One bank can contain multiple sequences using different spritesheets.
     */
    struct AnimationSequence
    {
        std::string name;               ///< Animation name (e.g., "idle", "walk", "attack")
        std::string spritesheetId;      ///< ID of spritesheet to use (references SpritesheetInfo.id)
        
        // Frame range
        int startFrame = 0;             ///< Starting frame index (0-based)
        int frameCount = 1;             ///< Number of frames in sequence
        
        // Playback properties
        float frameDuration = 0.1f;     ///< Duration of each frame in seconds
        bool loop = true;               ///< Whether animation loops
        float speed = 1.0f;             ///< Speed multiplier (1.0 = normal)
        
        // Transition
        std::string nextAnimation;      ///< Animation to play after this one (if not looping)
        
        // Events (Phase 2)
        // std::vector<AnimationEvent> events;
        
        // ✅ NO METHODS (pure data)
        AnimationSequence() = default;
        AnimationSequence(const AnimationSequence&) = default;
        AnimationSequence& operator=(const AnimationSequence&) = default;
        
        /**
         * @brief Calculate total duration of animation
         * @return Duration in seconds
         */
        float GetTotalDuration() const {
            return frameCount * frameDuration / speed;
        }
        
        /**
         * @brief Calculate effective FPS
         * @return Frames per second
         */
        float GetEffectiveFPS() const {
            return (frameDuration > 0.0f) ? (1.0f / frameDuration) * speed : 0.0f;
        }
    };
    
} // namespace Olympe


Usage:
C++


AnimationSequence seq;
seq.name = "idle";
seq.spritesheetId = "thesee_idle";
seq.startFrame = 0;
seq.frameCount = 128;
seq.frameDuration = 0.08f;  // 12.5 FPS
seq.loop = true;
seq.speed = 1.0f;




3. AnimationBank (Collection of Spritesheets + Sequences)
C++


// Dans Animation/AnimationTypes.h
namespace Olympe
{
    /**
     * @struct AnimationBank
     * @brief Collection of spritesheets and animation sequences for an entity
     * 
     * A bank is the top-level container for all animations of a character/entity.
     * It can contain multiple spritesheets (one per animation type) and multiple
     * sequences (logical animations using those spritesheets).
     */
    struct AnimationBank
    {
        std::string bankId;             ///< Unique identifier (e.g., "thesee", "player", "zombie")
        std::string description;        ///< Optional description
        
        // Metadata (optional)
        std::string author;
        std::string createdDate;
        std::string lastModifiedDate;
        std::vector<std::string> tags;
        
        // ✅ Multi-spritesheet support
        std::vector<SpritesheetInfo> spritesheets;
        
        // ✅ Animation sequences
        std::vector<AnimationSequence> sequences;
        
        // ✅ NO METHODS (pure data, except simple lookups)
        AnimationBank() = default;
        AnimationBank(const AnimationBank&) = default;
        AnimationBank& operator=(const AnimationBank&) = default;
        
        /**
         * @brief Get spritesheet by ID
         * @param id Spritesheet identifier
         * @return Pointer to spritesheet or nullptr if not found
         */
        const SpritesheetInfo* GetSpritesheet(const std::string& id) const {
            for (const auto& sheet : spritesheets) {
                if (sheet.id == id) return &sheet;
            }
            return nullptr;
        }
        
        /**
         * @brief Get non-const spritesheet by ID
         */
        SpritesheetInfo* GetSpritesheet(const std::string& id) {
            for (auto& sheet : spritesheets) {
                if (sheet.id == id) return &sheet;
            }
            return nullptr;
        }
        
        /**
         * @brief Get animation sequence by name
         * @param name Sequence name (e.g., "idle", "walk")
         * @return Pointer to sequence or nullptr if not found
         */
        const AnimationSequence* GetSequence(const std::string& name) const {
            for (const auto& seq : sequences) {
                if (seq.name == name) return &seq;
            }
            return nullptr;
        }
        
        /**
         * @brief Get non-const sequence by name
         */
        AnimationSequence* GetSequence(const std::string& name) {
            for (auto& seq : sequences) {
                if (seq.name == name) return &seq;
            }
            return nullptr;
        }
    };
    
} // namespace Olympe


Usage:
C++


AnimationBank bank;
bank.bankId = "thesee";
bank.description = "Thésée character animations";
bank.author = "Atlasbruce";

// Add spritesheets
SpritesheetInfo idleSheet;
idleSheet.id = "thesee_idle";
// ... configure ...
bank.spritesheets.push_back(idleSheet);

// Add sequences
AnimationSequence idleSeq;
idleSeq.name = "idle";
idleSeq.spritesheetId = "thesee_idle";
// ... configure ...
bank.sequences.push_back(idleSeq);

// Lookup
const AnimationSequence* seq = bank.GetSequence("idle");
const SpritesheetInfo* sheet = bank.GetSpritesheet("thesee_idle");




4. VisualAnimation_data (ECS Component - Pure Data)
C++


// Dans ECS_Components.h
namespace Olympe
{
    /**
     * @struct VisualAnimation_data
     * @brief Component for sprite-based 2D animation (pure data, no methods)
     * 
     * Stores animation playback state and references to animation bank/graph.
     * All logic is handled by AnimationSystem and AnimationHelpers.
     * 
     * ⚠️ ECS RULE: NO METHODS (except constructors/assignment)
     */
    struct VisualAnimation_data
    {
        // ═══════════════════════════════════════════════════════════════
        // CORE DATA
        // ═══════════════════════════════════════════════════════════════
        std::string bankId;             ///< Animation bank ID ("thesee", "player"...)
        std::string currentAnimName;    ///< Current animation name ("idle", "walk"...)
        std::string animGraphPath;      ///< Path to FSM graph JSON (optional)
        
        // ═══════════════════════════════════════════════════════════════
        // PLAYBACK STATE
        // ═══════════════════════════════════════════════════════════════
        float playbackSpeed = 1.0f;     ///< Speed multiplier (0.5x to 5x)
        bool isPlaying = true;          ///< Is animation playing
        bool loop = true;               ///< Loop enabled (overrides sequence setting)
        
        int currentFrameIndex = 0;      ///< Current frame index in sequence
        float frameTimer = 0.0f;        ///< Timer for frame advancement (seconds)
        
        // ═══════════════════════════════════════════════════════════════
        // ANIMATION GRAPH PARAMETERS (Phase 2)
        // ═══════════════════════════════════════════════════════════════
        /// Runtime parameters for FSM transitions (set by AI/BT systems)
        /// These are manipulated via AnimationHelpers, NOT directly
        std::unordered_map<std::string, float> floatParams;
        std::unordered_map<std::string, bool> boolParams;
        std::unordered_map<std::string, int> intParams;
        
        // ═══════════════════════════════════════════════════════════════
        // CONSTRUCTORS (ECS requirement - ONLY allowed methods)
        // ═══════════════════════════════════════════════════════════════
        VisualAnimation_data() = default;
        VisualAnimation_data(const VisualAnimation_data&) = default;
        VisualAnimation_data& operator=(const VisualAnimation_data&) = default;
    };
    
} // namespace Olympe


⚠️ IMPORTANT: Ce component N'A PAS de méthodes comme SetParameter() ou GetParameter(). Ces fonctions sont dans AnimationHelpers.


5. AnimationHelpers (Utility Functions - Outside Component)
C++


// Dans Animation/AnimationHelpers.h
/**
 * @file AnimationHelpers.h
 * @brief Helper functions for manipulating VisualAnimation_data components
 * @author Atlasbruce
 * @date 2026-02-16
 * 
 * Provides utility functions to set/get animation graph parameters
 * without violating ECS principles (components remain pure data).
 */

#pragma once

#include "../ECS_Components.h"
#include <string>

namespace Olympe
{
    namespace AnimationHelpers
    {
        // ═══════════════════════════════════════════════════════════════
        // PARAMETER SETTERS
        // ═══════════════════════════════════════════════════════════════
        
        /**
         * @brief Set float parameter for animation graph
         * @param animData Component to modify
         * @param name Parameter name (e.g., "speed", "health")
         * @param value Parameter value
         * 
         * Example:
         * @code
         * auto& animData = world.GetComponent<VisualAnimation_data>(entity);
         * AnimationHelpers::SetParameter(animData, "speed", 120.0f);
         * @endcode
         */
        inline void SetParameter(VisualAnimation_data& animData, 
                                  const std::string& name, 
                                  float value)
        {
            animData.floatParams[name] = value;
        }
        
        /**
         * @brief Set bool parameter for animation graph
         * @param animData Component to modify
         * @param name Parameter name (e.g., "isMoving", "isDead")
         * @param value Parameter value
         * 
         * Example:
         * @code
         * AnimationHelpers::SetParameter(animData, "isMoving", true);
         * @endcode
         */
        inline void SetParameter(VisualAnimation_data& animData, 
                                  const std::string& name, 
                                  bool value)
        {
            animData.boolParams[name] = value;
        }
        
        /**
         * @brief Set int parameter for animation graph
         * @param animData Component to modify
         * @param name Parameter name (e.g., "comboCount")
         * @param value Parameter value
         * 
         * Example:
         * @code
         * AnimationHelpers::SetParameter(animData, "comboCount", 3);
         * @endcode
         */
        inline void SetParameter(VisualAnimation_data& animData, 
                                  const std::string& name, 
                                  int value)
        {
            animData.intParams[name] = value;
        }
        
        // ═══════════════════════════════════════════════════════════════
        // PARAMETER GETTERS
        // ═══════════════════════════════════════════════════════════════
        
        /**
         * @brief Get float parameter from animation graph
         * @param animData Component to read from
         * @param name Parameter name
         * @param defaultValue Value to return if parameter not found
         * @return Parameter value or default
         * 
         * Example:
         * @code
         * float speed = AnimationHelpers::GetFloatParameter(animData, "speed", 0.0f);
         * @endcode
         */
        inline float GetFloatParameter(const VisualAnimation_data& animData, 
                                        const std::string& name, 
                                        float defaultValue = 0.0f)
        {
            auto it = animData.floatParams.find(name);
            return (it != animData.floatParams.end()) ? it->second : defaultValue;
        }
        
        /**
         * @brief Get bool parameter from animation graph
         * @param animData Component to read from
         * @param name Parameter name
         * @param defaultValue Value to return if parameter not found
         * @return Parameter value or default
         * 
         * Example:
         * @code
         * bool isMoving = AnimationHelpers::GetBoolParameter(animData, "isMoving", false);
         * @endcode
         */
        inline bool GetBoolParameter(const VisualAnimation_data& animData, 
                                      const std::string& name, 
                                      bool defaultValue = false)
        {
            auto it = animData.boolParams.find(name);
            return (it != animData.boolParams.end()) ? it->second : defaultValue;
        }
        
        /**
         * @brief Get int parameter from animation graph
         * @param animData Component to read from
         * @param name Parameter name
         * @param defaultValue Value to return if parameter not found
         * @return Parameter value or default
         * 
         * Example:
         * @code
         * int combo = AnimationHelpers::GetIntParameter(animData, "comboCount", 0);
         * @endcode
         */
        inline int GetIntParameter(const VisualAnimation_data& animData, 
                                    const std::string& name, 
                                    int defaultValue = 0)
        {
            auto it = animData.intParams.find(name);
            return (it != animData.intParams.end()) ? it->second : defaultValue;
        }
        
        // ═══════════════════════════════════════════════════════════════
        // CONVENIENCE FUNCTIONS
        // ═══════════════════════════════════════════════════════════════
        
        /**
         * @brief Check if parameter exists
         * @param animData Component to check
         * @param name Parameter name
         * @return true if parameter exists (any type)
         */
        inline bool HasParameter(const VisualAnimation_data& animData, 
                                  const std::string& name)
        {
            return (animData.floatParams.find(name) != animData.floatParams.end()) ||
                   (animData.boolParams.find(name) != animData.boolParams.end()) ||
                   (animData.intParams.find(name) != animData.intParams.end());
        }
        
        /**
         * @brief Clear all parameters
         * @param animData Component to clear
         */
        inline void ClearParameters(VisualAnimation_data& animData)
        {
            animData.floatParams.clear();
            animData.boolParams.clear();
            animData.intParams.clear();
        }
        
        /**
         * @brief Copy parameters from one component to another
         * @param source Source component
         * @param dest Destination component
         */
        inline void CopyParameters(const VisualAnimation_data& source, 
                                    VisualAnimation_data& dest)
        {
            dest.floatParams = source.floatParams;
            dest.boolParams = source.boolParams;
            dest.intParams = source.intParams;
        }
        
    } // namespace AnimationHelpers
    
} // namespace Olympe


Usage Example (BehaviorTree → Animation):
C++


// Dans AI/BehaviorTree.cpp
#include "../Animation/AnimationHelpers.h"

BTStatus ExecuteBTAction(BTActionType actionType, ...)
{
    auto& world = World::Get();
    
    // ✅ Synchronize animation with BT action
    if (world.HasComponent<VisualAnimation_data>(entity))
    {
        auto& animData = world.GetComponent<VisualAnimation_data>(entity);
        
        switch (actionType)
        {
            case BTActionType::MoveToGoal:
                AnimationHelpers::SetParameter(animData, "isMoving", true);
                AnimationHelpers::SetParameter(animData, "speed", blackboard.currentSpeed);
                break;
                
            case BTActionType::AttackIfClose:
                AnimationHelpers::SetParameter(animData, "isAttacking", true);
                break;
                
            case BTActionType::Idle:
                AnimationHelpers::SetParameter(animData, "isMoving", false);
                break;
        }
    }
    
    // ... rest of action code ...
}




📄 FORMAT JSON UNIFIÉ
Schema Version 2 (Unified Format)
Emplacement: GameData/Animations/Banks/{bankId}.json
Structure Complète:
JSON


{
  "schema_version": 2,
  "type": "AnimationBank",
  "bankId": "thesee",
  "description": "Thésée character animations - Complete combat moveset",
  "metadata": {
    "author": "Atlasbruce",
    "created": "2026-02-16T10:00:00Z",
    "lastModified": "2026-02-16T15:30:00Z",
    "tags": ["character", "hero", "minotaur"],
    "version": "1.0.0"
  },
  "spritesheets": [
    {
      "id": "thesee_idle",
      "path": "./Gamedata/Minotaurus/texture/thesee_idle.png",
      "description": "Idle breathing animation",
      "frameWidth": 46,
      "frameHeight": 81,
      "columns": 8,
      "rows": 16,
      "totalFrames": 128,
      "spacing": 0,
      "margin": 0,
      "hotspot": {
        "x": 23,
        "y": 70
      }
    },
    {
      "id": "thesee_walk",
      "path": "./Gamedata/Minotaurus/texture/thesee_walk.png",
      "description": "Walk cycle animation",
      "frameWidth": 46,
      "frameHeight": 81,
      "columns": 12,
      "rows": 11,
      "totalFrames": 128,
      "spacing": 0,
      "margin": 0,
      "hotspot": {
        "x": 23,
        "y": 70
      }
    },
    {
      "id": "thesee_run",
      "path": "./Gamedata/Minotaurus/texture/thesee_run.png",
      "description": "Run cycle animation",
      "frameWidth": 46,
      "frameHeight": 81,
      "columns": 8,
      "rows": 16,
      "totalFrames": 128,
      "spacing": 0,
      "margin": 0,
      "hotspot": {
        "x": 23,
        "y": 70
      }
    },
    {
      "id": "thesee_hit",
      "path": "./Gamedata/Minotaurus/texture/thesee_hit.png",
      "description": "Hit reaction animation",
      "frameWidth": 46,
      "frameHeight": 81,
      "columns": 7,
      "rows": 16,
      "totalFrames": 112,
      "spacing": 0,
      "margin": 0,
      "hotspot": {
        "x": 23,
        "y": 70
      }
    }
  ],
  "sequences": [
    {
      "name": "idle",
      "spritesheetId": "thesee_idle",
      "frames": {
        "start": 0,
        "count": 128
      },
      "frameDuration": 0.08,
      "loop": true,
      "speed": 1.0,
      "nextAnimation": "",
      "events": []
    },
    {
      "name": "walk",
      "spritesheetId": "thesee_walk",
      "frames": {
        "start": 0,
        "count": 128
      },
      "frameDuration": 0.06,
      "loop": true,
      "speed": 1.0,
      "nextAnimation": "",
      "events": []
    },
    {
      "name": "run",
      "spritesheetId": "thesee_run",
      "frames": {
        "start": 0,
        "count": 128
      },
      "frameDuration": 0.05,
      "loop": true,
      "speed": 1.2,
      "nextAnimation": "",
      "events": []
    },
    {
      "name": "hit",
      "spritesheetId": "thesee_hit",
      "frames": {
        "start": 0,
        "count": 112
      },
      "frameDuration": 0.04,
      "loop": false,
      "speed": 1.5,
      "nextAnimation": "idle",
      "events": [
        {
          "frame": 0,
          "type": "sound",
          "data": {
            "soundPath": "./Resources/Sounds/hit_impact.wav",
            "volume": 0.8
          }
        }
      ]
    }
  ]
}


Champs Requis (Validation)

Champ	Type	Requis	Description
schema_version	int	✅	Doit être 2
type	string	✅	Doit être "AnimationBank"
bankId	string	✅	Unique identifier
spritesheets	array	✅	Au moins 1 spritesheet
sequences	array	✅	Au moins 1 séquence

Champs Optionnels

Champ	Type	Description
description	string	Description de la bank
metadata	object	Métadonnées (author, created, tags...)
events	array	Événements par frame (Phase 2)



Rétrocompatibilité Format V1
Format ancien (thesee_animations.json actuel):
JSON


{
  "bankId": "thesee",
  "frameWidth": 46,
  "frameHeight": 81,
  "columns": 16,
  "spacing": 0,
  "margin": 0,
  "animations": {
    "idle": {
      "spritesheet": "./Gamedata/Minotaurus/texture/thesee_idle.png",
      "frames": { "start": 0, "count": 128 },
      "frameDuration": 0.08,
      "loop": true,
      "speed": 1.0
    },
    "walk": {
      "spritesheet": "./Gamedata/Minotaurus/texture/thesee_walk.png",
      "frames": { "start": 0, "count": 128 },
      "frameDuration": 0.06,
      "loop": true,
      "speed": 1.0
    }
  }
}


Conversion V1 → V2 (dans AnimationManager::LoadAnimationBank()):
C++


// Detect schema version
int schemaVersion = json_get_int(bankJson, "schema_version", 1);

if (schemaVersion == 1)
{
    // ✅ Convert V1 to V2 format
    AnimationBank bank;
    bank.bankId = json_get_string(bankJson, "bankId", "");
    
    // Extract single spritesheet metadata (common to all animations in V1)
    int frameWidth = json_get_int(bankJson, "frameWidth", 32);
    int frameHeight = json_get_int(bankJson, "frameHeight", 32);
    int columns = json_get_int(bankJson, "columns", 1);
    int spacing = json_get_int(bankJson, "spacing", 0);
    int margin = json_get_int(bankJson, "margin", 0);
    
    // For each animation, create spritesheet + sequence
    if (bankJson.contains("animations") && bankJson["animations"].is_object())
    {
        for (auto it = bankJson["animations"].begin(); it != bankJson["animations"].end(); ++it)
        {
            std::string animName = it.key();
            const auto& animJson = it.value();
            
            // Create spritesheet info
            SpritesheetInfo sheet;
            sheet.id = bank.bankId + "_" + animName;
            sheet.path = json_get_string(animJson, "spritesheet", "");
            sheet.frameWidth = frameWidth;
            sheet.frameHeight = frameHeight;
            sheet.columns = columns;
            sheet.spacing = spacing;
            sheet.margin = margin;
            
            // Calculate rows from total frames
            int startFrame = 0;
            int count = 1;
            if (animJson.contains("frames") && animJson["frames"].is_object())
            {
                startFrame = json_get_int(animJson["frames"], "start", 0);
                count = json_get_int(animJson["frames"], "count", 1);
            }
            sheet.totalFrames = count;
            sheet.rows = (count + columns - 1) / columns;  // Ceiling division
            
            bank.spritesheets.push_back(sheet);
            
            // Create sequence
            AnimationSequence seq;
            seq.name = animName;
            seq.spritesheetId = sheet.id;
            seq.startFrame = startFrame;
            seq.frameCount = count;
            seq.frameDuration = json_get_float(animJson, "frameDuration", 0.1f);
            seq.loop = json_get_bool(animJson, "loop", true);
            seq.speed = json_get_float(animJson, "speed", 1.0f);
            
            bank.sequences.push_back(seq);
        }
    }
    
    return bank;
}




🎨 INTERFACE UTILISATEUR
Layout Global (3-Panels)
Code


┌──────────────────────────────────────────────────────────────────────┐
│ Animation Editor - F9                                          [_][□][×]│
├──────────────────────────────────────────────────────────────────────┤
│ File | Edit | View | Tools | Help                                     │
├─────────┬────────────────────────────────────────────────┬─────────────┤
│         │                                                │             │
│ 📦 BANKS│         🖼️ SPRITESHEET VIEWER               │  ⚙️ PROPS   │
│ ════════│  ┌───────────────────────────────────────────┐│  ═══════════│
│         │  │ 📑 Tabs: [Idle][Walk][Run][Hit][+New]    ││             │
│ 🔍Search│  ├───────────────────────────────────────────┤│ 🏦 Bank Info│
│ [     ]│  │                                           ││ ┌───────────┐│
│         │  │   [Grid with highlighted current frame]  ││ │ID: thesee ││
│ ☑ Banks │  │                                           ││ │Desc: ... ││
│ ☐ Sheets│  │   Zoom: [░░░░█████░░░] 150%             ││ └───────────┘│
│         │  │   Pan: Middle-drag to move               ││             │
│ thesee  │  │   Select: Left-drag rectangle            ││ 📄 Spritesheet│
│ ├─ idle │  │                                           ││ ┌───────────┐│
│ ├─ walk │  │   [Character sprite centered]            ││ │ID: thesee_││
│ ├─ run  │  │                                           ││ │   walk    ││
│ └─ hit  │  │                                           ││ │Path: ...  ││
│         │  │                                           ││ │W: 46  H:81││
│ player  │  └───────────────────────────────────────────┘│ │Cols: 12   ││
│ zombie  │  ┌───────────────────────────────────────────┐│ │Rows: 11   ││
│         │  │ 🎬 PREVIEW: "walk"                       ││ └───────────┘│
│ 🖼️ SHEETS│  │ [▶][⏸][⏹][🔁] Frame: 45/128            ││             │
│ ════════│  │ ░░░░░░█░░░░░░░░░░░░░░░░░░░░░░░░░░ (scrub)││ 🎞️ Sequence │
│         │  │ Speed: [1.0x▼] FPS: 16.67                ││ ┌───────────┐│
│ 📂 idle │  │                                           ││ │Name: walk ││
│ 📂 walk │  │ [Animated preview of current sequence]   ││ │Sheet: ... ││
│ 📂 run  │  │                                           ││ │Start: 0   ││
│ 📂 hit  │  └───────────────────────────────────────────┘│ │Count: 128 ││
│         │                                                │ │Dur: 0.06s ││
│ + New   │                                                │ │Loop: ✓    ││
│         │                                                │ │Speed: 1.0 ││
│         │                                                │ └───────────┘│
│         │                                                │ [Apply]     │
│         │                                                │             │
└─────────┴────────────────────────────────────────────────┴─────────────┘
│ Status: Bank loaded (4 spritesheets, 4 sequences) | Modified: Yes    │
└──────────────────────────────────────────────────────────────────────┘




Panel 1: Banks & Spritesheets (Gauche, 250-300px)
Section Banks
Liste hiérarchique:
Code


📦 thesee [✓]              ← Bank (checkbox = loaded)
├─ 📄 idle (128f)
├─ 📄 walk (128f)
├─ 📄 run (128f)
└─ 📄 hit (112f)

📦 player [✓]
├─ 📄 idle (8f)
└─ 📄 walk (6f)

📦 zombie [ ]              ← Unloaded bank


Interactions:

Clic bank → Charger bank (afficher spritesheets)
Clic séquence → Charger dans preview
Double-clic → Ouvrir spritesheet viewer
Clic droit → Menu contextuel:New Bank...
Edit Bank...
Duplicate Bank...
Delete Bank (avec confirmation)
Export Bank...
Reload Bank (discard changes)
Search bar:

Filtrer par nom (temps réel)
Support wildcards (thes*, *idle)
Toggle filters:

☑ Show Banks (afficher banks)
☐ Show Individual Spritesheets (afficher sheets isolés)
☑ Show Loaded Only (masquer non chargés)
Status badges:

📦 Bank loaded
📦 [✏️] Bank modified
📦 [❌] Bank has errors


Section Spritesheets (optionnel, si toggle activé)
Liste plate de tous les spritesheets scannés:
Code


🖼️ All Spritesheets (12)
├─ 📂 thesee_idle.png (368×1296, 128f)
├─ 📂 thesee_walk.png (552×891, 128f)
├─ 📂 player_idle.png (256×32, 8f)
└─ ...


Drag & Drop vers bank pour ajouter un spritesheet existant


Panel 2: Spritesheet Viewer + Preview (Centre, flex)
Tabs Spritesheets
Code


┌─[Idle]─[Walk]─[Run]─[Hit]─[+ New Spritesheet]─────────────┐
│                                                             │
│  [Spritesheet texture avec grid overlay]                   │
│                                                             │
│  Controls:                                                  │
│  - Zoom: Molette souris ou slider [░░░█████░░░] 150%       │
│  - Pan: Middle-drag ou Space+Drag                          │
│  - Select Frame: Left-drag rectangle                       │
│  - Grid: Toggle with [G] key                               │
│                                                             │
│  Selection Info: Frame 12 (Col 4, Row 1)                   │
│  Rect: (184, 81, 46, 81)                                   │
│                                                             │
└─────────────────────────────────────────────────────────────┘


Fonctionnalités:

Grid Overlay:Afficher colonnes/lignes selon métadonnées spritesheet
Highlight frame sous souris (border jaune)
Numérotation frames (0, 1, 2... en overlay)
Grid color: blanc semi-transparent (customizable)
Frame Selection:Drag rectangle → Sélectionner range
Shift+Click → Multi-sélection
Ctrl+Click → Ajouter à sélection
Selected frames: border verte épaisse
Zoom/Pan:Zoom: Molette (0.1x à 10x, default 1.0x)
Pan: MMB drag ou Space+LMB drag
Fit to view: F key (center + zoom to fit all)
Reset: R key (zoom 100%, center)
Context Menu (right-click):Set as Sequence Start Frame
Add Selected Frames to Sequence
Copy Frame Coordinates
--- (separator)
Edit Spritesheet Properties...
Replace Spritesheet...
Remove Spritesheet from Bank
Keyboard Shortcuts:G: Toggle grid
F: Fit to view
R: Reset zoom
Arrow keys: Navigate frames
Ctrl+A: Select all frames
Escape: Clear selection


Preview Zone
Code


┌─ 🎬 ANIMATION PREVIEW: "walk" ─────────────────────────────┐
│                                                             │
│  [▶ Play] [⏸ Pause] [⏹ Stop] [🔁 Loop]                    │
│                                                             │
│  ░░░░░░░░█░░░░░░░░░░░░░░░░░░░░░░░░░░░ (Timeline scrubber) │
│  Frame: 45 / 128    Time: 2.7s / 7.68s                    │
│                                                             │
│  [Animated sprite preview centered]                        │
│                                                             │
│  Speed: [1.0x ▼]   FPS: 16.67   Loop: ✓                   │
│                                                             │
│  Background: [Checkered ▼] Hotspot: ☑ Visible             │
│                                                             │
└─────────────────────────────────────────────────────────────┘


Fonctionnalités:

Playback Controls:Play (Space): Lancer animation
Pause: Mettre en pause (conserve frame)
Stop: Revenir frame 0
Loop: Toggle boucle (override sequence setting)
Step Frame: Arrow keys (précédent/suivant)
Timeline:Scrubber: Drag pour naviguer
Click: Jump to frame
Markers: Événements (Phase 2)
Frame number display: Current / Total
Speed Control:Dropdown presets: 0.1x, 0.25x, 0.5x, 1x, 2x, 5x
Custom input: Slider (0.1 à 10.0)
FPS display: Calcul automatique (1/frameDuration * speed)
Display Options:Background:Checkered (transparence)
Black
White
Custom color
Hotspot:☑ Visible (croix rouge)
Position affichée (x, y)
Bounding Box: Show collision rect (Phase 2)
Info Display:Current frame index
Current time / Total duration
Effective FPS
Sequence name
Loop status


Panel 3: Properties Inspector (Droite, 300-350px)
Section 1: Bank Info
Code


🏦 Bank Info
┌─────────────────────────────┐
│ Bank ID:    [thesee        ]│
│ Description:                │
│ [Character animations...   ]│
│                             │
│ Author:     [Atlasbruce    ]│
│ Version:    [1.0.0         ]│
│ Created:    2026-02-16      │
│ Modified:   2026-02-16 15:30│
│                             │
│ Tags:       [character, hero]│
└─────────────────────────────┘
[Save Bank] [Reload] [Export]


Champs éditables:

Bank ID (unique, validation)
Description (multiline)
Author
Version
Tags (comma-separated)
Boutons:

Save Bank: Sauvegarder JSON
Reload: Recharger depuis disque (perd modifications)
Export: Export vers autre format (Phase 2)


Section 2: Spritesheet Properties (contextuel selon tab actif)
Code


📄 Spritesheet: thesee_walk
┌─────────────────────────────┐
│ ID:         [thesee_walk   ]│
│ Description:[Walk cycle... ]│
│                             │
│ Path:       [./Gamedata/.. ]│
│             [Browse...]     │
│                             │
│ Frame Size:                 │
│   Width:    [46    ] px     │
│   Height:   [81    ] px     │
│                             │
│ Grid Layout:                │
│   Columns:  [12    ]        │
│   Rows:     [11    ]        │
│   Spacing:  [0     ] px     │
│   Margin:   [0     ] px     │
│                             │
│ Total Frames: 128 (calc.)   │
│                             │
│ Hotspot:                    │
│   X:        [23    ] px     │
│   Y:        [70    ] px     │
│             [Center][Bottom]│
└─────────────────────────────┘
[Auto-Detect Grid] [Apply]


Fonctionnalités:

Browse: Sélectionner nouvelle texture (file dialog)
Auto-Detect Grid: Analyser image pour détecter colonnes/lignesScan vertical pour détecter columns
Scan horizontal pour détecter rows
Détection spacing/margin automatique
Presets Hotspot:Center: (frameWidth/2, frameHeight/2)
Bottom: (frameWidth/2, frameHeight)
Top-Left: (0, 0)
Custom: Input manuel
Apply: Appliquer modifications (marque bank modifié)
Validation:

Width/Height > 0
Columns/Rows > 0
Total frames = columns × rows (warning si incohérent)


Section 3: Sequence Properties (contextuel selon séquence sélectionnée)
Code


🎞️ Sequence: walk
┌─────────────────────────────┐
│ Name:       [walk          ]│
│ Description:[Walk cycle... ]│
│                             │
│ Spritesheet: [thesee_walk▼]│
│                             │
│ Frame Range:                │
│   Start:    [0     ]        │
│   Count:    [128   ]        │
│             [←][→] Navigate │
│   Preview:  Frame 0-127     │
│                             │
│ Timing:                     │
│   Duration: [0.06  ] sec/fr │
│   FPS:      16.67 (calc.)   │
│   Total:    7.68s (calc.)   │
│                             │
│ Playback:                   │
│   Loop:     ☑ Enabled       │
│   Speed:    [1.0   ]x       │
│                             │
│ Transition:                 │
│   Next Anim:[none      ▼]   │
│                             │
│ Events:     [0 events]      │
│             [Add Event...] │ (Phase 2)
└─────────────────────────────┘
[Apply] [Duplicate] [Delete]


Fonctionnalités:

Frame Range:Start/Count: Input manuel
Navigate buttons: ← → pour ajuster visuellement
Preview: Affiche "Frame X-Y" calculé
Validation: Start + Count ≤ totalFrames
Timing:Duration: Durée par frame (secondes)
FPS: Calcul auto (1/duration)
Total: Durée totale (count × duration / speed)
Playback:Loop: Checkbox (override sequence setting)
Speed: Multiplicateur (0.1x à 10x)
Transition:Next Anim: Dropdown liste animations de la bank
Si loop=false, joue next anim après complétion
"none" = arrêt après complétion
Events (Phase 2):Liste événements frame-specific
Bouton "Add Event" ouvre éditeur
Types: sound, hitbox, VFX, gamelogic
Boutons:

Apply: Appliquer modifications
Duplicate: Dupliquer séquence (nouveau nom)
Delete: Supprimer séquence (avec confirmation)


Section 4: Sequence List
Code


📋 All Sequences (4)
┌─────────────────────────────┐
│ ☑ idle    (128f, 10.24s)   │
│ ☑ walk    (128f, 7.68s)    │
│ ☑ run     (128f, 6.40s)    │
│ ☐ hit     (112f, 4.48s)    │← unchecked = hidden
│                             │
│ [+ New Sequence]            │
└─────────────────────────────┘
Sort: [Name ▼]
Filter: [    ]


Interactions:

Clic → Sélectionner séquence (charge dans inspector)
Checkbox → Show/hide dans preview
Drag & drop → Réordonner
Double-clic → Rename
Sort options:

Name (A-Z)
Duration (shortest first)
Frame count (lowest first)
Filter: Recherche temps réel par nom


🔧 IMPLÉMENTATION TECHNIQUE
Fichiers à Créer
Code


Source/
├── Animation/
│   ├── AnimationEditorWindow.h         ← Main editor class
│   ├── AnimationEditorWindow.cpp
│   ├── SpritesheetViewer.h             ← Spritesheet display + grid
│   ├── SpritesheetViewer.cpp
│   ├── AnimationPreview.h              ← Playback controller
│   ├── AnimationPreview.cpp
│   ├── AnimationHelpers.h              ← ✅ NOUVEAU: Helper functions
│   ├── AnimationTypes.h                ← Structures (AnimationBank, etc.)
│   ├── AnimationManager.h              ← Manager (load/cache banks)
│   └── AnimationManager.cpp
├── Core/
│   ├── DataManager.h                   ← EXTEND: Texture cache
│   ├── DataManager.cpp                 ← EXTEND: Scan methods
│   └── ECS_Components.h                ← VisualAnimation_data (pure data)
└── GameEngine.cpp                      ← HOOK: F9 keybind




Classe Principale: AnimationEditorWindow
Header (Animation/AnimationEditorWindow.h):
C++


/**
 * @file AnimationEditorWindow.h
 * @brief Main animation editor window (F9)
 * @author Atlasbruce
 * @date 2026-02-16
 * 
 * Provides tools to create/edit animation banks with multiple spritesheets,
 * preview animations, and save to JSON format.
 * 
 * Architecture:
 * - Separate SDL3 window (independent of main game window)
 * - Separate ImGui context (isolated UI state)
 * - 3-panel layout: Banks list | Viewer+Preview | Properties
 */

#pragma once

#include "../ECS_Entity.h"
#include "../vector.h"
#include "AnimationTypes.h"
#include "SpritesheetViewer.h"
#include "AnimationPreview.h"
#include <string>
#include <vector>
#include <SDL3/SDL.h>

// Forward declarations
struct ImGuiContext;

namespace Olympe
{
    /**
     * @class AnimationEditorWindow
     * @brief Main animation editor window (F9)
     */
    class AnimationEditorWindow
    {
    public:
        AnimationEditorWindow();
        ~AnimationEditorWindow();
        
        /**
         * @brief Initialize editor (create SDL window, ImGui context)
         */
        void Initialize();
        
        /**
         * @brief Shutdown editor (destroy resources)
         */
        void Shutdown();
        
        /**
         * @brief Render editor UI (in separate SDL3 window)
         */
        void Render();
        
        /**
         * @brief Process SDL events for editor window
         */
        void ProcessEvent(SDL_Event* event);
        
        /**
         * @brief Toggle visibility (F9)
         */
        void ToggleVisibility();
        
        /**
         * @brief Check if editor is visible
         */
        bool IsVisible() const { return m_isVisible; }
        
    private:
        // ═══════════════════════════════════════════════════════════════
        // WINDOW MANAGEMENT
        // ═══════════════════════════════════════════════════════════════
        void CreateSeparateWindow();
        void DestroySeparateWindow();
        void RenderInSeparateWindow();
        
        SDL_Window* m_window = nullptr;
        SDL_Renderer* m_renderer = nullptr;
        ImGuiContext* m_imguiContext = nullptr;
        bool m_isVisible = false;
        bool m_isInitialized = false;
        
        // ═══════════════════════════════════════════════════════════════
        // UI PANELS
        // ═══════════════════════════════════════════════════════════════
        void RenderLeftPanel();       // Banks + Spritesheets list
        void RenderCenterPanel();     // Spritesheet viewer + Preview
        void RenderRightPanel();      // Properties inspector
        
        float m_panelLeftWidth = 280.0f;
        float m_panelRightWidth = 330.0f;
        
        // ═══════════════════════════════════════════════════════════════
        // DATA MANAGEMENT
        // ═══════════════════════════════════════════════════════════════
        void ScanAnimationDirectory();
        void LoadBank(const std::string& filepath);
        void SaveBank(const std::string& filepath);
        void CreateNewBank();
        void CloseCurrentBank();
        
        std::vector<std::string> m_bankPaths;       // Scanned bank files
        std::vector<std::string> m_spritesheetPaths;// Scanned spritesheet files
        
        AnimationBank m_currentBank;                // Bank en cours d'édition
        std::string m_currentBankPath;              // Chemin du fichier bank
        bool m_bankModified = false;                // Dirty flag
        
        // ═══════════════════════════════════════════════════════════════
        // SPRITESHEET MANAGEMENT
        // ═══════════════════════════════════════════════════════════════
        int m_selectedSpritesheetIndex = -1;        // Index dans m_currentBank.spritesheets
        int m_selectedSequenceIndex = -1;           // Index dans sequences
        
        void AddSpritesheetToBank(const std::string& imagePath);
        void RemoveSpritesheetFromBank(int index);
        void AutoDetectGridLayout(SpritesheetInfo& sheet, SDL_Texture* texture);
        
        // ═══════════════════════════════════════════════════════════════
        // SEQUENCE MANAGEMENT
        // ═══════════════════════════════════════════════════════════════
        void CreateNewSequence();
        void DuplicateSequence(int index);
        void DeleteSequence(int index);
        void UpdateSequenceFromProperties();
        
        // ═══════════════════════════════════════════════════════════════
        // PREVIEW & VIEWER
        // ═══════════════════════════════════════════════════════════════
        SpritesheetViewer m_viewer;
        AnimationPreview m_preview;
        
        // ═══════════════════════════════════════════════════════════════
        // UI STATE
        // ═══════════════════════════════════════════════════════════════
        char m_searchFilter[256] = "";
        bool m_showBanksInList = true;
        bool m_showSpritesheetsInList = false;
        
        // Sequence properties editing (temp buffer)
        struct SequenceEditBuffer {
            char name[64] = "";
            int spritesheetIndex = 0;
            int startFrame = 0;
            int frameCount = 1;
            float frameDuration = 0.1f;
            bool loop = true;
            float speed = 1.0f;
            int nextAnimIndex = -1;  // -1 = none
        } m_seqEditBuffer;
        
        void LoadSequenceIntoEditBuffer(const AnimationSequence& seq);
        void ApplyEditBufferToSequence(AnimationSequence& seq);
        
        // ═══════════════════════════════════════════════════════════════
        // HELPERS
        // ═══════════════════════════════════════════════════════════════
        std::string GetRelativePath(const std::string& absolutePath) const;
        void MarkModified();
        bool PromptSaveChanges();
    };
    
} // namespace Olympe




Classe Helper: SpritesheetViewer
Header (Animation/SpritesheetViewer.h):
C++


/**
 * @file SpritesheetViewer.h
 * @brief Spritesheet display with zoom, pan, grid overlay, and frame selection
 * @author Atlasbruce
 * @date 2026-02-16
 */

#pragma once

#include "../vector.h"
#include "AnimationTypes.h"
#include <SDL3/SDL.h>
#include <string>

namespace Olympe
{
    /**
     * @class SpritesheetViewer
     * @brief Spritesheet display with zoom, pan, grid overlay, and frame selection
     */
    class SpritesheetViewer
    {
    public:
        SpritesheetViewer();
        ~SpritesheetViewer();
        
        /**
         * @brief Initialize viewer with renderer
         */
        void Initialize(SDL_Renderer* renderer);
        
        /**
         * @brief Render spritesheet with grid overlay
         * @param texture Spritesheet texture
         * @param sheet Spritesheet metadata (columns, rows, etc.)
         * @param highlightFrame Frame to highlight (-1 = none)
         */
        void Render(SDL_Texture* texture, const SpritesheetInfo& sheet, int highlightFrame = -1);
        
        /**
         * @brief Handle mouse input for zoom/pan/selection
         */
        void ProcessInput();
        
        /**
         * @brief Get selected frame index (or -1 if none)
         */
        int GetSelectedFrame() const { return m_selectedFrame; }
        
        /**
         * @brief Get selected frame range (for multi-select)
         */
        void GetSelectedFrameRange(int& outStart, int& outCount) const {
            outStart = m_selectionStart;
            outCount = m_selectionCount;
        }
        
        /**
         * @brief Reset view (center + zoom 100%)
         */
        void ResetView();
        
        /**
         * @brief Fit spritesheet to viewport
         */
        void FitToView(int textureWidth, int textureHeight);
        
        /**
         * @brief Toggle grid visibility
         */
        void ToggleGrid() { m_showGrid = !m_showGrid; }
        bool IsGridVisible() const { return m_showGrid; }
        
    private:
        SDL_Renderer* m_renderer = nullptr;
        
        // View transform
        float m_zoom = 1.0f;           // 0.1x to 10x
        Vector m_pan = {0, 0};         // Pan offset
        
        // Grid
        bool m_showGrid = true;
        
        // Selection
        bool m_isSelecting = false;
        int m_selectedFrame = -1;
        int m_selectionStart = 0;
        int m_selectionCount = 1;
        SDL_Rect m_selectionRect = {0, 0, 0, 0};
        
        // Mouse state
        Vector m_lastMousePos = {0, 0};
        bool m_isPanning = false;
        
        // Helpers
        Vector ScreenToWorld(const Vector& screenPos) const;
        Vector WorldToScreen(const Vector& worldPos) const;
        int GetFrameAtPosition(const Vector& worldPos, const SpritesheetInfo& sheet) const;
        SDL_FRect GetFrameRect(int frameIndex, const SpritesheetInfo& sheet) const;
    };
    
} // namespace Olympe




Classe Helper: AnimationPreview
Header (Animation/AnimationPreview.h):
C++


/**
 * @file AnimationPreview.h
 * @brief Animation playback controller with timeline
 * @author Atlasbruce
 * @date 2026-02-16
 */

#pragma once

#include "AnimationTypes.h"
#include <SDL3/SDL.h>

namespace Olympe
{
    /**
     * @class AnimationPreview
     * @brief Animation playback controller with timeline
     */
    class AnimationPreview
    {
    public:
        AnimationPreview();
        ~AnimationPreview();
        
        /**
         * @brief Initialize preview with renderer
         */
        void Initialize(SDL_Renderer* renderer);
        
        /**
         * @brief Set animation sequence to preview
         * @param bank Animation bank containing sequence
         * @param sequenceName Name of sequence to preview
         */
        void SetSequence(const AnimationBank& bank, const std::string& sequenceName);
        
        /**
         * @brief Update animation (advance frames)
         * @param deltaTime Frame time (seconds)
         */
        void Update(float deltaTime);
        
        /**
         * @brief Render preview (animated sprite + controls)
         */
        void Render();
        
        /**
         * @brief Playback controls
         */
        void Play();
        void Pause();
        void Stop();
        void ToggleLoop();
        void SetSpeed(float speed);
        void SetFrame(int frameIndex);
        
        /**
         * @brief Get current state
         */
        bool IsPlaying() const { return m_isPlaying; }
        bool IsLooping() const { return m_isLooping; }
        int GetCurrentFrame() const { return m_currentFrame; }
        int GetTotalFrames() const;
        float GetSpeed() const { return m_speed; }
        
    private:
        SDL_Renderer* m_renderer = nullptr;
        
        const AnimationBank* m_bank = nullptr;
        const AnimationSequence* m_sequence = nullptr;
        SDL_Texture* m_texture = nullptr;
        
        // Playback state
        bool m_isPlaying = false;
        bool m_isLooping = true;
        int m_currentFrame = 0;
        float m_frameTimer = 0.0f;
        float m_speed = 1.0f;
        
        // Helpers
        void AdvanceFrame();
        SDL_FRect GetCurrentFrameSrcRect() const;
    };
    
} // namespace Olympe




Extension DataManager
Ajouter à DataManager.h:
C++


class DataManager {
public:
    // ✅ NOUVEAU: Texture cache management
    void SetRenderer(SDL_Renderer* renderer);
    SDL_Texture* GetOrLoadTexture(const std::string& path);
    void UnloadTexture(const std::string& path);
    void ReloadTexture(const std::string& path);
    void ClearTextureCache();
    
    // ✅ NOUVEAU: Animation-specific scanning
    std::vector<std::string> ScanAnimationBanks(
        const std::string& directory = "GameData/Animations/Banks/");
    
    std::vector<std::string> ScanSpritesheets(
        const std::string& directory = "GameData/Animations/",
        bool recursive = true);
    
    std::vector<std::string> ScanAnimationGraphs(
        const std::string& directory = "GameData/Animations/Graphs/");
    
private:
    SDL_Renderer* m_renderer = nullptr;
    std::unordered_map<std::string, SDL_Texture*> m_textureCache;
    
    // Helper: Scan directory for extensions
    std::vector<std::string> ScanDirectoryForExtensions(
        const std::string& directory,
        const std::vector<std::string>& extensions,
        bool recursive = true);
};


Implémentation (DataManager.cpp):
C++


void DataManager::SetRenderer(SDL_Renderer* renderer)
{
    m_renderer = renderer;
}

SDL_Texture* DataManager::GetOrLoadTexture(const std::string& path)
{
    std::lock_guard<std::mutex> lock(m_mutex_);
    
    // Check cache
    auto it = m_textureCache.find(path);
    if (it != m_textureCache.end())
    {
        return it->second;  // ✅ Already loaded
    }
    
    // Load with fallback scan
    std::string actualPath = path;
    std::ifstream testFile(path);
    if (!testFile.good())
    {
        actualPath = FindResourceRecursive(path, "GameData");
        if (actualPath.empty())
        {
            std::cout << "[DataManager] ERROR: Cannot find texture: " << path << std::endl;
            return nullptr;
        }
    }
    testFile.close();
    
    // Load texture
    SDL_Texture* texture = IMG_LoadTexture(m_renderer, actualPath.c_str());
    if (!texture)
    {
        std::cout << "[DataManager] ERROR: Failed to load texture: " << actualPath 
                  << " - " << SDL_GetError() << std::endl;
        return nullptr;
    }
    
    // Cache it
    m_textureCache[path] = texture;
    std::cout << "[DataManager] Loaded texture: " << actualPath << std::endl;
    
    return texture;
}

void DataManager::UnloadTexture(const std::string& path)
{
    std::lock_guard<std::mutex> lock(m_mutex_);
    
    auto it = m_textureCache.find(path);
    if (it != m_textureCache.end())
    {
        SDL_DestroyTexture(it->second);
        m_textureCache.erase(it);
        std::cout << "[DataManager] Unloaded texture: " << path << std::endl;
    }
}

std::vector<std::string> DataManager::ScanAnimationBanks(const std::string& directory)
{
    return ScanDirectoryForExtensions(directory, {".json"}, true);
}

std::vector<std::string> DataManager::ScanSpritesheets(const std::string& directory, bool recursive)
{
    return ScanDirectoryForExtensions(directory, {".png", ".jpg", ".bmp"}, recursive);
}

std::vector<std::string> DataManager::ScanDirectoryForExtensions(
    const std::string& directory,
    const std::vector<std::string>& extensions,
    bool recursive)
{
    std::vector<std::string> results;
    
    // Platform-specific implementation
#ifdef _WIN32
    // Windows: Use FindFirstFileA/FindNextFileA
    // (voir implémentation FindResourceRecursive_Windows)
#else
    // Unix: Use dirent.h
    // (voir implémentation FindResourceRecursive_Unix)
#endif
    
    return results;
}




🔗 INTÉGRATION SYSTÈME
1. Hook dans GameEngine
GameEngine.cpp:
C++


#include "Animation/AnimationEditorWindow.h"

class GameEngine {
private:
    Olympe::AnimationEditorWindow m_animationEditor;
    
public:
    void Initialize() {
        // ... existing code ...
        
        // Initialize DataManager renderer (for texture cache)
        DataManager::Get().SetRenderer(m_renderer);
        
        // Initialize Animation Editor
        m_animationEditor.Initialize();
        
        std::cout << "[GameEngine] Animation Editor initialized (press F9)" << std::endl;
    }
    
    void ProcessEvent(SDL_Event* event) {
        // ... existing code ...
        
        // Animation Editor events
        m_animationEditor.ProcessEvent(event);
        
        // F9 toggle
        if (event->type == SDL_EVENT_KEY_DOWN && event->key.key == SDLK_F9)
        {
            m_animationEditor.ToggleVisibility();
        }
    }
    
    void Render() {
        // ... existing game rendering ...
        
        // Render Animation Editor (separate window)
        m_animationEditor.Render();
    }
    
    void Shutdown() {
        // ... existing code ...
        m_animationEditor.Shutdown();
    }
};




2. AnimationManager Loader (Extended)
Animation/AnimationManager.cpp (modifier):
C++


bool AnimationManager::LoadAnimationBank(const std::string& filePath)
{
    std::string jsonContent;
    if (!DataManager::Get().LoadTextFile(filePath, jsonContent))
    {
        std::cout << "[AnimationManager] ERROR: Cannot read file: " << filePath << std::endl;
        return false;
    }
    
    json bankJson;
    try {
        bankJson = json::parse(jsonContent);
    } catch (const json::exception& e) {
        std::cout << "[AnimationManager] ERROR: JSON parse error: " << e.what() << std::endl;
        return false;
    }
    
    // Detect schema version
    int schemaVersion = json_get_int(bankJson, "schema_version", 1);
    
    AnimationBank bank;
    
    if (schemaVersion == 2)
    {
        // ✅ Parse schema v2 (multi-spritesheets)
        bank.bankId = json_get_string(bankJson, "bankId", "");
        bank.description = json_get_string(bankJson, "description", "");
        
        // Parse spritesheets
        if (bankJson.contains("spritesheets") && bankJson["spritesheets"].is_array())
        {
            for (const auto& sheetJson : bankJson["spritesheets"])
            {
                SpritesheetInfo sheet;
                sheet.id = json_get_string(sheetJson, "id", "");
                sheet.path = json_get_string(sheetJson, "path", "");
                sheet.frameWidth = json_get_int(sheetJson, "frameWidth", 32);
                sheet.frameHeight = json_get_int(sheetJson, "frameHeight", 32);
                sheet.columns = json_get_int(sheetJson, "columns", 1);
                sheet.rows = json_get_int(sheetJson, "rows", 1);
                sheet.totalFrames = json_get_int(sheetJson, "totalFrames", 1);
                sheet.spacing = json_get_int(sheetJson, "spacing", 0);
                sheet.margin = json_get_int(sheetJson, "margin", 0);
                
                if (sheetJson.contains("hotspot") && sheetJson["hotspot"].is_object())
                {
                    const auto& hotspotJson = sheetJson["hotspot"];
                    sheet.hotspot.x = json_get_float(hotspotJson, "x", 0.0f);
                    sheet.hotspot.y = json_get_float(hotspotJson, "y", 0.0f);
                }
                
                bank.spritesheets.push_back(sheet);
            }
        }
        
        // Parse sequences
        if (bankJson.contains("sequences") && bankJson["sequences"].is_array())
        {
            for (const auto& seqJson : bankJson["sequences"])
            {
                AnimationSequence seq;
                seq.name = json_get_string(seqJson, "name", "");
                seq.spritesheetId = json_get_string(seqJson, "spritesheetId", "");
                seq.loop = json_get_bool(seqJson, "loop", true);
                seq.speed = json_get_float(seqJson, "speed", 1.0f);
                seq.nextAnimation = json_get_string(seqJson, "nextAnimation", "");
                
                // Parse frame range
                if (seqJson.contains("frames") && seqJson["frames"].is_object())
                {
                    const auto& framesJson = seqJson["frames"];
                    seq.startFrame = json_get_int(framesJson, "start", 0);
                    seq.frameCount = json_get_int(framesJson, "count", 1);
                }
                
                seq.frameDuration = json_get_float(seqJson, "frameDuration", 0.1f);
                
                bank.sequences.push_back(seq);
            }
        }
    }
    else if (schemaVersion == 1)
    {
        // ✅ Parse schema v1 (retrocompatibilité)
        // (Code de conversion V1 → V2, voir section précédente)
    }
    
    // Store bank
    m_banks[bank.bankId] = std::make_unique<AnimationBank>(bank);
    
    std::cout << "[AnimationManager] Loaded bank: " << bank.bankId 
              << " (" << bank.spritesheets.size() << " spritesheets, "
              << bank.sequences.size() << " sequences)" << std::endl;
    
    return true;
}




3. AnimationSystem (Update Logic)
Animation/AnimationSystem.cpp:
C++


#include "AnimationHelpers.h"

void AnimationSystem::Update(float deltaTime)
{
    auto& world = World::Get();
    
    // For each entity with animation
    for (EntityID entity : m_entities)
    {
        auto& animData = world.GetComponent<VisualAnimation_data>(entity);
        
        if (!animData.isPlaying)
            continue;
        
        // Get bank
        const AnimationBank* bank = AnimationManager::Get().GetBank(animData.bankId);
        if (!bank)
            continue;
        
        // Get sequence
        const AnimationSequence* seq = bank->GetSequence(animData.currentAnimName);
        if (!seq)
            continue;
        
        // Get spritesheet
        const SpritesheetInfo* sheet = bank->GetSpritesheet(seq->spritesheetId);
        if (!sheet)
            continue;
        
        // ✅ If has FSM graph, evaluate transitions
        if (!animData.animGraphPath.empty())
        {
            const AnimationGraph* graph = AnimationManager::Get().GetGraph(animData.animGraphPath);
            if (graph)
            {
                std::string nextState = EvaluateTransitions(graph, animData);
                
                if (nextState != animData.currentAnimName)
                {
                    ChangeAnimation(entity, nextState);
                    continue;  // Sequence changed, restart frame timer
                }
            }
        }
        
        // Update frame timer
        animData.frameTimer += deltaTime * animData.playbackSpeed;
        
        // Check if need to advance frame
        float frameDuration = seq->frameDuration / seq->speed;
        if (animData.frameTimer >= frameDuration)
        {
            animData.frameTimer -= frameDuration;
            animData.currentFrameIndex++;
            
            // Check loop
            if (animData.currentFrameIndex >= seq->frameCount)
            {
                if (seq->loop && animData.loop)
                {
                    animData.currentFrameIndex = 0;
                }
                else
                {
                    animData.currentFrameIndex = seq->frameCount - 1;
                    animData.isPlaying = false;
                    
                    // Transition to next animation
                    if (!seq->nextAnimation.empty())
                    {
                        ChangeAnimation(entity, seq->nextAnimation);
                    }
                }
            }
        }
        
        // Update VisualSprite_data.srcRect
        if (world.HasComponent<VisualSprite_data>(entity))
        {
            auto& sprite = world.GetComponent<VisualSprite_data>(entity);
            
            int absoluteFrame = seq->startFrame + animData.currentFrameIndex;
            int row = absoluteFrame / sheet->columns;
            int col = absoluteFrame % sheet->columns;
            
            sprite.srcRect.x = static_cast<float>(sheet->margin + col * (sheet->frameWidth + sheet->spacing));
            sprite.srcRect.y = static_cast<float>(sheet->margin + row * (sheet->frameHeight + sheet->spacing));
            sprite.srcRect.w = static_cast<float>(sheet->frameWidth);
            sprite.srcRect.h = static_cast<float>(sheet->frameHeight);
            
            // Update hotspot
            sprite.hotSpot = sheet->hotspot;
        }
    }
}

std::string AnimationSystem::EvaluateTransitions(
    const AnimationGraph* graph,
    const VisualAnimation_data& animData)
{
    std::string currentState = animData.currentAnimName;
    
    // For each transition from current state
    for (const auto& transition : graph->transitions)
    {
        if (transition.from != currentState && transition.from != "ANY")
            continue;
        
        // ✅ Check conditions using AnimationHelpers
        bool allConditionsMet = true;
        for (const auto& condition : transition.conditions)
        {
            bool conditionMet = false;
            
            if (condition.type == ParameterType::Bool)
            {
                bool value = AnimationHelpers::GetBoolParameter(animData, condition.parameter, false);
                conditionMet = (value == condition.boolValue);
            }
            else if (condition.type == ParameterType::Float)
            {
                float value = AnimationHelpers::GetFloatParameter(animData, condition.parameter, 0.0f);
                conditionMet = EvaluateComparison(value, condition.op, condition.floatValue);
            }
            else if (condition.type == ParameterType::Int)
            {
                int value = AnimationHelpers::GetIntParameter(animData, condition.parameter, 0);
                conditionMet = EvaluateComparison(value, condition.op, condition.intValue);
            }
            
            if (!conditionMet)
            {
                allConditionsMet = false;
                break;
            }
        }
        
        if (allConditionsMet)
        {
            return transition.to;  // ✅ Transition valid
        }
    }
    
    return currentState;  // No transition
}




📋 WORKFLOW UTILISATEUR
Scenario 1: Créer une Nouvelle Bank (Multi-Spritesheets)
Étape 1: Ouvrir l'éditeur

Lancer le jeu
Appuyer sur F9
Fenêtre séparée "Animation Editor" s'ouvre
Étape 2: Créer nouvelle bank

Menu → File > New Bank...
Dialog popup:Bank ID: thesee (unique, alphanumeric + underscore)
Description: Thésée character animations
Author: Atlasbruce
Cliquer Create
Bank vide créée, apparaît dans la liste (panel gauche)
Étape 3: Ajouter spritesheets
Pour chaque spritesheet (idle, walk, run, hit):

Dans Panel Gauche, cliquer + Add Spritesheet (ou bouton dans panel droit)
File browser → Sélectionner thesee_idle.png
Dialog popup "Add Spritesheet":ID: thesee_idle (auto-généré depuis filename)
Description: Idle breathing animation
Path: ./Gamedata/Minotaurus/texture/thesee_idle.png (auto-rempli)
Cliquer Auto-Detect Grid:Analyse image
Détecte: Columns: 8, Rows: 16, Frame size: 46×81
Total frames: 128
Ajuster si nécessaire (généralement correct)
Hotspot:Cliquer preset Bottom → Sets (23, 70) automatiquement
Ou input manuel
Cliquer Add
Spritesheet apparaît dans tab "thesee_idle" (panel centre)
Répéter pour walk, run, hit (total 4 spritesheets)
Étape 4: Créer séquences
Pour chaque séquence (idle, walk, run, hit):

Sélectionner tab correspondant (ex: "Idle")
Panel Droit → Cliquer + New Sequence
Remplir propriétés:Name: idle
Spritesheet: thesee_idle (dropdown)
Start Frame: 0
Frame Count: 128 (use all frames)
Frame Duration: 0.08s (12.5 FPS)
Loop: ✓ Enabled
Speed: 1.0x
Next Animation: (none)
Cliquer Apply
Séquence ajoutée, visible dans liste (panel droit bas)
Répéter pour walk, run, hit avec durées différentes:

walk: 0.06s (16.67 FPS)
run: 0.05s (20 FPS)
hit: 0.04s (25 FPS), Loop: ✗, Next: idle
Étape 5: Preview animation

Panel Droit → Liste séquences → Cliquer séquence "idle"
Preview charge automatiquement (panel centre bas)
Cliquer Play ▶
Animation joue en boucle
Ajuster speed si besoin (slider 0.1x à 5x)
Tester scrubber (drag timeline)
Tester keyboard shortcuts (Space = play/pause)
Répéter pour toutes séquences pour vérifier
Étape 6: Sauvegarder

Menu → File > Save Bank (Ctrl+S)
File dialog (optionnel, default: GameData/Animations/Banks/thesee.json)
Fichier sauvé au format JSON v2
Status bar: "Saved successfully at [timestamp]"
Dirty flag cleared (plus de [Modified])
Résultat: Fichier GameData/Animations/Banks/thesee.json créé avec 4 spritesheets et 4 séquences.


Scenario 2: Éditer une Bank Existante
Étape 1: Charger bank

Ouvrir éditeur (F9)
Panel Gauche → Cliquer bank thesee dans la liste
Bank chargée, spritesheets apparaissent dans tabs
Séquences chargées dans liste (panel droit bas)
Étape 2: Modifier séquence

Sélectionner séquence "walk" dans la liste
Panel Droit → Inspector affiche propriétés
Modifier frameDuration: 0.06 → 0.05
FPS recalculé automatiquement: 16.67 → 20.0
Cliquer Apply
Preview met à jour automatiquement
Tester avec Play pour vérifier vitesse
Étape 3: Ajouter nouvelle séquence

Sélectionner tab "Hit" (spritesheet)
Panel Droit → + New Sequence
Propriétés:Name: special_attack
Spritesheet: thesee_hit
Start: 50 (deuxième moitié du spritesheet)
Count: 62 (reste du spritesheet, 112 - 50)
Duration: 0.05s
Loop: ✗
Next Anim: idle
Apply
Séquence ajoutée à la liste
Étape 4: Sauvegarder modifications

Status bar affiche: "Modified: Yes"
Ctrl+S (ou Menu Save)
Confirmation dialog (optionnel): "Overwrite existing file?"
Fichier écrasé avec nouvelles données
Status: "Changes saved"


Scenario 3: Utiliser Bank dans un Prefab
Étape 1: Créer bank (si pas déjà fait)

Voir Scenario 1
Étape 2: Référencer dans prefab JSON
Créer/Éditer: GameData/EntityPrefab/thesee.json
JSON


{
  "schema_version": 2,
  "type": "Character",
  "name": "Thesee",
  "components": [
    {
      "type": "Identity_data",
      "properties": {
        "name": "Thésée",
        "tag": "Player",
        "entityType": "Player"
      }
    },
    {
      "type": "Position_data",
      "properties": {
        "position": { "x": 100, "y": 100, "z": 0 }
      }
    },
    {
      "type": "VisualAnimation_data",
      "properties": {
        "bankId": "thesee",               ← Référence à la bank
        "currentAnimName": "idle",        ← Animation initiale
        "playbackSpeed": 1.0,
        "isPlaying": true,
        "loop": true
      }
    }
  ]
}


Étape 3: Charger au runtime
Dans code d'initialisation niveau:
C++


// 1. Load animation banks
AnimationManager::Get().LoadAnimationBanks("GameData/Animations/Banks/");

// 2. Instantiate prefab
EntityID theseusEntity = EntityFactory::CreateFromJSON("GameData/EntityPrefab/thesee.json");

// 3. VisualAnimation_data is created with bankId = "thesee"
// 4. AnimationSystem::Update() will play "idle" animation automatically


Résultat: Entité Thésée affichée avec animation idle qui joue en boucle.


✅ PLAN D'IMPLÉMENTATION
Phase 1.1: Infrastructure (Semaine 1)
Objectifs:

Fenêtre séparée SDL3 fonctionnelle (F9)
Contexte ImGui isolé
Scan récursif GameData/Animations/
Tasks:

 Créer AnimationEditorWindow (header + cpp)
 Hook F9 dans GameEngine
 Créer/détruire fenêtre SDL3 (CreateSeparateWindow/Destroy)
 Contexte ImGui séparé (CreateContext/DestroyContext)
 Scan directory (banks, spritesheets, graphs)
 Layout 3-panels basique (vide, juste splitters)
Validation:

✅ F9 ouvre/ferme fenêtre (toggle fonctionne)
✅ Fenêtre indépendante du jeu (peut bouger sur second écran)
✅ Liste fichiers scannés dans console (banks + sheets)
✅ Pas de crash au close (cleanup correct)


Phase 1.2: DataManager Extensions (Semaine 1-2)
Objectifs:

Texture cache fonctionnel
Scan methods pour animations
Tasks:

 Ajouter SetRenderer() dans DataManager
 Implémenter GetOrLoadTexture() avec cache (std::unordered_map)
 Implémenter UnloadTexture() et ClearTextureCache()
 Implémenter ScanAnimationBanks()
 Implémenter ScanSpritesheets() avec recursive option
 Tests unitaires cache (load, reload, clear)
Validation:

✅ Texture chargée une seule fois (vérifier pointeur identique)
✅ Scan retourne tous les fichiers JSON/PNG récursivement
✅ Fallback scan fonctionne (fichier pas à path exact)
✅ Pas de memory leak (Valgrind/sanitizers)


Phase 1.3: Bank CRUD (Semaine 2)
Objectifs:

Créer, charger, sauvegarder banks
Tasks:

 UI: Bouton "New Bank" (dialog avec ID, desc, author)
 UI: Liste banks scannées (panel gauche, hierarchical)
 UI: Formulaire propriétés bank (panel droit, section Bank Info)
 Fonction CreateNewBank() (init AnimationBank struct)
 Fonction LoadBank(filepath) (JSON → AnimationBank)
 Fonction SaveBank(filepath) (AnimationBank → JSON v2)
 Parser JSON v2 → AnimationBank (schema_version = 2)
 Serializer AnimationBank → JSON v2 (with metadata)
Validation:

✅ Créer bank vide (ID unique, pas de collision)
✅ Charger bank existante (thesee) sans erreur
✅ Sauvegarder bank (vérifier JSON valide)
✅ Reload bank (pas de perte de données)
✅ Validation ID (alphanumeric + underscore only)


Phase 1.4: Multi-Spritesheets Support (Semaine 2-3)
Objectifs:

Ajouter/retirer spritesheets dans une bank
Tabs pour naviguer entre spritesheets
Tasks:

 Extend AnimationBank avec std::vector<SpritesheetInfo> (déjà fait dans structures)
 UI: Bouton "Add Spritesheet" (dialog file browser)
 UI: Tabs pour chaque spritesheet (ImGui TabBar)
 Fonction AddSpritesheetToBank(imagePath)
 Fonction RemoveSpritesheetFromBank(index)
 Auto-detect grid (AutoDetectGridLayout())Scan vertical pour colonnes
Scan horizontal pour lignes
Détecter spacing/margin
Validation:

✅ Ajouter 4 spritesheets (idle, walk, run, hit)
✅ Tabs affichent noms corrects (id extrait du filename)
✅ Auto-detect colonnes/lignes correct (comparer avec valeurs attendues)
✅ Supprimer spritesheet fonctionne (tab disparaît)


Phase 1.5: Spritesheet Viewer (Semaine 3)
Objectifs:

Afficher spritesheet avec grid overlay
Zoom/Pan fonctionnels
Frame selection (visual)
Tasks:

 Créer classe SpritesheetViewer (header + cpp)
 Render texture avec SDL_RenderTexture()
 Zoom (molette souris, 0.1x à 10x)
 Pan (MMB drag ou Space+LMB drag)
 Grid overlay (lignes blanches semi-transparentes)
 Highlight frame sous souris (border jaune)
 Frame selection (drag rectangle, shift+click)
 Numérotation frames (overlay text)
Validation:

✅ Spritesheet visible dans centre panel
✅ Zoom in/out smooth (pas de saccades)
✅ Pan fonctionne (drag avec MMB)
✅ Grid overlay correct (colonnes/lignes alignées)
✅ Frame highlight au survol (border jaune)
✅ Frame selection (drag rectangle, frames verts)


Phase 1.6: Sequence Editor (Semaine 3-4)
Objectifs:

Créer/éditer séquences d'animation
Formulaire propriétés (name, start, count, duration...)
Tasks:

 UI: Bouton "New Sequence" (panel droit)
 UI: Formulaire propriétés séquence (panel droit, section Sequence)
 UI: Liste séquences (panel droit bas, checkbox + drag)
 Fonction CreateNewSequence() (init AnimationSequence)
 Fonction DuplicateSequence(index) (copy + rename)
 Fonction DeleteSequence(index) (confirmation dialog)
 Fonction UpdateSequenceFromProperties() (apply changes)
 Validation (start + count ≤ totalFrames)
Validation:

✅ Créer séquence "idle" (0-127 frames)
✅ Modifier duration → recalcule FPS (affiché)
✅ Dupliquer séquence (nouveau nom unique)
✅ Supprimer séquence (confirmation + disparaît liste)
✅ Validation range (erreur si hors bounds)


Phase 1.7: Animation Preview (Semaine 4)
Objectifs:

Playback animation en temps réel
Contrôles Play/Pause/Stop
Timeline scrubber
Tasks:

 Créer classe AnimationPreview (header + cpp)
 Fonction SetSequence(bank, sequenceName) (charge texture + sequence)
 Fonction Update(deltaTime) → avancer frames
 Render sprite animé (centre écran, background checkered)
 UI: Boutons Play/Pause/Stop (ImGui)
 UI: Timeline scrubber (slider horizontal)
 UI: Speed slider (0.1x à 5x, dropdown + custom)
 UI: Loop toggle (checkbox)
 Hotspot visible (croix rouge)
Validation:

✅ Animation joue à 16 FPS (duration 0.06s, vérifier timing)
✅ Pause fonctionne (frame statique)
✅ Scrubber change frame (drag timeline)
✅ Speed 2x double vitesse (observer)
✅ Loop revient frame 0 (pas d'arrêt)
✅ Hotspot visible (croix rouge centrée)


Phase 1.8: Save/Load JSON v2 (Semaine 4-5)
Objectifs:

Sauvegarder bank au format JSON v2 unifié
Charger avec rétrocompatibilité v1
Tasks:

 Serializer complet (AnimationBank → JSON v2)spritesheets array
sequences array
metadata object
 Parser rétrocompatible (v1 → v2 conversion)Détecter schema_version
Convertir animations object → sequences array
Créer spritesheets depuis paths
 Validation schema JSON (required fields)
 Tests de round-trip (save → load → verify)
Validation:

✅ Sauvegarder bank thesee → JSON valide (parser externe)
✅ Charger JSON v2 → bank identique (compare structs)
✅ Charger ancien JSON v1 → converti en v2 (pas d'erreur)
✅ Pas de perte de données (all fields preserved)


Phase 1.9: Integration Tests (Semaine 5)
Objectifs:

Tester workflow complet end-to-end
Tasks:

 Test: Créer bank depuis zéro (thesee, 4 sheets, 4 sequences)
 Test: Charger bank existante (player, thesee)
 Test: Modifier séquence + save (reload + verify)
 Test: Prefab utilise bank (create entity + runtime)
 Test: AnimationSystem joue animation runtime (visual check)
 Test: Memory leaks (Valgrind full run)
 Test: Cross-platform (Windows + Linux)
Validation:

✅ Workflow complet fonctionne (zéro erreur console)
✅ Aucun crash (stable 30 minutes)
✅ Pas de memory leak (Valgrind clean)
✅ Cross-platform (compile + run sur Windows et Linux)


Phase 1.10: Polish & Documentation (Semaine 5-6)
Objectifs:

UI polish (icons, tooltips, shortcuts)
Documentation utilisateur
Tasks:

 Ajouter icons (📦 🖼️ 🎬 ⚙️ dans UI)
 Tooltips sur tous les contrôles (hover descriptions)
 Keyboard shortcuts (Ctrl+S, Ctrl+N, F5, Space, G, F, R...)
 Status bar (modified, saved, errors, timestamp)
 Help menu avec controls (F1 ou Help button)
 Créer guide utilisateur (Markdown, avec screenshots)
 Créer vidéo tutorial (optionnel, 5-10 min)
Validation:

✅ UI intuitive (test user novice)
✅ Documentation complète (couvre tous workflows)
✅ Tooltips utiles (pas de confusion)


🧪 VALIDATION ET TESTS
Tests Unitaires
Test 1: DataManager Texture Cache
C++


TEST(DataManager, TextureCache)
{
    auto& dm = DataManager::Get();
    dm.SetRenderer(testRenderer);
    
    // First load
    SDL_Texture* tex1 = dm.GetOrLoadTexture("./test_sprite.png");
    ASSERT_NE(tex1, nullptr);
    
    // Second load (should return same pointer)
    SDL_Texture* tex2 = dm.GetOrLoadTexture("./test_sprite.png");
    ASSERT_EQ(tex1, tex2);  // Same pointer = cached
    
    // Unload
    dm.UnloadTexture("./test_sprite.png");
    
    // Load again (should reload)
    SDL_Texture* tex3 = dm.GetOrLoadTexture("./test_sprite.png");
    ASSERT_NE(tex3, nullptr);
    ASSERT_NE(tex3, tex1);  // Different pointer = reloaded
}




Copilot said: ```cpp TEST(AnimationBank, MultiSpritesheets) { AnimationBank bank;
C++


TEST(AnimationBank, MultiSpritesheets)
{
    AnimationBank bank;
    bank.bankId = "test";
    
    // Add 2 spritesheets
    SpritesheetInfo sheet1;
    sheet1.id = "test_idle";
    sheet1.frameWidth = 32;
    sheet1.frameHeight = 32;
    sheet1.columns = 4;
    sheet1.rows = 2;
    sheet1.totalFrames = 8;
    bank.spritesheets.push_back(sheet1);
    
    SpritesheetInfo sheet2;
    sheet2.id = "test_walk";
    sheet2.frameWidth = 32;
    sheet2.frameHeight = 32;
    sheet2.columns = 6;
    sheet2.rows = 1;
    sheet2.totalFrames = 6;
    bank.spritesheets.push_back(sheet2);
    
    // Verify
    ASSERT_EQ(bank.spritesheets.size(), 2);
    
    // Get by ID
    const SpritesheetInfo* found = bank.GetSpritesheet("test_idle");
    ASSERT_NE(found, nullptr);
    ASSERT_EQ(found->id, "test_idle");
    ASSERT_EQ(found->totalFrames, 8);
    
    // Get non-existent
    const SpritesheetInfo* notFound = bank.GetSpritesheet("test_invalid");
    ASSERT_EQ(notFound, nullptr);
}




Test 3: JSON Round-Trip (Save → Load → Verify)
C++


TEST(AnimationManager, JSONRoundTrip)
{
    // Create bank with data
    AnimationBank originalBank;
    originalBank.bankId = "test";
    originalBank.description = "Test bank";
    originalBank.author = "Unit Test";
    
    SpritesheetInfo sheet;
    sheet.id = "test_idle";
    sheet.path = "./test_idle.png";
    sheet.frameWidth = 46;
    sheet.frameHeight = 81;
    sheet.columns = 8;
    sheet.rows = 16;
    sheet.totalFrames = 128;
    sheet.hotspot = {23.0f, 70.0f};
    originalBank.spritesheets.push_back(sheet);
    
    AnimationSequence seq;
    seq.name = "idle";
    seq.spritesheetId = "test_idle";
    seq.startFrame = 0;
    seq.frameCount = 128;
    seq.frameDuration = 0.08f;
    seq.loop = true;
    seq.speed = 1.0f;
    originalBank.sequences.push_back(seq);
    
    // Save to JSON
    std::string jsonPath = "./test_bank.json";
    AnimationManager::Get().SaveBank(originalBank, jsonPath);
    
    // Load from JSON
    AnimationBank loadedBank;
    bool success = AnimationManager::Get().LoadBank(jsonPath, loadedBank);
    ASSERT_TRUE(success);
    
    // Verify all fields match
    ASSERT_EQ(loadedBank.bankId, originalBank.bankId);
    ASSERT_EQ(loadedBank.description, originalBank.description);
    ASSERT_EQ(loadedBank.author, originalBank.author);
    
    ASSERT_EQ(loadedBank.spritesheets.size(), 1);
    ASSERT_EQ(loadedBank.spritesheets[0].id, "test_idle");
    ASSERT_EQ(loadedBank.spritesheets[0].frameWidth, 46);
    ASSERT_EQ(loadedBank.spritesheets[0].totalFrames, 128);
    ASSERT_FLOAT_EQ(loadedBank.spritesheets[0].hotspot.x, 23.0f);
    
    ASSERT_EQ(loadedBank.sequences.size(), 1);
    ASSERT_EQ(loadedBank.sequences[0].name, "idle");
    ASSERT_EQ(loadedBank.sequences[0].frameCount, 128);
    ASSERT_FLOAT_EQ(loadedBank.sequences[0].frameDuration, 0.08f);
    
    // Cleanup
    std::remove(jsonPath.c_str());
}




Test 4: Schema V1 → V2 Conversion (Rétrocompatibilité)
C++


TEST(AnimationManager, SchemaV1Conversion)
{
    // Create V1 JSON format
    json v1Json = json::object();
    v1Json["bankId"] = "thesee";
    v1Json["frameWidth"] = 46;
    v1Json["frameHeight"] = 81;
    v1Json["columns"] = 8;
    v1Json["spacing"] = 0;
    v1Json["margin"] = 0;
    
    json animations = json::object();
    json idleAnim = json::object();
    idleAnim["spritesheet"] = "./Gamedata/Minotaurus/texture/thesee_idle.png";
    idleAnim["frames"] = json::object();
    idleAnim["frames"]["start"] = 0;
    idleAnim["frames"]["count"] = 128;
    idleAnim["frameDuration"] = 0.08f;
    idleAnim["loop"] = true;
    idleAnim["speed"] = 1.0f;
    animations["idle"] = idleAnim;
    
    v1Json["animations"] = animations;
    
    // Save V1 JSON to file
    std::string v1Path = "./test_v1.json";
    std::ofstream file(v1Path);
    file << v1Json.dump(2);
    file.close();
    
    // Load with AnimationManager (should auto-convert)
    AnimationBank bank;
    bool success = AnimationManager::Get().LoadBank(v1Path, bank);
    ASSERT_TRUE(success);
    
    // Verify conversion
    ASSERT_EQ(bank.bankId, "thesee");
    
    // Should have created spritesheet from animation
    ASSERT_EQ(bank.spritesheets.size(), 1);
    ASSERT_EQ(bank.spritesheets[0].id, "thesee_idle");
    ASSERT_EQ(bank.spritesheets[0].frameWidth, 46);
    ASSERT_EQ(bank.spritesheets[0].frameHeight, 81);
    ASSERT_EQ(bank.spritesheets[0].columns, 8);
    ASSERT_EQ(bank.spritesheets[0].totalFrames, 128);
    
    // Should have created sequence
    ASSERT_EQ(bank.sequences.size(), 1);
    ASSERT_EQ(bank.sequences[0].name, "idle");
    ASSERT_EQ(bank.sequences[0].spritesheetId, "thesee_idle");
    ASSERT_EQ(bank.sequences[0].startFrame, 0);
    ASSERT_EQ(bank.sequences[0].frameCount, 128);
    ASSERT_FLOAT_EQ(bank.sequences[0].frameDuration, 0.08f);
    ASSERT_TRUE(bank.sequences[0].loop);
    
    // Cleanup
    std::remove(v1Path.c_str());
}




Test 5: AnimationHelpers (ECS Parameter Manipulation)
C++


TEST(AnimationHelpers, SetGetParameters)
{
    VisualAnimation_data animData;
    
    // Set parameters
    AnimationHelpers::SetParameter(animData, "speed", 120.0f);
    AnimationHelpers::SetParameter(animData, "isMoving", true);
    AnimationHelpers::SetParameter(animData, "comboCount", 3);
    
    // Get parameters
    float speed = AnimationHelpers::GetFloatParameter(animData, "speed", 0.0f);
    bool isMoving = AnimationHelpers::GetBoolParameter(animData, "isMoving", false);
    int combo = AnimationHelpers::GetIntParameter(animData, "comboCount", 0);
    
    ASSERT_FLOAT_EQ(speed, 120.0f);
    ASSERT_TRUE(isMoving);
    ASSERT_EQ(combo, 3);
    
    // Get non-existent (should return default)
    float notFound = AnimationHelpers::GetFloatParameter(animData, "invalid", 99.0f);
    ASSERT_FLOAT_EQ(notFound, 99.0f);
    
    // HasParameter
    ASSERT_TRUE(AnimationHelpers::HasParameter(animData, "speed"));
    ASSERT_FALSE(AnimationHelpers::HasParameter(animData, "invalid"));
    
    // Clear all
    AnimationHelpers::ClearParameters(animData);
    ASSERT_FALSE(AnimationHelpers::HasParameter(animData, "speed"));
}




Test 6: Auto-Detect Grid Layout
C++


TEST(SpritesheetViewer, AutoDetectGrid)
{
    // Create test image (8 columns × 16 rows of 46×81 frames)
    const int frameWidth = 46;
    const int frameHeight = 81;
    const int columns = 8;
    const int rows = 16;
    const int imageWidth = columns * frameWidth;
    const int imageHeight = rows * frameHeight;
    
    SDL_Surface* surface = SDL_CreateSurface(imageWidth, imageHeight, SDL_PIXELFORMAT_RGBA32);
    ASSERT_NE(surface, nullptr);
    
    // Fill with alternating pattern (for grid detection)
    for (int row = 0; row < rows; ++row)
    {
        for (int col = 0; col < columns; ++col)
        {
            SDL_Rect rect = {
                col * frameWidth,
                row * frameHeight,
                frameWidth,
                frameHeight
            };
            
            // Alternating colors
            Uint32 color = ((row + col) % 2 == 0) ? 
                SDL_MapRGBA(SDL_GetPixelFormatDetails(surface->format), 255, 0, 0, 255) :
                SDL_MapRGBA(SDL_GetPixelFormatDetails(surface->format), 0, 255, 0, 255);
            
            SDL_FillSurfaceRect(surface, &rect, color);
        }
    }
    
    SDL_Texture* texture = SDL_CreateTextureFromSurface(testRenderer, surface);
    SDL_DestroySurface(surface);
    
    // Auto-detect
    SpritesheetInfo sheet;
    sheet.path = "./test.png";
    AnimationEditorWindow::AutoDetectGridLayout(sheet, texture);
    
    // Verify detection
    ASSERT_EQ(sheet.columns, columns);
    ASSERT_EQ(sheet.rows, rows);
    ASSERT_EQ(sheet.frameWidth, frameWidth);
    ASSERT_EQ(sheet.frameHeight, frameHeight);
    ASSERT_EQ(sheet.totalFrames, columns * rows);
    ASSERT_EQ(sheet.spacing, 0);
    ASSERT_EQ(sheet.margin, 0);
    
    SDL_DestroyTexture(texture);
}




Tests d'Intégration
Test 7: End-to-End Workflow (Editor → JSON → Runtime)
C++


TEST(Integration, EditorToRuntime)
{
    // PHASE 1: Create bank in editor
    AnimationBank bank;
    bank.bankId = "test_char";
    
    SpritesheetInfo sheet;
    sheet.id = "test_char_idle";
    sheet.path = "./test_idle.png";
    sheet.frameWidth = 32;
    sheet.frameHeight = 32;
    sheet.columns = 4;
    sheet.rows = 1;
    sheet.totalFrames = 4;
    bank.spritesheets.push_back(sheet);
    
    AnimationSequence seq;
    seq.name = "idle";
    seq.spritesheetId = "test_char_idle";
    seq.startFrame = 0;
    seq.frameCount = 4;
    seq.frameDuration = 0.2f;
    seq.loop = true;
    bank.sequences.push_back(seq);
    
    // Save
    std::string bankPath = "./GameData/Animations/Banks/test_char.json";
    AnimationManager::Get().SaveBank(bank, bankPath);
    
    // PHASE 2: Load at runtime
    AnimationManager::Get().LoadAnimationBanks("./GameData/Animations/Banks/");
    
    const AnimationBank* loadedBank = AnimationManager::Get().GetBank("test_char");
    ASSERT_NE(loadedBank, nullptr);
    
    // PHASE 3: Create entity with animation
    auto& world = World::Get();
    EntityID entity = world.CreateEntity();
    
    VisualAnimation_data animData;
    animData.bankId = "test_char";
    animData.currentAnimName = "idle";
    animData.isPlaying = true;
    world.AddComponent(entity, animData);
    
    VisualSprite_data spriteData;
    spriteData.sprite = DataManager::Get().GetOrLoadTexture("./test_idle.png");
    world.AddComponent(entity, spriteData);
    
    // PHASE 4: Update animation system (simulate 1 second)
    AnimationSystem animSystem;
    animSystem.Initialize();
    
    for (int frame = 0; frame < 60; ++frame)
    {
        animSystem.Update(1.0f / 60.0f);  // 60 FPS
    }
    
    // Verify animation advanced
    auto& updatedAnimData = world.GetComponent<VisualAnimation_data>(entity);
    ASSERT_GT(updatedAnimData.currentFrameIndex, 0);  // Frame advanced
    ASSERT_TRUE(updatedAnimData.isPlaying);
    
    // Verify sprite srcRect updated
    auto& updatedSprite = world.GetComponent<VisualSprite_data>(entity);
    ASSERT_GT(updatedSprite.srcRect.x, 0);  // Frame rect changed
    
    // Cleanup
    world.DestroyEntity(entity);
    std::remove(bankPath.c_str());
}




Test 8: AI/BT → Animation Bridge
C++


TEST(Integration, BehaviorTreeAnimationSync)
{
    // Setup: Entity with animation + AI components
    auto& world = World::Get();
    EntityID entity = world.CreateEntity();
    
    // Animation component
    VisualAnimation_data animData;
    animData.bankId = "test";
    animData.currentAnimName = "idle";
    world.AddComponent(entity, animData);
    
    // AI components
    AIBlackboard_data blackboard;
    blackboard.hasTarget = false;
    world.AddComponent(entity, blackboard);
    
    BehaviorTreeRuntime_data btRuntime;
    btRuntime.AITreeAssetId = 1;
    btRuntime.isActive = true;
    world.AddComponent(entity, btRuntime);
    
    // Execute BT action: MoveToGoal
    BTStatus status = ExecuteBTAction(
        BTActionType::MoveToGoal,
        0.0f, 0.0f,
        entity,
        blackboard
    );
    
    // Verify animation parameters updated
    auto& updatedAnimData = world.GetComponent<VisualAnimation_data>(entity);
    
    bool isMoving = AnimationHelpers::GetBoolParameter(updatedAnimData, "isMoving", false);
    ASSERT_TRUE(isMoving);  // ✅ BT set isMoving = true
    
    float speed = AnimationHelpers::GetFloatParameter(updatedAnimData, "speed", 0.0f);
    ASSERT_GT(speed, 0.0f);  // ✅ BT set speed > 0
    
    // Execute BT action: Idle
    status = ExecuteBTAction(
        BTActionType::Idle,
        0.0f, 0.0f,
        entity,
        blackboard
    );
    
    // Verify animation parameters reset
    isMoving = AnimationHelpers::GetBoolParameter(updatedAnimData, "isMoving", false);
    ASSERT_FALSE(isMoving);  // ✅ BT reset isMoving = false
    
    // Cleanup
    world.DestroyEntity(entity);
}




Tests de Performance
Test 9: Memory Leak Check (Valgrind)
bash


# Compile with debug symbols and sanitizers
g++ -std=c++14 -Wall -Wextra -g -fsanitize=address -fsanitize=leak \
    -o animation_editor_test \
    Animation/*.cpp \
    Tests/AnimationTests.cpp \
    -lSDL3 -lSDL3_image

# Run with Valgrind
valgrind --leak-check=full \
         --show-leak-kinds=all \
         --track-origins=yes \
         --verbose \
         ./animation_editor_test

# Expected output:
# ==12345== HEAP SUMMARY:
# ==12345==     in use at exit: 0 bytes in 0 blocks
# ==12345==   total heap usage: 1,234 allocs, 1,234 frees, 567,890 bytes allocated
# ==12345==
# ==12345== All heap blocks were freed -- no leaks are possible




Test 10: Performance (1000 Entities with Animation)
C++


TEST(Performance, ManyEntitiesAnimation)
{
    auto& world = World::Get();
    AnimationSystem animSystem;
    animSystem.Initialize();
    
    // Load bank
    AnimationManager::Get().LoadAnimationBanks("./GameData/Animations/Banks/");
    
    // Create 1000 entities
    std::vector<EntityID> entities;
    for (int i = 0; i < 1000; ++i)
    {
        EntityID entity = world.CreateEntity();
        
        VisualAnimation_data animData;
        animData.bankId = "thesee";
        animData.currentAnimName = "idle";
        animData.isPlaying = true;
        world.AddComponent(entity, animData);
        
        VisualSprite_data spriteData;
        spriteData.sprite = DataManager::Get().GetOrLoadTexture("./test_idle.png");
        world.AddComponent(entity, spriteData);
        
        entities.push_back(entity);
    }
    
    // Benchmark update
    auto startTime = std::chrono::high_resolution_clock::now();
    
    for (int frame = 0; frame < 60; ++frame)
    {
        animSystem.Update(1.0f / 60.0f);
    }
    
    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
    
    std::cout << "[Performance] 1000 entities × 60 frames = " 
              << duration.count() << " ms" << std::endl;
    
    // Should be < 500ms for 60 frames (8.33ms per frame average)
    ASSERT_LT(duration.count(), 500);
    
    // Cleanup
    for (EntityID entity : entities)
    {
        world.DestroyEntity(entity);
    }
}




Tests Utilisateur (Manual QA)
Test 11: UX Workflow (Manual)
Scenario: Nouvel utilisateur crée sa première bank
Steps:

✅ Ouvrir éditeur (F9) → Fenêtre apparaît immédiatement
✅ Cliquer "New Bank" → Dialog clair, champs pré-remplis
✅ Entrer ID "my_char" → Pas d'erreur, validation OK
✅ Cliquer "Add Spritesheet" → File browser s'ouvre
✅ Sélectionner image → Texture preview visible
✅ Cliquer "Auto-Detect Grid" → Colonnes/lignes détectées correctement
✅ Cliquer "Add" → Tab apparaît avec nom spritesheet
✅ Cliquer "New Sequence" → Formulaire vide
✅ Remplir propriétés → Validation temps réel (FPS recalculé)
✅ Cliquer "Apply" → Séquence ajoutée à liste
✅ Cliquer "Play" dans preview → Animation joue smoothly
✅ Ctrl+S pour sauvegarder → Status bar "Saved successfully"
✅ Fermer éditeur (F9) → Pas de prompt (pas de changements non sauvés)
✅ Réouvrir (F9) + charger bank → Toutes données intactes
Critères de Succès:

⏱️ Temps total workflow: < 5 minutes
🎯 Zéro confusion (pas de "je ne sais pas quoi faire")
✅ Zéro erreur inattendue
😊 Feedback positif utilisateur


Test 12: Keyboard Shortcuts (Manual)
Keys to Test:

Shortcut	Action	Expected Result
F9	Toggle editor	Window opens/closes
Ctrl+S	Save bank	Status "Saved", dirty flag cleared
Ctrl+N	New bank	Dialog opens
F5	Refresh scan	Bank list updates
Space	Play/Pause preview	Animation toggles
G	Toggle grid	Grid overlay shows/hides
F	Fit to view	Spritesheet centered + zoomed
R	Reset view	Zoom 100%, centered
Arrow Keys	Navigate frames	Highlighted frame changes
Ctrl+A	Select all frames	All frames green border
Escape	Clear selection	No frames selected
Ctrl+Z	Undo	Last action reverted (Phase 2)
Ctrl+Y	Redo	Action re-applied (Phase 2)

Validation: Tous les raccourcis fonctionnent sans conflit


🚀 ROADMAP PHASE 2
Features Phase 2 (Post-MVP)
1. Animation Graph Editor Visuel (FSM)
Objectif: Créer transitions visuellement (nœuds + liens)
Features:

Node-based UI (ImNodes ou custom canvas)
States = nœuds (rectangles, couleurs par type)
Transitions = liens (arrows, conditions affichées)
Condition Editor (popup pour éditer comparaisons)
Live Preview (highlight nœud actif au runtime)
Timeline: 2-3 semaines


2. Animation Events (Frame-Specific)
Objectif: Trigger events à frame précise (hitbox, sound, VFX)
Features:

Event Track dans timeline (markers)
Event Types:🔊 Sound (play audio file)
💥 VFX (spawn particle effect)
⚔️ Hitbox (enable collision rect)
🎮 Gameplay (call custom function)
Event Editor (popup properties par event)
Preview Events (visualiser hitbox, play sound)
Timeline: 1-2 semaines


3. Bridge AI → Animation Automatique
Objectif: Auto-set parameters depuis BT actions
Features:

Mapping Config (JSON: BTAction → AnimParam)
Example:JSON


{
  "BTActionType::MoveToGoal": {
    "setParameters": [
      { "name": "isMoving", "value": true },
      { "name": "speed", "source": "blackboard.currentSpeed" }
    ]
  }
}



Auto-apply sans code manuel dans BT
Debug Visualizer (afficher parameters live)
Timeline: 1 semaine


4. Hot-Reload Banks (Sans Restart)
Objectif: Modifier bank JSON → reload automatique runtime
Features:

File Watcher (detect changes GameData/Animations/)
Reload Texture (destroy + reload sans crash)
Refresh Entities (update tous components avec cette bank)
Status Notification (toast: "Bank reloaded successfully")
Timeline: 1 semaine


5. Import/Export Formats Externes
Objectif: Compatibilité avec outils tiers
Formats:

Aseprite (.ase → AnimationBank)
Spine (.json → AnimationBank)
GIF (export séquence → GIF animé)
Spritesheet PNG (export bank → combined spritesheet)
Timeline: 2 semaines (par format)


6. Multi-Animation Preview
Objectif: Comparer plusieurs animations côte à côte
Features:

Split View (2-4 previews simultanés)
Sync Playback (toutes animations synchronisées)
Diff Highlight (afficher différences entre animations)
Timeline: 1 semaine


7. Animation Blending (Smooth Transitions)
Objectif: Transition smooth entre animations (fade)
Features:

Blend Duration (0.1s à 1s transition)
Blend Curve (linear, ease-in, ease-out)
Preview Blend (visualiser transition)
Runtime Support (AnimationSystem blend logic)
Timeline: 2 semaines


📚 ANNEXES
A. Conventions de Nommage
Files:

Banks: {entity_name}_animations.json (ex: thesee_animations.json)
Graphs: {entity_name}_animgraph.json (ex: thesee_animgraph.json)
Spritesheets: {entity}_{animation}.png (ex: thesee_idle.png)
IDs:

Bank ID: {entity_name} (lowercase, underscore) (ex: thesee, zombie_knight)
Spritesheet ID: {bank_id}_{animation} (ex: thesee_idle)
Sequence Name: {animation} (lowercase) (ex: idle, walk, special_attack)
Components:

Struct name: {Name}_data (ex: VisualAnimation_data)
Member vars: camelCase (ex: bankId, currentAnimName)
Helpers:

Namespace: Olympe::AnimationHelpers
Functions: PascalCase (ex: SetParameter, GetFloatParameter)


B. Formats JSON (Quick Reference)
Animation Bank (Schema v2)
JSON


{
  "schema_version": 2,
  "type": "AnimationBank",
  "bankId": "string",
  "description": "string",
  "metadata": {
    "author": "string",
    "created": "ISO8601",
    "lastModified": "ISO8601",
    "tags": ["string"]
  },
  "spritesheets": [
    {
      "id": "string",
      "path": "string",
      "description": "string",
      "frameWidth": int,
      "frameHeight": int,
      "columns": int,
      "rows": int,
      "totalFrames": int,
      "spacing": int,
      "margin": int,
      "hotspot": { "x": float, "y": float }
    }
  ],
  "sequences": [
    {
      "name": "string",
      "spritesheetId": "string",
      "frames": { "start": int, "count": int },
      "frameDuration": float,
      "loop": bool,
      "speed": float,
      "nextAnimation": "string",
      "events": [
        {
          "frame": int,
          "type": "sound|hitbox|vfx|gamelogic",
          "data": object
        }
      ]
    }
  ]
}




Entity Prefab (with Animation)
JSON


{
  "schema_version": 2,
  "type": "Character",
  "name": "string",
  "components": [
    {
      "type": "VisualAnimation_data",
      "properties": {
        "bankId": "string",
        "currentAnimName": "string",
        "animGraphPath": "string",
        "playbackSpeed": float,
        "isPlaying": bool,
        "loop": bool
      }
    }
  ]
}




C. Architecture ECS Recap
Code


┌─────────────────────────────────────────────────────────────┐
│                    ARCHITECTURE ECS                          │
├─────────────────────────────────────────────────────────────┤
│                                                              │
│ COMPONENTS (Pure Data - NO METHODS)                         │
│ ├─ VisualAnimation_data                                     │
│ │   ├─ bankId, currentAnimName, animGraphPath              │
│ │   ├─ playbackSpeed, isPlaying, loop                      │
│ │   ├─ currentFrameIndex, frameTimer                        │
│ │   └─ floatParams, boolParams, intParams (FSM)            │
│ │                                                            │
│ HELPERS (Utility Functions)                                 │
│ ├─ AnimationHelpers::SetParameter(...)                     │
│ ├─ AnimationHelpers::GetFloatParameter(...)                │
│ └─ AnimationHelpers::GetBoolParameter(...)                 │
│                                                              │
│ SYSTEMS (Game Logic)                                        │
│ ├─ AnimationSystem::Update(deltaTime)                      │
│ │   ├─ Update frame timers                                  │
│ │   ├─ Evaluate FSM transitions                            │
│ │   └─ Update VisualSprite_data.srcRect                    │
│ ├─ BehaviorTreeSystem::ExecuteAction(...)                  │
│ │   └─ Set FSM parameters via helpers                      │
│ └─ PlayerControllerSystem::Update(...)                     │
│     └─ Set FSM parameters based on input                   │
│                                                              │
│ MANAGERS (Asset Management)                                 │
│ ├─ AnimationManager::LoadAnimationBanks(...)               │
│ │   ├─ Parse JSON → AnimationBank                          │
│ │   └─ Cache in std::unordered_map<bankId, bank>           │
│ └─ DataManager::GetOrLoadTexture(...)                      │
│     ├─ Check cache                                          │
│     ├─ Load with SDL_IMG                                    │
│     └─ Return SDL_Texture*                                  │
│                                                              │
└─────────────────────────────────────────────────────────────┘




D. Checklist Validation C++14
Avant chaque commit:

 Aucune fonctionnalité C++17/20 utilisée Pas de std::optional, std::variant, std::string_view
 Pas de structured bindings (auto& [k, v])
 Pas de if constexpr
 Pas de fold expressions
 Tout le code dans namespace Olympe { } Namespace fermé avec commentaire // namespace Olympe
 Aucun code orphelin hors namespace
 Accès JSON via helpers json_get_int(), json_get_float(), json_get_string(), json_get_bool()
 json::array() SANS arguments
 Itération avec .begin()/.end() et it.key()/it.value()
 Components = pure data VisualAnimation_data sans méthodes (sauf constructeurs)
 Helpers dans AnimationHelpers namespace
 Toutes structures initialisées Valeurs par défaut dans déclaration
 = default constructors
 Plateforme-safe localtime_s() avec #ifdef _WIN32
 Casts explicites pour conversions
 Pointeurs vérifiés avant utilisation
 Compilation mentale Code relu ligne par ligne
 Signatures API tierces vérifiées (ImGui, SDL3)
 0 erreurs prévisibles, 0 warnings prévisibles


E. Ressources et Références
Documentation:

SDL3 Documentation
ImGui Documentation
nlohmann/json Documentation
C++14 Standard
Olympe Engine:

COPILOT_CODING_RULES.md (C++14 strict guidelines)
GameData/Animations/README.md (asset organization)
Source/Animation/AnimationTypes.h (data structures)
Source/DataManager.h (asset loading API)
Contact:

Author: Atlasbruce
Email: nchereau@gmail.com <nchereau@gmail.com>
GitHub: https://github.com/Atlasbruce/Olympe-Engine


✅ CONCLUSION
Ce brief détaille intégralement l'implémentation de l'Animation Editor v1 pour Olympe Engine, incluant:

✅ Architecture complète (diagrammes, flux de données, ECS)
✅ Structures de données (pure data, respect ECS)
✅ Format JSON unifié (v2 + rétrocompatibilité v1)
✅ Interface utilisateur (3-panels, tous contrôles détaillés)
✅ Implémentation technique (classes, extensions DataManager)
✅ Intégration système (GameEngine, AnimationManager, AnimationSystem)
✅ Workflow utilisateur (scenarios step-by-step)
✅ Plan d'implémentation (10 phases, 5-6 semaines)
✅ Tests complets (unitaires, intégration, performance, UX)
✅ Roadmap Phase 2 (FSM editor, events, hot-reload, import/export)
Ce document peut être copié tel quel dans une nouvelle conversation pour implémenter l'Animation Editor sans rien oublier.

