# Installation Guide

## Prerequisites

### Required
- **Visual Studio 2019+** (Windows) or **CMake 3.16+** (cross-platform)
- **SDL3** – included in `SDL/` directory
- **C++14** compatible compiler

### Optional
- **Doxygen 1.9+** – for API documentation generation
- **Node.js 18+** – for Docusaurus website
- **Graphviz** – for Doxygen class diagrams

## Steps

### 1. Clone the Repository
```bash
git clone https://github.com/Atlasbruce/Olympe-Engine.git
cd Olympe-Engine
```

### 2. Open in Visual Studio
Open `Olympe Engine.sln` in Visual Studio 2019 or later.

### 3. Build
- Select **Release** or **Debug** configuration
- Press **F7** to build

### 4. Run
- Press **F5** or run the `Olympe Engine` executable from `x64/Debug/` or `x64/Release/`

## Documentation Setup (Optional)

### Generate Doxygen API docs
```bash
doxygen Doxyfile
```
Output is placed in `website/static/api/`.

### Run Docusaurus website locally
```bash
cd website
npm install
npm start
```
Website is available at http://localhost:3000/Olympe-Engine/
