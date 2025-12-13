#!/bin/bash
# Olympe Blueprint Editor - Demo Script
# This script demonstrates the editor functionality without manual interaction

echo "========================================="
echo "Olympe Blueprint Editor - Demonstration"
echo "========================================="
echo ""
echo "This demo shows the functional Blueprint Editor that allows:"
echo "  1. Loading entity blueprints from JSON files"
echo "  2. Editing component properties visually"
echo "  3. Adding/Removing components"
echo "  4. Saving blueprints back to JSON"
echo ""
echo "The editor is fully functional and ready to use!"
echo ""
echo "--- Running automated tests ---"
echo ""

# Get the script directory and navigate to project root
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"

cd "$PROJECT_ROOT"

# Build the test if not already built
if [ ! -f "./OlympeBlueprintEditor/build/blueprint_test" ]; then
    echo "Building test suite..."
    cd OlympeBlueprintEditor && make test > /dev/null 2>&1 && cd ..
fi

# Run the blueprint test to show functionality
if [ -f "./OlympeBlueprintEditor/build/blueprint_test" ]; then
    ./OlympeBlueprintEditor/build/blueprint_test
else
    echo "Note: Test suite not available. Run 'make test' in OlympeBlueprintEditor directory."
fi

echo ""
echo "========================================="
echo "Editor Features:"
echo "========================================="
echo ""
echo "✓ Load/Save entity blueprints (JSON)"
echo "✓ View and edit component properties"
echo "✓ Add new components (Position, BoundingBox, VisualSprite, etc.)"
echo "✓ Remove components"
echo "✓ Visual property inspector"
echo "✓ Support for nested properties (x, y, z coordinates, etc.)"
echo "✓ Type-safe property editing (numbers, strings, booleans)"
echo ""
echo "To run the interactive editor manually:"
echo "  cd /home/runner/work/Olympe-Engine/Olympe-Engine"
echo "  ./OlympeBlueprintEditor/OlympeBlueprintEditor"
echo ""
echo "Example blueprints available:"
echo "  - Blueprints/example_entity_simple.json"
echo "  - Blueprints/example_entity_complete.json"
echo "  - Blueprints/test_entity_generated.json (created by test)"
echo "  - Blueprints/test_entity_modified.json (modified by test)"
echo ""
echo "========================================="
