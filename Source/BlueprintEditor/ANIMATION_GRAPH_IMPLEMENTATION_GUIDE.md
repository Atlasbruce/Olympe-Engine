# Plan Détaillé: Implémentation du Type de Graphe "Animation"
**Date**: 2026-04-16 (Phase 52+)  
**Objet**: Guide pas-à-pas pour ajouter Animation Graph Type au framework  
**Audience**: Dev team prêt à commencer implémentation

---

## 1. OVERVIEW - Animation Graph System Architecture

### 1.1 Vision Globale

L'Animation Graph sera un système de node graph temporel permettant:
- **Keyframe Animation**: Édition d'animations avec keyframes sur timeline
- **Track-Based**: Multiple tracks (Position, Rotation, Scale, custom properties)
- **Visual Timeline**: Scrubber + playback dans l'éditeur
- **Document Model**: Sauvegarde en `.anim.json`, intégration framework complet

### 1.2 Position dans Framework

```
Framework Tier 2                 Framework Tier 3              Framework Tier 4
─────────────────────────        ──────────────────────       ──────────────────────
AnimationGraphDocument     ──→   AnimationGraphRenderer  ──→   TimelineCanvasEditor
(IGraphDocument impl)            (IGraphRenderer impl)         (ICanvasEditor impl)
├─ Load/Save .anim.json         ├─ Render()                  ├─ Pan/Zoom timeline
├─ AnimationTimelineData         ├─ Load/Save frames         ├─ Scrubber handle
├─ Dirty flag tracking           ├─ Canvas state save/restore ├─ Keyframe selection
└─ Lifecycle methods             └─ Framework modals          └─ Time grid snapping
```

---

## 2. DATA MODEL - AnimationTimelineData Structure

### 2.1 Classe Principale: AnimationTimelineData

```cpp
// File: Source/BlueprintEditor/AnimationEditor/AnimationTimelineData.h

namespace Olympe {

// ============================================================================
// Data Types
// ============================================================================

struct Keyframe {
    float time;                    // Seconds (0.0 - duration)
    float value;                   // Property value at this time
    std::string interpolation;     // "Linear", "Bezier", "Step"
    float tangentIn = 0.0f;       // For Bezier curves
    float tangentOut = 0.0f;      // For Bezier curves
};

struct AnimationTrack {
    int trackId;                   // Unique within document
    std::string name;              // e.g., "Position.X", "Rotation.Z"
    std::string type;              // "Float", "Vector3", "Color", "Bool"
    std::vector<Keyframe> keyframes;
    bool enabled = true;
    float minValue = -1000.0f;    // Value range for UI normalization
    float maxValue = 1000.0f;
    
    // Helpers
    bool IsKeyframeAtTime(float time) const;
    int FindKeyframeIndex(float time) const;
    float EvaluateAtTime(float time) const;
};

struct AnimationCanvasState {
    float timelineZoom = 1.0f;           // Horizontal zoom (time axis)
    float trackVerticalScroll = 0.0f;    // Vertical scroll offset
    float currentPlaybackTime = 0.0f;    // Playback position
    bool isPlaying = false;
};

// ============================================================================
// Main Document Data
// ============================================================================

class AnimationTimelineData {
public:
    // Metadata
    std::string name;                      // Animation name
    float duration = 1.0f;                // Total duration in seconds
    int frameRate = 30;                   // For keyframe snapping
    
    // Tracks
    std::vector<AnimationTrack> tracks;
    int nextTrackId = 1;
    
    // Canvas state
    AnimationCanvasState canvasState;
    
    // Dirty tracking
    bool isDirty = false;
    
    // API
    AnimationTrack* CreateTrack(const std::string& name, const std::string& type);
    bool RemoveTrack(int trackId);
    AnimationTrack* GetTrack(int trackId);
    const AnimationTrack* GetTrack(int trackId) const;
    
    Keyframe* CreateKeyframe(int trackId, float time, float value);
    bool RemoveKeyframe(int trackId, int keyframeIndex);
    void DeleteAllKeyframesInTimeRange(float timeStart, float timeEnd);
    
    // Evaluation
    float EvaluateTrack(int trackId, float time) const;
    
    // Serialization
    json ToJson() const;
    static bool FromJson(const json& data, AnimationTimelineData& outData);
    
    // Utilities
    float GetFPS() const { return static_cast<float>(frameRate); }
    float GetFrameTime() const { return 1.0f / GetFPS(); }
    int GetTotalFrames() const { return static_cast<int>(duration * frameRate); }
    int TimeToFrame(float time) const { return static_cast<int>(time * frameRate); }
    float FrameToTime(int frame) const { return frame / GetFPS(); }
};

} // namespace Olympe
```

