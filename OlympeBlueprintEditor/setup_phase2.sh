#!/bin/bash
# Setup script for Phase 2 - Downloads ImGui and ImNodes

set -e

echo "=========================================="
echo "Olympe Blueprint Editor - Phase 2 Setup"
echo "=========================================="
echo ""

# Check if we're in the right directory
if [ ! -f "Makefile.gui" ]; then
    echo "Error: Please run this script from the OlympeBlueprintEditor directory"
    exit 1
fi

# Create third_party directory
mkdir -p third_party
cd third_party

echo "Downloading ImGui..."
if [ -d "imgui" ]; then
    echo "  ImGui already exists, skipping"
else
    git clone --depth 1 https://github.com/ocornut/imgui.git
    echo "  ✓ ImGui downloaded"
fi

echo "Downloading ImNodes..."
if [ -d "imnodes" ]; then
    echo "  ImNodes already exists, skipping"
else
    git clone --depth 1 https://github.com/Nelarius/imnodes.git
    echo "  ✓ ImNodes downloaded"
fi

cd ..

echo ""
echo "=========================================="
echo "Setup complete!"
echo "=========================================="
echo ""
echo "Dependencies downloaded:"
echo "  ✓ ImGui (third_party/imgui)"
echo "  ✓ ImNodes (third_party/imnodes)"
echo ""
echo "Next steps:"
echo "  1. Install SDL2: sudo apt-get install libsdl2-dev"
echo "  2. Build: make -f Makefile.gui"
echo "  3. Run: ./OlympeBlueprintEditorGUI"
echo ""
