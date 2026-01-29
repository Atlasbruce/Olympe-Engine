/*
    Olympe Engine - 2025
	Nicolas Chereau
    nchereau@gmail.com

	This file is part of Olympe Engine.
	Purpose: This file contains functions related to drawing shapes and images on the screen.


*/
#pragma once
#include "drawing.h"
#include <SDL3/SDL_render.h>
#include <cmath>
#include "GameEngine.h"
#include "RenderContext.h"


// Portable pi definition for C++14 (avoids M_PI reliance)
static const float PI = static_cast<float>(std::acos(-1.0));

// Pre-calculated hexagon vertices (unit circle) for performance optimization
// These are the normalized positions for a regular hexagon
static const float HEXAGON_COS_ANGLES[6] = {
    1.0f,           // 0°
    0.5f,           // 60°
    -0.5f,          // 120°
    -1.0f,          // 180°
    -0.5f,          // 240°
    0.5f            // 300°
};

static const float HEXAGON_SIN_ANGLES[6] = {
    0.0f,           // 0°
    0.866025404f,   // 60° (sqrt(3)/2)
    0.866025404f,   // 120°
    0.0f,           // 180°
    -0.866025404f,  // 240°
    -0.866025404f   // 300°
};

// Color conversion helpers
static inline SDL_FColor ToFColor(const SDL_Color& c)
{
    return SDL_FColor{
        c.r / 255.0f,
        c.g / 255.0f,
        c.b / 255.0f,
        c.a / 255.0f
    };
}