### 2.2 JSON Schema v1

```json
{
  "version": 1,
  "animationType": "Animation",
  "metadata": {
    "name": "run_cycle",
    "duration": 1.0,
    "frameRate": 30,
    "lastModified": "2026-04-16T10:30:45Z"
  },
  "tracks": [
    {
      "trackId": 1,
      "name": "Position.X",
      "type": "Float",
      "enabled": true,
      "minValue": -1000.0,
      "maxValue": 1000.0,
      "keyframes": [
        {
          "time": 0.0,
          "value": 0.0,
          "interpolation": "Linear",
          "tangentIn": 0.0,
          "tangentOut": 0.0
        },
        {
          "time": 0.5,
          "value": 50.0,
          "interpolation": "Linear",
          "tangentIn": 100.0,
          "tangentOut": 100.0
        },
        {
          "time": 1.0,
          "value": 0.0,
          "interpolation": "Linear",
          "tangentIn": -100.0,
          "tangentOut": -100.0
        }
      ]
    },
    {
      "trackId": 2,
      "name": "Rotation.Z",
      "type": "Float",
      "enabled": true,
      "minValue": -360.0,
      "maxValue": 360.0,
      "keyframes": [
        { "time": 0.0, "value": 0.0, "interpolation": "Linear" },
        { "time": 1.0, "value": 360.0, "interpolation": "Linear" }
      ]
    }
  ],
  "canvasState": {
    "timelineZoom": 1.0,
    "trackVerticalScroll": 0.0,
    "currentPlaybackTime": 0.0,
    "isPlaying": false
  }
}
```

---

## 3. DOCUMENT LAYER - AnimationGraphDocument

### 3.1 Classe: AnimationGraphDocument (Direct Implementation)

```cpp
// File: Source/BlueprintEditor/AnimationEditor/AnimationGraphDocument.h

class AnimationGraphDocument : public IGraphDocument {
public:
    AnimationGraphDocument();
    ~AnimationGraphDocument() override;
    
    // ========================================================================
    // IGraphDocument Interface (Required Methods)
    // ========================================================================
    
    bool Load(const std::string& filePath) override;
    bool Save(const std::string& filePath) override;
    bool IsDirty() const override;
    
    std::string GetName() const override;
    DocumentType GetType() const override;
    std::string GetFilePath() const override;
    void SetFilePath(const std::string& path) override;
    
    IGraphRenderer* GetRenderer() override;
    const IGraphRenderer* GetRenderer() const override;
    
    void OnDocumentModified() override;
    
    // Canvas state persistence (Phase 35)
    void SaveCanvasState() override;
    void RestoreCanvasState() override;
    std::string GetCanvasStateJSON() const override;
    void SetCanvasStateJSON(const std::string& json) override;
    
    // ========================================================================
    // Animation-Specific API
    // ========================================================================
    
    // Data access
    AnimationTimelineData& GetData() { return m_timelineData; }
    const AnimationTimelineData& GetData() const { return m_timelineData; }
    
    // Track management (delegates to m_timelineData)
    AnimationTrack* CreateTrack(const std::string& name, const std::string& type);
    bool RemoveTrack(int trackId);
    
    // Keyframe management
    Keyframe* CreateKeyframe(int trackId, float time, float value);
    bool RemoveKeyframe(int trackId, int keyframeIndex);
    
    // Playback
    float GetCurrentPlaybackTime() const;
    void SetCurrentPlaybackTime(float time);
    bool IsPlaying() const;
    void SetPlaying(bool playing);
    
private:
    AnimationTimelineData m_timelineData;
    std::unique_ptr<AnimationGraphRenderer> m_renderer;
    std::string m_filePath;
    
    // Canvas state backup for tab switching
    std::string m_savedCanvasStateJSON;
};
```

