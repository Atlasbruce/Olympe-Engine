# Development Environment Setup

## Requirements

| Tool | Version | Purpose |
|------|---------|---------|
| Visual Studio | 2019+ | C++ compilation |
| Windows SDK | 10.0+ | Platform headers |
| Doxygen | 1.9+ | API doc generation |
| Node.js | 18+ | Docusaurus website |
| Graphviz | 2.44+ | Doxygen class diagrams |

## IDE Setup

### Visual Studio
1. Open `Olympe Engine.sln`
2. Set startup project to `Olympe Engine`
3. Build with **F7**, run with **F5**

### VS Code (alternative)
Install extensions:
- C/C++ (Microsoft)
- CMake Tools
- clangd

## Documentation Build

### Doxygen
```bash
# From repo root
doxygen Doxyfile

# Output: website/static/api/html/index.html
```

### Docusaurus (local preview)
```bash
cd website
npm install
npm start

# Opens: http://localhost:3000/Olympe-Engine/
```

### Docusaurus build
```bash
cd website
npm run build

# Output: website/build/
```

## Running Tests
```bash
cd Tests
# Run all tests
./run_tests.sh

# Or open Tests/*.vcxproj in Visual Studio
```

## Debugging Tips

1. Use `imgui.ini` to restore window layout
2. Set breakpoints in `VisualScriptEditorPanel_*.cpp` for graph operations
3. Check `olympe.ini` for runtime configuration
4. Enable `EXTRACT_ALL = YES` in Doxyfile to see all undocumented classes