static inline SDL_Color ToColor(const SDL_FColor& f)
{
    auto clamp01 = [](float v) {
        if (v < 0.0f) return 0.0f;
        if (v > 1.0f) return 1.0f;
        return v;
        };
    return SDL_Color{
        static_cast<Uint8>(std::round(clamp01(f.r) * 255.0f)),
        static_cast<Uint8>(std::round(clamp01(f.g) * 255.0f)),
        static_cast<Uint8>(std::round(clamp01(f.b) * 255.0f)),
        static_cast<Uint8>(std::round(clamp01(f.a) * 255.0f))
    };
}
//----------------------------------------------------------
// Draws a circle using the Midpoint Circle Algorithm
void Draw_Circle(int cx, int cy, int radius)
{
    const CameraTransform& cam = RenderContext::Get().GetActiveCamera();
    
    if (!cam.isActive) {
        // Legacy screen-space rendering (no camera active)
        int x = radius;
        int y = 0;
        int err = 0;

        while (x >= y)
        {
            SDL_RenderPoint(GameEngine::renderer, (float)cx + x, (float)cy + y);
            SDL_RenderPoint(GameEngine::renderer, (float)cx + y, (float)cy + x);
            SDL_RenderPoint(GameEngine::renderer, (float)cx - y, (float)cy + x);
            SDL_RenderPoint(GameEngine::renderer, (float)cx - x, (float)cy + y);
            SDL_RenderPoint(GameEngine::renderer, (float)cx - x, (float)cy - y);
            SDL_RenderPoint(GameEngine::renderer, (float)cx - y, (float)cy - x);
            SDL_RenderPoint(GameEngine::renderer, (float)cx + y, (float)cy - x);
            SDL_RenderPoint(GameEngine::renderer, (float)cx + x, (float)cy - y);

            if (err <= 0)
            {
                y += 1;
                err += 2 * y + 1;
            }
            if (err > 0)
            {
                x -= 1;
                err -= 2 * x + 1;
            }
        }
        return;
    }
    
    // Camera active: transform world coordinates to screen
    // Note: Circles are rotationally symmetric, so camera rotation has no effect
    Vector worldCenter((float)cx, (float)cy, 0.0f);
    Vector screenCenter = cam.WorldToScreen(worldCenter);
    float screenRadius = radius * cam.zoom;
    
    int sCx = static_cast<int>(screenCenter.x);
    int sCy = static_cast<int>(screenCenter.y);
    int sRadius = static_cast<int>(screenRadius);
    
    int x = sRadius;
    int y = 0;
    int err = 0;

    while (x >= y) {
        SDL_RenderPoint(GameEngine::renderer, (float)sCx + x, (float)sCy + y);
        SDL_RenderPoint(GameEngine::renderer, (float)sCx + y, (float)sCy + x);
        SDL_RenderPoint(GameEngine::renderer, (float)sCx - y, (float)sCy + x);
        SDL_RenderPoint(GameEngine::renderer, (float)sCx - x, (float)sCy + y);
        SDL_RenderPoint(GameEngine::renderer, (float)sCx - x, (float)sCy - y);
        SDL_RenderPoint(GameEngine::renderer, (float)sCx - y, (float)sCy - x);
        SDL_RenderPoint(GameEngine::renderer, (float)sCx + y, (float)sCy - x);
        SDL_RenderPoint(GameEngine::renderer, (float)sCx + x, (float)sCy - y);

        if (err <= 0) { y += 1; err += 2 * y + 1; }
        if (err > 0) { x -= 1; err -= 2 * x + 1; }
    }
}
//----------------------------------------------------------
// Draws a filled circle using horizontal scanlines
// Optimized: use integer arithmetic to avoid sqrt in loop
void Draw_FilledCircle(int cx, int cy, int radius)
{
    const CameraTransform& cam = RenderContext::Get().GetActiveCamera();
    
    if (!cam.isActive) {
        // Legacy screen-space rendering
        int r2 = radius * radius;
        for (int dy = -radius; dy <= radius; ++dy)
        {
            // Use integer arithmetic: dx^2 = r^2 - dy^2
            int dy2 = dy * dy;
            int dx2 = r2 - dy2;
            // Only compute sqrt once per scanline
            int dx = static_cast<int>(std::sqrt(static_cast<float>(dx2)));
            int x1 = cx - dx;
            int x2 = cx + dx;
            SDL_RenderLine(GameEngine::renderer, (float)x1, (float)cy + dy, (float)x2, (float)cy + dy);
        }
        return;
    }
    
    // Camera active: transform center and scale radius
    // Note: Filled circles are rotationally symmetric, so camera rotation has no effect
    Vector worldCenter((float)cx, (float)cy, 0.0f);
    Vector screenCenter = cam.WorldToScreen(worldCenter);
    float screenRadius = radius * cam.zoom;
    
    int sCx = static_cast<int>(screenCenter.x);
    int sCy = static_cast<int>(screenCenter.y);
    int sRadius = static_cast<int>(screenRadius);
    
    int r2 = sRadius * sRadius;
    for (int dy = -sRadius; dy <= sRadius; ++dy) {
        int dy2 = dy * dy;
        int dx2 = r2 - dy2;
        int dx = static_cast<int>(std::sqrt(static_cast<float>(dx2)));
        int x1 = sCx - dx;
        int x2 = sCx + dx;
        SDL_RenderLine(GameEngine::renderer, (float)x1, (float)sCy + dy, (float)x2, (float)sCy + dy);
    }
}
//----------------------------------------------------------
// Draws the outline of a triangle
void Draw_Triangle(Vector p1, Vector p2, Vector p3)
{
    const CameraTransform& cam = RenderContext::Get().GetActiveCamera();
    
    if (!cam.isActive) {
        // Legacy screen-space rendering
        SDL_RenderLine(GameEngine::renderer, p1.x, p1.y, p2.x, p2.y);
        SDL_RenderLine(GameEngine::renderer, p2.x, p2.y, p3.x, p3.y);
        SDL_RenderLine(GameEngine::renderer, p3.x, p3.y, p1.x, p1.y);
        return;
    }
    
    // Transform world coordinates to screen
    Vector s1 = cam.WorldToScreen(p1);
    Vector s2 = cam.WorldToScreen(p2);
    Vector s3 = cam.WorldToScreen(p3);
    
    SDL_RenderLine(GameEngine::renderer, s1.x, s1.y, s2.x, s2.y);
    SDL_RenderLine(GameEngine::renderer, s2.x, s2.y, s3.x, s3.y);
    SDL_RenderLine(GameEngine::renderer, s3.x, s3.y, s1.x, s1.y);
}
//----------------------------------------------------------
// Draws a filled triangle using SDL_RenderGeometry
void Draw_FilledTriangle(
    const Vector& p1,
    const Vector& p2,
    const Vector& p3,
    SDL_FColor color)
{
    const CameraTransform& cam = RenderContext::Get().GetActiveCamera();
    
    Vector s1 = cam.isActive ? cam.WorldToScreen(p1) : p1;
    Vector s2 = cam.isActive ? cam.WorldToScreen(p2) : p2;
    Vector s3 = cam.isActive ? cam.WorldToScreen(p3) : p3;
    
    SDL_Vertex vertices[3];

    vertices[0].position = s1.ToFPoint();
    vertices[1].position = s2.ToFPoint();
    vertices[2].position = s3.ToFPoint();

    vertices[0].color = color;
    vertices[1].color = color;
    vertices[2].color = color;

    vertices[0].tex_coord = { 0, 0 };
    vertices[1].tex_coord = { 0, 0 };
    vertices[2].tex_coord = { 0, 0 };

    SDL_RenderGeometry(GameEngine::renderer, nullptr, vertices, 3, nullptr, 0);
}
//----------------------------------------------------------
// Draws a filled hexagon using SDL_RenderGeometry
void Draw_FilledHexagon(
    Vector center,
    float radius,
    SDL_FColor color)
{
    const CameraTransform& cam = RenderContext::Get().GetActiveCamera();
    
    const int numSides = 6;
    SDL_Vertex vertices[numSides + 1];
    int indices[numSides * 3];

    // Transform center through camera (applies rotation, zoom, position)
    Vector screenCenter = cam.isActive ? cam.WorldToScreen(center) : center;

    // Le centre du polygone
    vertices[0].position = screenCenter.ToFPoint();
    vertices[0].color = color;
    vertices[0].tex_coord = { 0, 0 };

    // Calculate vertices in world space, then transform each through camera
    // This ensures rotation is applied correctly by WorldToScreen
    for (int i = 0; i < numSides; ++i) {
        Vector worldVertex(
            center.x + radius * HEXAGON_COS_ANGLES[i],
            center.y + radius * HEXAGON_SIN_ANGLES[i],
            0.0f
        );
        
        Vector screenVertex = cam.isActive ? cam.WorldToScreen(worldVertex) : worldVertex;
        
        vertices[i + 1].position = screenVertex.ToFPoint();
        vertices[i + 1].color = color;
        vertices[i + 1].tex_coord = { 0, 0 };
    }

    // Cr�e 6 triangles reliant le centre et les sommets du bord
    for (int i = 0; i < numSides; ++i) {
        int next = (i + 1) % numSides;
        indices[i * 3 + 0] = 0;           // centre
        indices[i * 3 + 1] = i + 1;       // sommet actuel
        indices[i * 3 + 2] = next + 1;    // sommet suivant
    }

    // Rendu de la g�om�trie
    SDL_RenderGeometry(GameEngine::renderer, nullptr, vertices, numSides + 1, indices, numSides * 3);
}
//----------------------------------------------------------
// Draw hexagon outline
void Draw_Hexagon(Vector center, float radius, SDL_Color color)
{
    const CameraTransform& cam = RenderContext::Get().GetActiveCamera();
    
    const int numSides = 6;
    Vector verts[numSides];
    
    // Calculate vertices in world space, then transform each through camera
    // This ensures rotation is applied correctly by WorldToScreen
    for (int i = 0; i < numSides; ++i) {
        Vector worldVertex(
            center.x + radius * HEXAGON_COS_ANGLES[i],
            center.y + radius * HEXAGON_SIN_ANGLES[i],
            0.0f
        );
        
        verts[i] = cam.isActive ? cam.WorldToScreen(worldVertex) : worldVertex;
    }

    // Apply color
    SDL_SetRenderDrawColor(GameEngine::renderer, color.r, color.g, color.b, color.a);

    // Draw lines between consecutive vertices
    for (int i = 0; i < numSides; ++i) {
        int next = (i + 1) % numSides;
        SDL_RenderLine(GameEngine::renderer, verts[i].x, verts[i].y, verts[next].x, verts[next].y);
    }
}
//----------------------------------------------------------
// Draw rectangle outline with float coordinates
void Draw_Rectangle(const SDL_FRect* rect, SDL_Color color)
{
    const CameraTransform& cam = RenderContext::Get().GetActiveCamera();
    
    if (!cam.isActive) {
        // Legacy screen-space rendering
        SDL_SetRenderDrawColor(GameEngine::renderer, color.r, color.g, color.b, color.a);
        SDL_RenderLine(GameEngine::renderer, rect->x, rect->y, rect->x + rect->w, rect->y);
        SDL_RenderLine(GameEngine::renderer, rect->x + rect->w, rect->y, rect->x + rect->w, rect->y + rect->h);
        SDL_RenderLine(GameEngine::renderer, rect->x + rect->w, rect->y + rect->h, rect->x, rect->y + rect->h);
        SDL_RenderLine(GameEngine::renderer, rect->x, rect->y + rect->h, rect->x, rect->y);
        return;
    }
    
    // Transform 4 corners to screen space
    Vector corners[4] = {
        Vector(rect->x, rect->y, 0.0f),
        Vector(rect->x + rect->w, rect->y, 0.0f),
        Vector(rect->x + rect->w, rect->y + rect->h, 0.0f),
        Vector(rect->x, rect->y + rect->h, 0.0f)
    };
    
    Vector screenCorners[4];
    for (int i = 0; i < 4; ++i) {
        screenCorners[i] = cam.WorldToScreen(corners[i]);
    }
    
    SDL_SetRenderDrawColor(GameEngine::renderer, color.r, color.g, color.b, color.a);
    for (int i = 0; i < 4; ++i) {
        int next = (i + 1) % 4;
        SDL_RenderLine(GameEngine::renderer, 
                      screenCorners[i].x, screenCorners[i].y,
                      screenCorners[next].x, screenCorners[next].y);
    }
}