### 3.2 Implémentation: AnimationGraphDocument.cpp

```cpp
// Snippets clés

bool AnimationGraphDocument::Load(const std::string& filePath) {
    // 1. Charger JSON du fichier
    // 2. Valider schema (version check)
    // 3. Deserializer via AnimationTimelineData::FromJson()
    // 4. Créer renderer
    // 5. m_isDirty = false
    // 6. m_filePath = filePath
    // 7. return true/false
}

bool AnimationGraphDocument::Save(const std::string& filePath) {
    // 1. Mettre à jour m_filePath
    // 2. Serialize via m_timelineData.ToJson()
    // 3. Écrire JSON au fichier
    // 4. m_isDirty = false
    // 5. return true/false
}

void AnimationGraphDocument::OnDocumentModified() {
    m_isDirty = true;
    // Optionnel: m_renderer->NotifyDataChanged()
}

IGraphRenderer* AnimationGraphDocument::GetRenderer() {
    if (!m_renderer) {
        m_renderer = std::make_unique<AnimationGraphRenderer>(this);
    }
    return m_renderer.get();
}
```

---

## 4. RENDERER LAYER - AnimationGraphRenderer

### 4.1 Classe: AnimationGraphRenderer

```cpp
// File: Source/BlueprintEditor/AnimationEditor/AnimationGraphRenderer.h

class AnimationGraphRenderer : public IGraphRenderer {
public:
    explicit AnimationGraphRenderer(AnimationGraphDocument* document);
    ~AnimationGraphRenderer() override;
    
    // ========================================================================
    // IGraphRenderer Interface (Required Methods)
    // ========================================================================
    
    void Render() override;
    bool Load(const std::string& path) override;
    bool Save(const std::string& path) override;
    bool IsDirty() const override;
    std::string GetGraphType() const override;
    std::string GetCurrentPath() const override;
    
    // Canvas state management (Phase 35)
    void SaveCanvasState() override;
    void RestoreCanvasState() override;
    
    // Framework modals (Phase 43-45)
    void RenderFrameworkModals() override;
    
    // ========================================================================
    // Animation-Specific Rendering
    // ========================================================================
    
    void RenderToolbar();
    void RenderTimelineHeader();
    void RenderTracks();
    void RenderPlaybackControls();
    void RenderKeyframeEditor();
    void RenderContextMenu();
    
private:
    AnimationGraphDocument* m_document;  // Non-owning reference
    std::unique_ptr<TimelineCanvasEditor> m_canvas;
    std::unique_ptr<KeyframeTrackRenderer> m_trackRenderer;
    std::unique_ptr<TimelinePlaybackController> m_playback;
    
    // Save/SaveAs modals (Phase 45)
    bool m_showSaveAsModal = false;
    std::string m_saveAsPath;
    
    // Selection state
    int m_selectedTrackId = -1;
    int m_selectedKeyframeIndex = -1;
};
```

### 4.2 Rendering Strategy

**Layout du Renderer:**

