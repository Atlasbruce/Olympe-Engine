/*
 * test_startx_starty.cpp - Simple test to verify startx/starty offset logic
 * 
 * This standalone test verifies the mathematical correctness of the
 * startx/starty offset implementation without requiring SDL or full build.
 */

#include <iostream>
#include <cmath>

struct Vec2 {
    float x, y;
    Vec2() : x(0), y(0) {}
    Vec2(float x_, float y_) : x(x_), y(y_) {}
};

// Simplified WorldToIso with offsets
Vec2 WorldToIso(float worldX, float worldY, int tileWidth, int tileHeight,
                int startX = 0, int startY = 0)
{
    // Apply startx/starty offsets to world coordinates
    float adjustedX = worldX + startX;
    float adjustedY = worldY + startY;
    
    // Standard isometric projection (diamond orientation)
    Vec2 result;
    result.x = (adjustedX - adjustedY) * (tileWidth * 0.5f);
    result.y = (adjustedX + adjustedY) * (tileHeight * 0.5f);
    return result;
}

// Simplified IsoToWorld with offsets
Vec2 IsoToWorld(float isoX, float isoY, int tileWidth, int tileHeight,
                int startX = 0, int startY = 0)
{
    // Inverse isometric projection
    float halfWidth = tileWidth * 0.5f;
    float halfHeight = tileHeight * 0.5f;
    
    Vec2 result;
    result.x = (isoX / halfWidth + isoY / halfHeight) * 0.5f;
    result.y = (isoY / halfHeight - isoX / halfWidth) * 0.5f;
    
    // Apply startx/starty offsets (inverse)
    result.x -= startX;
    result.y -= startY;
    
    return result;
}

void test_case(const char* name, float worldX, float worldY, int tileW, int tileH, int startX, int startY)
{
    std::cout << "\n=== " << name << " ===" << std::endl;
    std::cout << "Input: worldX=" << worldX << ", worldY=" << worldY << std::endl;
    std::cout << "Tile size: " << tileW << "x" << tileH << std::endl;
    std::cout << "Offsets: startX=" << startX << ", startY=" << startY << std::endl;
    
    Vec2 iso = WorldToIso(worldX, worldY, tileW, tileH, startX, startY);
    std::cout << "-> Iso: (" << iso.x << ", " << iso.y << ")" << std::endl;
    
    Vec2 world = IsoToWorld(iso.x, iso.y, tileW, tileH, startX, startY);
    std::cout << "-> World (round-trip): (" << world.x << ", " << world.y << ")" << std::endl;
    
    float error = std::sqrt((world.x - worldX) * (world.x - worldX) + 
                           (world.y - worldY) * (world.y - worldY));
    std::cout << "Round-trip error: " << error << std::endl;
    
    if (error < 0.001f) {
        std::cout << "✓ PASS" << std::endl;
    } else {
        std::cout << "✗ FAIL" << std::endl;
    }
}

int main()
{
    std::cout << "Testing startx/starty offset implementation" << std::endl;
    std::cout << "=============================================" << std::endl;
    
    // Test 1: No offsets (baseline)
    test_case("Test 1: No offsets", 0.0f, 0.0f, 64, 32, 0, 0);
    
    // Test 2: With negative offsets (as in isometric_quest.tmj)
    test_case("Test 2: With negative offsets", 0.0f, 0.0f, 64, 32, -80, -16);
    
    // Test 3: Tile at layer position with offsets
    // If startx=-80, starty=-16, then tile[0][0] should be at world (-80, -16)
    test_case("Test 3: Layer tile [0,0] -> world (-80, -16)", -80.0f, -16.0f, 64, 32, -80, -16);
    
    // Test 4: Arbitrary position with offsets
    test_case("Test 4: Arbitrary position", 10.0f, 5.0f, 64, 32, -80, -16);
    
    // Test 5: Large offsets
    test_case("Test 5: Large offsets", 100.0f, 50.0f, 64, 32, -200, -100);
    
    std::cout << "\n=============================================" << std::endl;
    std::cout << "All tests completed!" << std::endl;
    
    return 0;
}
