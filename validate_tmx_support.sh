#!/bin/bash
# Validation script for TMX Support and GID Resolver implementation

echo "=========================================="
echo "TMX Support & GID Resolver Validation"
echo "=========================================="
echo ""

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Track results
PASSED=0
FAILED=0

# Function to report test result
report_result() {
    if [ $1 -eq 0 ]; then
        echo -e "${GREEN}✓ PASSED${NC}: $2"
        ((PASSED++))
    else
        echo -e "${RED}✗ FAILED${NC}: $2"
        ((FAILED++))
    fi
}

# Change to TiledLevelLoader directory
cd "$(dirname "$0")/Source/TiledLevelLoader" || exit 1

echo "1. Checking file structure..."
echo "----------------------------------------"

# Check that all necessary files exist
for file in "include/TiledLevelLoader.h" "include/TiledStructures.h" "src/TiledLevelLoader.cpp" "README.md"; do
    if [ -f "$file" ]; then
        report_result 0 "File exists: $file"
    else
        report_result 1 "File missing: $file"
    fi
done

echo ""
echo "2. Syntax validation..."
echo "----------------------------------------"

# Test TiledStructures.h syntax
g++ -std=c++14 -fsyntax-only \
    -I./include -I./third_party -I../third_party -I../third_party/nlohmann -I.. \
    include/TiledStructures.h 2>&1 | grep "error:" > /tmp/tiled_syntax_test.log

if [ ! -s /tmp/tiled_syntax_test.log ]; then
    report_result 0 "TiledStructures.h syntax is valid"
else
    report_result 1 "TiledStructures.h has syntax errors"
    cat /tmp/tiled_syntax_test.log
fi

# Test TiledLevelLoader.cpp compilation
g++ -std=c++14 -c \
    -I./include -I./third_party -I../third_party -I../third_party/nlohmann -I.. \
    src/TiledLevelLoader.cpp -o /tmp/TiledLevelLoader.o 2> /tmp/tiled_compile_test.log

if [ $? -eq 0 ]; then
    report_result 0 "TiledLevelLoader.cpp compiles successfully"
else
    report_result 1 "TiledLevelLoader.cpp has compilation errors"
    cat /tmp/tiled_compile_test.log
fi

# Test TilesetParser.cpp compilation
g++ -std=c++14 -c \
    -I./include -I./third_party -I../third_party -I../third_party/nlohmann -I.. \
    src/TilesetParser.cpp -o /tmp/TilesetParser.o 2> /tmp/tileset_compile_test.log

if [ $? -eq 0 ]; then
    report_result 0 "TilesetParser.cpp compiles successfully"
else
    report_result 1 "TilesetParser.cpp has compilation errors"
    cat /tmp/tileset_compile_test.log
fi

echo ""
echo "3. Feature verification..."
echo "----------------------------------------"

# Check for TMX parsing functions in header
if grep -q "ParseMapXML" include/TiledLevelLoader.h; then
    report_result 0 "TMX parsing functions declared"
else
    report_result 1 "TMX parsing functions not found"
fi

# Check for lastgid in TiledTileset
if grep -q "int lastgid" include/TiledStructures.h; then
    report_result 0 "lastgid field added to TiledTileset"
else
    report_result 1 "lastgid field not found in TiledTileset"
fi

# Check for CalculateLastGid method
if grep -q "CalculateLastGid" include/TiledStructures.h; then
    report_result 0 "CalculateLastGid method implemented"
else
    report_result 1 "CalculateLastGid method not found"
fi

# Check for ResolvedGid structure
if grep -q "struct ResolvedGid" include/TiledStructures.h; then
    report_result 0 "ResolvedGid structure defined"
else
    report_result 1 "ResolvedGid structure not found"
fi

# Check for FindTilesetForGid method
if grep -q "FindTilesetForGid" include/TiledStructures.h; then
    report_result 0 "FindTilesetForGid method implemented"
else
    report_result 1 "FindTilesetForGid method not found"
fi

# Check for ResolveGid function
if grep -q "ResolveGid" include/TiledStructures.h; then
    report_result 0 "ResolveGid helper function implemented"
else
    report_result 1 "ResolveGid function not found"
fi

# Check for GetAllImagePaths function
if grep -q "GetAllImagePaths" include/TiledStructures.h; then
    report_result 0 "GetAllImagePaths helper function implemented"
else
    report_result 1 "GetAllImagePaths function not found"
fi

# Check for TMX XML parsing implementation
if grep -q "ParseMapXML" src/TiledLevelLoader.cpp; then
    report_result 0 "TMX parsing implementation found"
else
    report_result 1 "TMX parsing implementation not found"
fi

# Check for auto-detection in LoadFromFile
if grep -q ".tmx" src/TiledLevelLoader.cpp && grep -q ".tmj" src/TiledLevelLoader.cpp; then
    report_result 0 "Format auto-detection implemented"
else
    report_result 1 "Format auto-detection not found"
fi

# Check for CalculateAllLastGids call
if grep -q "CalculateAllLastGids" src/TiledLevelLoader.cpp; then
    report_result 0 "lastgid calculation integrated"
else
    report_result 1 "lastgid calculation not integrated"
fi

echo ""
echo "4. Documentation verification..."
echo "----------------------------------------"

# Check README for TMX support
if grep -q "TMX" README.md; then
    report_result 0 "TMX support documented in README"
else
    report_result 1 "TMX support not documented"
fi

# Check README for GID resolver
if grep -q "GID" README.md && grep -q "ResolvedGid" README.md; then
    report_result 0 "GID resolver documented in README"
else
    report_result 1 "GID resolver not documented"
fi

# Check for usage examples
if grep -q "ResolveGid" README.md; then
    report_result 0 "Usage examples in README"
else
    report_result 1 "Usage examples not found"
fi

# Check for implementation summary document
if [ -f "../../IMPLEMENTATION_TMX_SUPPORT_AND_GID_RESOLVER.md" ]; then
    report_result 0 "Implementation summary document exists"
else
    report_result 1 "Implementation summary document not found"
fi

echo ""
echo "5. Example code verification..."
echo "----------------------------------------"

# Check for example file
if [ -f "../../Examples/TiledLoader/example_tmx_and_gid_resolver.cpp" ]; then
    report_result 0 "Example code exists"
    
    # Check example compiles
    g++ -std=c++14 -fsyntax-only \
        -I../../Source/TiledLevelLoader/include \
        -I../../Source/TiledLevelLoader/third_party \
        -I../../Source/third_party \
        -I../../Source/third_party/nlohmann \
        -I../../Source \
        ../../Examples/TiledLoader/example_tmx_and_gid_resolver.cpp 2> /tmp/example_syntax.log
    
    if [ $? -eq 0 ]; then
        report_result 0 "Example code syntax is valid"
    else
        report_result 1 "Example code has syntax errors"
        cat /tmp/example_syntax.log
    fi
else
    report_result 1 "Example code not found"
fi

echo ""
echo "=========================================="
echo "Validation Results"
echo "=========================================="
echo -e "${GREEN}Passed: $PASSED${NC}"
echo -e "${RED}Failed: $FAILED${NC}"
echo "Total: $((PASSED + FAILED))"
echo ""

if [ $FAILED -eq 0 ]; then
    echo -e "${GREEN}✓ All validation checks passed!${NC}"
    exit 0
else
    echo -e "${RED}✗ Some validation checks failed.${NC}"
    exit 1
fi