```
┌─────────────────────────────────────────────────────┐
│ Toolbar (File, Edit, View buttons)                  │
├─────────────────────────────────────────────────────┤
│ Timeline Header (ruler, time labels)                │
├───────────────┬─────────────────────────────────────┤
│ Track List    │ Timeline Canvas (keyframes visual)  │
│ (scrollable)  │ ├─ Grid background                  │
│               │ ├─ Playback scrubber (vertical)     │
│ Track 1 ──────┼─●─●────●──────                     │
│ Track 2 ──────┼───●──●──●───────                   │
│ [+]           │                                     │
│ [-]           │ (drag to pan, scroll to zoom)      │
│               │                                     │
└───────────────┴─────────────────────────────────────┘
│ Playback Controls (Play/Pause, Timeline display)   │
└─────────────────────────────────────────────────────┘
```

---

## 5. CANVAS LAYER - TimelineCanvasEditor

### 5.1 Classe: TimelineCanvasEditor (derives CustomCanvasEditor)

```cpp
// File: Source/BlueprintEditor/AnimationEditor/TimelineCanvasEditor.h

class TimelineCanvasEditor : public ICanvasEditor {
public:
    TimelineCanvasEditor();
    ~TimelineCanvasEditor() override;
    
    // ========================================================================
    // ICanvasEditor Interface (Core Implementation)
    // ========================================================================
    
    void BeginRender() override;
    void EndRender() override;
    
    // Pan/Zoom (timeline-specific)
    ImVec2 GetCanvasOffset() const override;
    void SetCanvasOffset(const ImVec2& offset) override;
    float GetCanvasZoom() const override;
    void SetCanvasZoom(float zoom) override;
    
    // Coordinate transformations
    ImVec2 ScreenToCanvas(const ImVec2& screenPos) const override;
    ImVec2 CanvasToScreen(const ImVec2& canvasPos) const override;
    
    // ========================================================================
    // Timeline-Specific Features
    // ========================================================================
    
    // Time coordinate system
    float ScreenToTime(float screenX) const;     // Pixels → Seconds
    float TimeToScreen(float time) const;        // Seconds → Pixels
    float GetPixelsPerSecond() const;            // timelineZoom * basePixelsPerSec
    
    // Scrubber (playback position indicator)
    float GetScrubberTime() const;
    void SetScrubberTime(float time);
    bool IsScrubberDragging() const;
    
    // Keyframe interaction
    int GetHoveredKeyframeTrackIndex() const;
    int GetHoveredKeyframeIndex() const;
    int GetSelectedKeyframeTrackIndex() const;
    int GetSelectedKeyframeIndex() const;
    
    // Snapping
    float SnapTimeToGrid(float time, float snapAmount = 0.0f) const;
    void SetTimeGridSize(float timeInSeconds);  // Grid spacing
    
    // Time range visibility
    float GetVisibleTimeStart() const;
    float GetVisibleTimeEnd() const;
    void FocusTimeRange(float timeStart, float timeEnd);
    
    // Track vertical scrolling
    float GetTrackScrollOffset() const;
    void SetTrackScrollOffset(float offset);
    
    // Rendering
    void RenderGrid(float maxTime, const CanvasGridRenderer::Style& style);
    void RenderScrubber(float time, ImU32 color = 0xFFFFFFFF);
    void RenderTimeRuler(float maxTime, float height);
    
    // Configuration
    void SetFrameRate(int fps);
    void SetDuration(float duration);
    
private:
    // Pan/Zoom state (per-axis different meaning than EntityPrefab)
    ImVec2 m_canvasOffset;         // (x=time offset, y=track offset)
    float m_timelineZoom = 1.0f;  // Horizontal zoom (time axis)
    
    // Scrubber state
    float m_currentTime = 0.0f;
    bool m_isScrubberDragging = false;
    
    // Keyframe selection
    int m_hoveredTrackIndex = -1;
    int m_hoveredKeyframeIndex = -1;
    int m_selectedTrackIndex = -1;
    int m_selectedKeyframeIndex = -1;
    
    // Grid/Snapping
    float m_timeGridSize = 0.1f;   // 100ms grid by default
    float m_basePixelsPerSecond = 100.0f;  // Base scale before zoom
    
    // Playback
    int m_frameRate = 30;
    float m_duration = 1.0f;
    
    // ImGui tracking
    ImVec2 m_canvasScreenPos;
};
```

