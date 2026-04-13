# Troubleshooting Guide

## Build Issues

### "Cannot open source file" errors
- Check that `Source/` subdirectory paths are correct in the `.vcxproj`
- Note: repo has both `Source/System/` and `Source/system/` (case-sensitive on Linux)

### SDL3 not found
- Ensure `SDL/` directory is present at repo root
- Check `$(SolutionDir)SDL\` in project include paths

## Editor Issues

### Node positions incorrect after zoom/pan
- This was fixed in Phase 29. Ensure `ScreenToCanvas()` uses:
  ```cpp
  canvas = (screen - canvasOrigin - offset) / zoom
  ```
  NOT `offset * zoom`.

### Prefab not loading
- Verify JSON file exists in `Gamedata/PrefabEntities/`
- Check `version` field equals `4`
- Check `nodes` array is not empty

### Component palette empty
- Check `Gamedata/PrefabEntities/ComponentsParameters.json` exists
- Validate JSON syntax (use a JSON validator)
- Fallback: hardcoded components are used if file is missing

### Minimap not showing
- Check `CanvasMinimapRenderer::SetVisible(true)` is called
- Ensure `Render()` is called after the canvas child window

## Documentation Issues

### Docusaurus build fails
```bash
cd website
npm ci       # clean install
npm run build
```

### Doxygen not generating diagrams
- Install Graphviz: `sudo apt-get install graphviz`
- Ensure `HAVE_DOT = YES` in Doxyfile
- Check `dot` is in PATH: `which dot`

### Broken links in sidebar
- Verify the `id:` in the front matter matches the sidebar entry
- Run: `npm run build` to see all broken link errors

## Common JSON Errors

| Error | Cause | Fix |
|-------|-------|-----|
| `[json.exception.type_error]` | Wrong type for a field | Check field type in schema reference |
| `[json.exception.out_of_range]` | Array index out of bounds | Validate array sizes |
| `version` mismatch | Old file format | Run migration tool or update `version: 4` |
