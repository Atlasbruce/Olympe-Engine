# Quick Reference - Phase 44.4 Updates

**When**: Phase 44.4 (Latest session)  
**What**: Logging discipline + Framework UI wiring + Documentation  
**Status**: ✅ Production Ready (0 errors)

---

## 🔴 CRITICAL: Logging Rules (New)

### ❌ DO NOT DO THIS
```cpp
void RenderButtons()  // Called every frame (60 FPS)
{
    if (ImGui::Button("Save"))
        SYSTEM_LOG << "Save clicked";  // ❌ SPAM - happens 60x/sec when rendering
    
    SYSTEM_LOG << "Button rendering";  // ❌ SPAM - happens 60x/sec
}

void OnMouseMove()  // Called 100+ times per second
{
    SYSTEM_LOG << "Mouse at " << x << "," << y;  // ❌ SPAM
}
```

### ✅ DO THIS INSTEAD
```cpp
void OnSaveClicked()  // Called once when user clicks
{
    std::string path = m_document->GetFilePath();
    if (path.empty())
    {
        OnSaveAsClicked();
        return;
    }
    
    if (ExecuteSave(path))  // ✅ Log on save success
        SYSTEM_LOG << "✓ Saved: " << path << std::endl;
}

void OnSaveAsClicked()  // Called once when user clicks
{
    m_showSaveAsModal = true;  // ✅ Let modal system handle rest
    SYSTEM_LOG << "Opening Save As dialog" << std::endl;
}
```

**Rule**: If it happens 60+ times per second, DON'T LOG IT

---

## 🏗️ Framework UI + Backend Pattern (New)

### Two-Layer Architecture
```
User clicks "Save" button
         ↓
Framework UI Layer (CanvasToolbarRenderer)
  - OnSaveClicked() checks if document valid
  - Gets current path
  - ↓
Backend Layer (ExecuteSave)
  - Actually writes file to disk
  - Handles I/O errors
  - ↓
Success: File persists, callbacks fired
```

### Code Pattern
```cpp
// Framework layer - CanvasToolbarRenderer
void CanvasToolbarRenderer::OnSaveClicked()
{
    if (!m_document) return;
    std::string path = m_document->GetFilePath();
    if (path.empty()) { OnSaveAsClicked(); return; }
    
    if (ExecuteSave(path))  // ← Delegate to backend
    {
        TabManager::Get().OnGraphDocumentSaved(...);
        if (m_onSaveComplete) m_onSaveComplete(path);
    }
}

// Backend layer - Already exists and works
bool ExecuteSave(const std::string& filepath)
{
    // File I/O, serialization, error handling
    // Proven working implementation
}
```

### Key Principle
- **Framework = UI presentation** (buttons, modals, toolbars)
- **Backend = Business logic** (file I/O, validation)
- **Never duplicate** backend logic in framework
- **Always use existing utilities** when available

---

## 📋 Phase 44.4 Changes - Summary

| Changed | File | What |
|---------|------|------|
| ✅ | CanvasToolbarRenderer.cpp | Removed spam logs (lines 262-264) |
| ✅ | CanvasToolbarRenderer.cpp | OnSaveClicked → uses ExecuteSave() |
| ✅ | CanvasToolbarRenderer.cpp | OnSaveAsClicked → uses modal flag |
| ✅ | CanvasToolbarRenderer.cpp | Cleaned includes (removed problematic ones) |
| ✅ | copilot-instructions.md | Added Logging Discipline section |
| ✅ | copilot-instructions.md | Added Framework UI + Backend Pattern section |
| ✅ | copilot-instructions.md | Added Phase 44.4 directive |
| ✅ | COPILOT_CONTEXT.md | Verified complete (400+ lines, all phases) |
| ✅ | BUILD | 0 errors, 0 warnings |

---

## 🐛 If Something Breaks - Debugging Guide

### Save Button Not Working
```
Symptom: Click Save, nothing happens
Solution:
  1. Check OnSaveClicked() calls ExecuteSave(path)
  2. Verify ExecuteSave() method exists
  3. Check m_document is not null
  4. Verify path is not empty (should call SaveAs if empty)
```

### Console Spam (60 FPS logs)
```
Symptom: Console flooded with 60 FPS log messages
Solution:
  1. Find log in RenderButtons() or OnMouseMove() or other frequent methods
  2. DELETE IT (it's spam)
  3. Move log to user action (OnSaveClicked, etc.)
  4. Keep only "important" logs (errors, actions, init)
```

### Build Fails with "Can't find include"
```
Symptom: C1083 error on include path
Solution:
  1. Check if include is REALLY needed
  2. Look for existing utility that does the same thing
  3. Use existing method instead of adding new include
  4. Example: Use ExecuteSave() not NodeGraphManager
  5. Remove problematic include
  6. Rebuild
```

---

## 📚 Documentation Files to Know

| File | Purpose | Read Time |
|------|---------|-----------|
| `.github/copilot-instructions.md` | Developer guidelines (UPDATED Phase 44.4) | 5 min |
| `.github/COPILOT_CONTEXT.md` | Complete history + patterns (400+ lines) | 15 min |
| `.github/PHASE_44_4_FINAL_SUMMARY.md` | Phase 44.4 completion summary | 10 min |
| `.github/QUICK_REFERENCE_44_2_1.md` | Old reference (for comparison) | Optional |

---

## 🚀 Before You Code

### Checklist
- [ ] Read copilot-instructions.md (logging section especially)
- [ ] Reference COPILOT_CONTEXT.md for patterns
- [ ] Check if existing utility exists before writing new code
- [ ] Avoid logging in render loops
- [ ] Use framework + backend pattern for new features
- [ ] Test build (0 errors is mandatory)

### Quick Commands
```bash
# Build (from repo root)
cmake . && cmake --build . --config Release

# Check build status
# Expected: "Génération réussie" or "Build successful"
```

---

## 🎯 Key Learnings (Don't Forget!)

1. **Simple > Complex**
   - 8-line method > 80-line method every time
   - Fewer lines = fewer bugs
   - Keep it simple

2. **Proven > New**
   - Use working legacy code instead of incomplete new designs
   - ExecuteSave() works, don't rewrite it
   - Ship working code now, improve architecture later

3. **Delegate > Duplicate**
   - Framework uses ExecuteSave(), not reimplementing it
   - Backend handles business logic
   - Framework handles presentation

4. **Build Iteration**
   - Build fails with 142 errors? Recognize over-complication
   - Simplify and rebuild
   - If it worked before, use what worked

5. **Log Discipline**
   - Frame-by-frame logs = debugging nightmare
   - Action-based logs = clean console
   - If it happens 60x/sec, don't log it

---

## 📞 Need More Info?

- **Logging questions**: See `.github/copilot-instructions.md` Logging Discipline section
- **Architecture questions**: See `.github/COPILOT_CONTEXT.md` Architecture Patterns section
- **Bug questions**: See `.github/COPILOT_CONTEXT.md` Critical Bug Fixes section
- **File questions**: See `.github/COPILOT_CONTEXT.md` File Reference Guide section

---

**Phase 44.4 Complete ✅**  
**Production Ready ✅**  
**Documented ✅**  
**Refer to this when coding next features**