### 5.2 Coordinate System pour Timeline

```
Screen Space (Pixels)
    │
    │ Horizontal: pixel position in timeline window
    │ Vertical: pixel position in track list
    │
    ├─ ScreenToTime() → time in seconds
    ├─ TimeToScreen() → pixel x position
    │
    ▼
Canvas Space (Time + Track)
    │
    │ Horizontal: time in seconds (0 to duration)
    │ Vertical: track index (0 to num_tracks)
    │
    │ Formulas:
    │   time = (screenX - canvasX - m_canvasOffset.x) / (m_basePixelsPerSecond * m_timelineZoom)
    │   screenX = time * (m_basePixelsPerSecond * m_timelineZoom) + m_canvasOffset.x + canvasX
    │
    ▼
Grid/Save Space
    │
    │ Horizontal: time (persistent storage)
    │ Vertical: track index (persistent storage)
    │
```

---

## 6. UTILITY RENDERERS

### 6.1 KeyframeTrackRenderer

```cpp
// File: Source/BlueprintEditor/AnimationEditor/KeyframeTrackRenderer.h

class KeyframeTrackRenderer {
public:
    struct KeyframeVisuals {
        ImVec2 position;       // Screen position
        ImU32 color;           // Default or selected
        bool isSelected;
        bool isHovered;
    };
    
    void RenderTrack(
        const AnimationTrack& track,
        float trackScreenY,
        float trackHeight,
        const TimelineCanvasEditor& canvas,
        float duration
    );
    
    void RenderKeyframe(
        const Keyframe& keyframe,
        float trackScreenY,
        const TimelineCanvasEditor& canvas,
        int trackIndex,
        int keyframeIndex,
        bool isSelected,
        bool isHovered
    );
    
    void RenderKeyframeCurve(
        const AnimationTrack& track,
        float trackScreenY,
        const TimelineCanvasEditor& canvas,
        ImU32 curveColor = 0xFF00FF00
    );
    
    // Hit detection
    bool IsPointInKeyframe(
        const ImVec2& point,
        const ImVec2& keyframePos,
        float keyframeRadius = 6.0f
    ) const;
};
```

### 6.2 TimelinePlaybackController

```cpp
// File: Source/BlueprintEditor/AnimationEditor/TimelinePlaybackController.h

class TimelinePlaybackController {
public:
    void Update(float deltaTime);
    
    void Play();
    void Pause();
    void Stop();
    void SetTime(float time);
    
    float GetCurrentTime() const;
    bool IsPlaying() const;
    
    void SetDuration(float duration);
    void SetLooping(bool looping);
    bool IsLooping() const;
    
private:
    float m_currentTime = 0.0f;
    float m_duration = 1.0f;
    bool m_isPlaying = false;
    bool m_isLooping = true;
};
```

---

## 7. INTEGRATION FRAMEWORK - TabManager Modifications

### 7.1 Changes à TabManager.cpp

