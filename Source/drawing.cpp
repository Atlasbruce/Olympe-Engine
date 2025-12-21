#include "drawing.h"
#include <SDL3/SDL_render.h>
#include <cmath>


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
void Draw_Circle(SDL_Renderer* renderer, int cx, int cy, int radius)
{
    int x = radius;
    int y = 0;
    int err = 0;

    while (x >= y)
    {
        SDL_RenderPoint(renderer, (float)cx + x, (float)cy + y);
        SDL_RenderPoint(renderer, (float)cx + y, (float)cy + x);
        SDL_RenderPoint(renderer, (float)cx - y, (float)cy + x);
        SDL_RenderPoint(renderer, (float)cx - x, (float)cy + y);
        SDL_RenderPoint(renderer, (float)cx - x, (float)cy - y);
        SDL_RenderPoint(renderer, (float)cx - y, (float)cy - x);
        SDL_RenderPoint(renderer, (float)cx + y, (float)cy - x);
        SDL_RenderPoint(renderer, (float)cx + x, (float)cy - y);

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
}
//----------------------------------------------------------
// Draws a filled circle using horizontal scanlines
// Optimized: use integer arithmetic to avoid sqrt in loop
void Draw_FilledCircle(SDL_Renderer* renderer, int cx, int cy, int radius)
{
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
        SDL_RenderLine(renderer, (float)x1, (float)cy + dy, (float)x2, (float)cy + dy);
    }
}
//----------------------------------------------------------
// Draws the outline of a triangle
void Draw_Triangle(SDL_Renderer* renderer, Vector p1, Vector p2, Vector p3)
{
    SDL_RenderLine(renderer, p1.x, p1.y, p2.x, p2.y);
    SDL_RenderLine(renderer, p2.x, p2.y, p3.x, p3.y);
    SDL_RenderLine(renderer, p3.x, p3.y, p1.x, p1.y);
}
//----------------------------------------------------------
// Draws a filled triangle using SDL_RenderGeometry
void Draw_FilledTriangle(SDL_Renderer* renderer,
    const Vector& p1,
    const Vector& p2,
    const Vector& p3,
    SDL_FColor color)
{
    SDL_Vertex vertices[3];

    vertices[0].position = p1.ToFPoint();
	vertices[1].position = p2.ToFPoint();
    vertices[2].position = p3.ToFPoint();

    vertices[0].color = color;
    vertices[1].color = color;
    vertices[2].color = color;

    vertices[0].tex_coord = { 0, 0 };
    vertices[1].tex_coord = { 0, 0 };
    vertices[2].tex_coord = { 0, 0 };

    SDL_RenderGeometry(renderer, nullptr, vertices, 3, nullptr, 0);
}
//----------------------------------------------------------
// Draws a filled hexagon using SDL_RenderGeometry
void Draw_FilledHexagon(SDL_Renderer* renderer,
    Vector center,
    float radius,
    SDL_FColor color)
{
    const int numSides = 6;
    SDL_Vertex vertices[numSides + 1];
    int indices[numSides * 3];

    // Le centre du polygone
    vertices[0].position = center.ToFPoint();
    vertices[0].color = color;
    vertices[0].tex_coord = { 0, 0 };

    // Calcul des sommets sur le cercle avec valeurs pré-calculées
    for (int i = 0; i < numSides; ++i) {
        vertices[i + 1].position = {
            center.x + radius * HEXAGON_COS_ANGLES[i],
            center.y + radius * HEXAGON_SIN_ANGLES[i]
        };
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
    SDL_RenderGeometry(renderer, nullptr, vertices, numSides + 1, indices, numSides * 3);
}
//----------------------------------------------------------
// Draw hexagon outline
void Draw_Hexagon(SDL_Renderer* renderer, Vector center, float radius, SDL_Color color)
{
    const int numSides = 6;
    Vector verts[numSides];
    // Use pre-calculated trig values for performance
    for (int i = 0; i < numSides; ++i) {
        verts[i].x = center.x + radius * HEXAGON_COS_ANGLES[i];
        verts[i].y = center.y + radius * HEXAGON_SIN_ANGLES[i];
    }

    // Apply color
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);

    // Draw lines between consecutive vertices
    for (int i = 0; i < numSides; ++i) {
        int next = (i + 1) % numSides;
        SDL_RenderLine(renderer, verts[i].x, verts[i].y, verts[next].x, verts[next].y);
    }
}
