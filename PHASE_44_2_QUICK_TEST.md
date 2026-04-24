# Phase 44.2 Quick Test - 5 Minutes

**Goal**: Verify toolbar Save/SaveAs works across all three editors

**Time**: ~5 minutes to complete all three tests

---

## Quick Test Procedure

### TEST 1: VisualScript (2 minutes)

```
1. Launch Olympe Blueprint Editor
2. File → New → VisualScript
3. Add 2 nodes (click nodes in canvas)
4. Verify title shows "*NewGraph.ats" (dirty flag)
5. CLICK TOOLBAR "Save" BUTTON
   ✅ If save dialog appears + file saves → PASS
   ❌ If nothing happens → FAIL
6. Verify "*" disappears from title
   ✅ If gone → PASS
   ❌ If still there → FAIL
```

**Quick Result**: PASS ✅ or FAIL ❌

---

### TEST 2: BehaviorTree (2 minutes)

```
1. File → New → BehaviorTree
2. Add 2 nodes (click nodes in canvas)
3. Verify title shows "*NewTree.bts" (dirty flag)
4. CLICK TOOLBAR "Save" BUTTON
   ✅ If save dialog appears + file saves → PASS
   ❌ If nothing happens → FAIL
5. Verify "*" disappears from title
   ✅ If gone → PASS
   ❌ If still there → FAIL
```

**Quick Result**: PASS ✅ or FAIL ❌

---

### TEST 3: EntityPrefab (1 minute)

```
1. File → New → Entity Prefab
2. In ComponentPalette, double-click one component
3. Verify title shows "*NewPrefab" (dirty flag)
4. CLICK TOOLBAR "Save" BUTTON
   ✅ If save dialog appears + file saves → PASS
   ❌ If nothing happens → FAIL
5. Verify "*" disappears from title
   ✅ If gone → PASS
   ❌ If still there → FAIL
```

**Quick Result**: PASS ✅ or FAIL ❌

---

## Overall Result

**IF ALL THREE PASS**: ✅ **Phase 44.2 is working correctly**
- Toolbar Save buttons functional
- Document unification successful
- Ready for full testing

**IF ANY FAIL**: ❌ **Investigate the failure**
- Check which editor(s) failed
- Use detailed testing guide for diagnosis
- Check logs for error messages

---

## What's Being Tested

These quick tests verify the **critical fix**:
- Before Phase 44.2: Toolbar saved to wrong document (bug)
- After Phase 44.2: Toolbar saves to correct document (fixed)

**How it works**:
1. New renderer created
2. Renderer creates internal document
3. TabManager calls `renderer->GetDocument()` (NOT creating new document)
4. Framework receives same document instance
5. Toolbar Save calls correct document's Save() method
6. ✅ File persists to disk

---

## Quick Troubleshooting

| Symptom | Likely Cause | Check |
|---------|---|---|
| Save dialog never appears | Toolbar button not calling framework | Check CanvasToolbarRenderer |
| Dialog appears but no file saved | Document reference wrong | Verify GetDocument() returns valid pointer |
| Dirty flag doesn't clear | ClearDirty() not called | Check Save() implementation |
| Wrong file saved | Multiple documents exist | Verify only one document per renderer |
| All tests fail in one editor | GetDocument() not exposed | Check renderer header for public method |

---

## One-Minute Result Summary

After running 3 quick tests, report:

```
TEST 1 (VisualScript): [PASS ✅ / FAIL ❌]
TEST 2 (BehaviorTree): [PASS ✅ / FAIL ❌]
TEST 3 (EntityPrefab): [PASS ✅ / FAIL ❌]

OVERALL: [ALL PASS ✅ / NEEDS DEBUG ❌]
```

---

## If All Tests Pass ✅

Next steps:
1. Run full testing guide (PHASE_44_2_FUNCTIONAL_TESTING_GUIDE.md)
2. Test SaveAs functionality
3. Test multi-tab scenarios
4. Test dirty flag lifecycle
5. Report completion to user

---

## If Any Test Fails ❌

Next steps:
1. Identify which editor(s) failed
2. Check error messages in Output window
3. Use detailed troubleshooting guide
4. Verify GetDocument() methods are public (not private)
5. Check build log for warnings

---

**Ready?** Launch the application and run the three quick tests!

*Phase 44.2 Quick Verification*  
*Execution Time: 5 minutes*  
*Target: Verify toolbar Save functionality*