```cpp
// In TabManager::DetectGraphType()

GraphType TabManager::DetectGraphType(const std::string& filePath)
{
    // ... existing checks for VisualScript, BehaviorTree, EntityPrefab ...
    
    // NEW: Animation detection
    if (filePath.find(".anim.json") != std::string::npos)
    {
        // Optional: verify JSON content has "timelineData" or "tracks" field
        return GraphType::ANIMATION;
    }
    
    return GraphType::UNKNOWN;
}

// In TabManager::OpenFileInTab()

std::string TabManager::OpenFileInTab(const std::string& filePath)
{
    // ... existing logic ...
    
    GraphType graphType = DetectGraphType(filePath);
    std::unique_ptr<IGraphDocument> document;
    
    switch (graphType)
    {
        case GraphType::VISUAL_SCRIPT:
            // ... existing ...
            break;
            
        case GraphType::BEHAVIOR_TREE:
            // ... existing ...
            break;
            
        case GraphType::ENTITY_PREFAB:
            // ... existing ...
            break;
            
        case GraphType::ANIMATION:  // NEW
        {
            auto animDoc = std::make_unique<AnimationGraphDocument>();
            if (!animDoc->Load(filePath))
            {
                SYSTEM_LOG << "[TabManager] Failed to load Animation: " << filePath << "\n";
                return "";
            }
            document = std::move(animDoc);
            break;
        }
        
        default:
            SYSTEM_LOG << "[TabManager] Unknown graph type for: " << filePath << "\n";
            return "";
    }
    
    // ... rest of existing logic ...
}
```

### 7.2 Changes à GraphType Enum

```cpp
// File: Source/BlueprintEditor/TabManager.h

enum class GraphType {
    UNKNOWN,
    VISUAL_SCRIPT,
    BEHAVIOR_TREE,
    ENTITY_PREFAB,
    ANIMATION                  // NEW
};
```

### 7.3 Changes à IGraphDocument.h

```cpp
enum class DocumentType {
    VISUAL_SCRIPT,
    BEHAVIOR_TREE,
    ENTITY_PREFAB,
    ANIMATION,                 // NEW
    UNKNOWN
};
```

### 7.4 Changes à BlueprintEditorGUI.cpp

```cpp
// In BlueprintEditorGUI::RenderFileMenu()

void BlueprintEditorGUI::RenderFileMenu()
{
    if (ImGui::BeginMenu("File"))
    {
        // Existing items...
        
        // NEW: Animation menu items
        if (ImGui::MenuItem("New Animation", "Ctrl+Alt+M"))
        {
            CreateNewAnimation();
        }
        
        ImGui::Separator();
        
        // ... rest of menu ...
    }
}

void BlueprintEditorGUI::CreateNewAnimation()
{
    // 1. Créer AnimationGraphDocument vide
    // 2. Ajouter tab via TabManager
    // 3. Optionnel: ouvrir "Save As" dialog
}
```

---

## 8. PHASE-BY-PHASE IMPLEMENTATION

### Phase 1: Foundation (Data Model)

**Objectif**: Serialization/Deserialization working

**Files à créer**:
- `AnimationTimelineData.h/cpp`
- `AnimationLoader.h/cpp` (JSON loading utility)
- `AnimationSerializer.h/cpp` (JSON saving utility)

**Testing**:
```cpp
// Test: Round-trip JSON
json original = { /* sample animation */ };
AnimationTimelineData data1;
AnimationTimelineData::FromJson(original, data1);
json serialized = data1.ToJson();
// Verify: original ≈ serialized
```

**Done Criteria**:
- [ ] All 3 files compile (0 errors)
- [ ] JSON schema v1 validated
- [ ] Round-trip test passes
- [ ] Handles 10+ keyframes on 5+ tracks without issues

### Phase 2: Document Layer

**Objectif**: IGraphDocument interface complete

**Files à créer**:
- `AnimationGraphDocument.h/cpp`

**Done Criteria**:
- [ ] Implements 100% IGraphDocument methods
- [ ] Load(filepath) works
- [ ] Save(filepath) works
- [ ] Dirty tracking works
- [ ] Canvas state save/restore works

### Phase 3: Renderer + Canvas

**Objectif**: Visual rendering working

**Files à créer**:
- `AnimationGraphRenderer.h/cpp`
- `TimelineCanvasEditor.h/cpp`
- `KeyframeTrackRenderer.h/cpp`
- `TimelinePlaybackController.h/cpp`

**Done Criteria**:
- [ ] Render() displays timeline with tracks
- [ ] Keyframes visible on timeline
- [ ] Pan/zoom working
- [ ] Playback scrubber visible
- [ ] No lag with 50+ keyframes

