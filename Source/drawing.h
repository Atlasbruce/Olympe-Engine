/*
purpose: This file contains functions related to drawing shapes and images on the screen.

Performance Notes:
- Draw_FilledCircle: Optimized to use integer arithmetic and minimize sqrt calls
- Draw_Hexagon/Draw_FilledHexagon: Use pre-calculated trigonometric values for better performance
*/
#pragma once

#include "vector.h"
#include <SDL_render.h>
#include <SDL_pixels.h>

static inline SDL_Color ToColor(const SDL_FColor&);
static inline SDL_FColor ToFColor(const SDL_Color&);

// Draws circle outline using Midpoint Circle Algorithm
void Draw_Circle(int cx, int cy, int radius);

// Draws filled circle using horizontal scanlines (optimized)
void Draw_FilledCircle(int cx, int cy, int radius);

void Draw_Triangle(Vector p1, Vector p2, Vector p3);
void Draw_FilledTriangle(
    const Vector& p1,
    const Vector& p2,
    const Vector& p3,
	SDL_FColor color);

// Draws hexagon outline (optimized with pre-calculated trig values)
void Draw_Hexagon(
    Vector center,
    float radius,
	SDL_Color color);

// Draws filled hexagon (optimized with pre-calculated trig values)
void Draw_FilledHexagon(
    Vector center,
	float radius,
	SDL_FColor color);