### Phase 4: Integration

**Objectif**: Full framework integration

**Files à modifier**:
- `TabManager.cpp/h` (+GraphType::ANIMATION case)
- `BlueprintEditorGUI.cpp` (+File→New Animation)
- `IGraphDocument.h` (+ANIMATION enum)

**Done Criteria**:
- [ ] File→New Animation works
- [ ] Double-click .anim.json opens in tab
- [ ] Save/SaveAs buttons work
- [ ] Tab switching preserves canvas state
- [ ] All modal flow works

### Phase 5+: Features (Scope Optional)

- Playback preview in editor
- Keyframe editing UI
- Track management UI
- Undo/Redo
- Copy/Paste keyframes
- Curve interpolation editor

---

## 9. HEADER STRUCTURE TEMPLATE

Tous les fichiers doivent suivre ce structure:

```cpp
/**
 * @file ClassName.h
 * @brief Description d'une ligne
 * @author Olympe Engine
 * @date 2026-04-16
 * 
 * @details
 * Detailed description of this class/file purpose.
 * 
 * Key features:
 * - Feature 1
 * - Feature 2
 * 
 * Typical usage:
 * @code
 *   // Example code
 * @endcode
 * 
 * @note Phase XX implementation note (if applicable)
 * 
 * C++14 Compatibility: No C++17+ features (no structured bindings, std::filesystem, etc.)
 */

#pragma once

#include <string>
#include <vector>
// ... other includes ...

namespace Olympe {

class ClassName {
    // ... implementation ...
};

} // namespace Olympe
```

---

## 10. TESTING CHECKLIST

### Unit Tests (Phase 1-2)

```
AnimationTimelineData:
  ✓ Create track
  ✓ Remove track
  ✓ Create keyframe
  ✓ Remove keyframe
  ✓ Evaluate at various times
  ✓ JSON serialization round-trip
  
AnimationGraphDocument:
  ✓ Load valid .anim.json
  ✓ Load invalid file (error handling)
  ✓ Save to file
  ✓ Dirty flag tracking
  ✓ IGraphDocument interface compliance
```

### Integration Tests (Phase 3-4)

```
TabManager:
  ✓ DetectGraphType recognizes .anim.json
  ✓ OpenFileInTab creates AnimationGraphDocument
  ✓ Tab appears and renders
  
Rendering:
  ✓ Tracks render without gaps
  ✓ Keyframes visible at correct positions
  ✓ Pan doesn't cause rendering errors
  ✓ Zoom in/out smooth
  
Canvas State:
  ✓ Save canvas state on tab switch
  ✓ Restore canvas state on tab switch
  ✓ Playback time preserved
```

### E2E Tests (Phase 4+)

```
User Workflow 1: Create + Save
  ✓ File → New Animation
  ✓ Add 2 tracks
  ✓ Add 5 keyframes
  ✓ File → Save (pick location)
  ✓ File → Close
  ✓ File → Open (same file)
  ✓ Data intact

User Workflow 2: Edit + Playback
  ✓ Open animation file
  ✓ Pan timeline left/right
  ✓ Zoom in on keyframes
  ✓ Click Play button
  ✓ Scrubber moves
  ✓ Playback stops at duration end

User Workflow 3: Tab Switching
  ✓ Open animation in tab 1
  ✓ Pan/zoom to specific area
  ✓ Open another graph in tab 2
  ✓ Click back to tab 1
  ✓ Canvas state restored (same pan/zoom)
```

---

## 11. ERROR HANDLING STRATEGY

### Errors à Gérer

| Error | Where | Action |
|-------|-------|--------|
| Invalid JSON | Load() | Log + return false |
| Missing required fields | Deserializer | Fill defaults + warn |
| File not found | Load() | Log path + return false |
| Disk write failed | Save() | Log system error + return false |
| Duration ≤ 0 | FromJson() | Set default 1.0f |
| Negative time | CreateKeyframe() | Clamp to 0.0f + warn |
| Out-of-memory | Create track | Log + return nullptr |

### Logging Pattern (Phase 44.4 Discipline)

```cpp
// ✅ GOOD: Log on actions (not in render loop)
void AnimationGraphRenderer::OnPlayClicked() {
    SYSTEM_LOG << "[AnimationGraphRenderer] Playback started\n";
    m_playback->Play();
}

// ❌ BAD: Log in render loop
void AnimationGraphRenderer::RenderPlaybackControls() {
    // NO LOGS HERE! Called 60 times/sec
    if (ImGui::Button("Play")) { ... }
}
```

---

## 12. PERFORMANCE CONSIDERATIONS

### Canvas Rendering Optimization

For animations with 1000+ keyframes:

```cpp
// 1. Culling: Don't render keyframes outside visible range
bool IsTimeInVisibleRange(float time, float visibleStart, float visibleEnd) {
    return time >= visibleStart && time <= visibleEnd;
}

// 2. LOD: Merge close keyframes at low zoom levels
void ReduceKeyframesForDisplay(const AnimationTrack& track, 
                               float pixelsPerSecond,
                               std::vector<Keyframe>& outReduced) {
    // If 2 keyframes < 5 pixels apart, show only one
}

// 3. Batching: Render multiple keyframes with single DrawList call
```

---

## 13. RELEASE NOTES TEMPLATE

```markdown
## Animation Graph Editor - v1.0

### New Features
- Timeline-based animation editor
- Multi-track support (Position, Rotation, Scale, custom properties)
- Keyframe manipulation on visual timeline
- Playback preview with scrubber
- Save/Load .anim.json format

### UI
- File → New Animation (Ctrl+Alt+M)
- Double-click .anim.json to open in tab
- Toolbar with Play/Pause, Save buttons
- Timeline ruler with grid snapping
- Track list with add/remove buttons

### Known Limitations
- No undo/redo (Phase 5+ feature)
- Linear interpolation only (Bezier curves Phase 5+)
- Preview limited to viewport (no real-time evaluation)

### Files Changed
- TabManager.cpp/h (+GraphType::ANIMATION)
- BlueprintEditorGUI.cpp (+menu entry)
- IGraphDocument.h (+DocumentType::ANIMATION)
- Created: AnimationEditor/ folder with 8 files
```

---

## 14. NEXT ACTIONS

### Before Implementation Starts

1. **Team Review** of this document
2. **Validate JSON schema** with stakeholders
3. **Confirm performance targets** (max keyframes, tracks)
4. **Assign review buddies** for PRs

### Week 1: Phase 1 (Data Model)

- [ ] Create 3 files (TimelineData, Loader, Serializer)
- [ ] Write unit tests
- [ ] Get first code review
- [ ] Build success: 0 errors

### Week 2: Phase 2 (Document)

- [ ] Create 1 file (AnimationGraphDocument)
- [ ] Implement all IGraphDocument methods
- [ ] Integration tests with TabManager
- [ ] Get framework integration review

### Week 3: Phase 3 (Renderer)

- [ ] Create 4 files (Renderer, Canvas, Track, Playback)
- [ ] Visual testing (render tracks, keyframes)
- [ ] Performance testing (50+ keyframes)
- [ ] UI review

### Week 4: Phase 4 (Integration)

- [ ] Modify 3 existing files
- [ ] End-to-end workflow testing
- [ ] Canvas state persistence testing
- [ ] Final code review

---

**Status**: 📋 **SPECIFICATION COMPLETE - Ready for Implementation**  
**Next Step**: Team Review + Architecture Discussion  
**Estimated LOC**: 1500-2000 lines  
**Estimated Effort**: 4 weeks (1-2 devs)  
**Complexity**: MEDIUM (established patterns, clear spec)  
**Risk Level**: LOW (follows proven patterns from EntityPrefab)
