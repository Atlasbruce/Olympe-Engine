/**
 * @file Tests/sdl3_stub/SDL3/SDL.h
 * @brief Minimal SDL3 stub for unit test compilation on Linux.
 *
 * This stub provides only the SDL types and macros used by engine source
 * headers (SDL_Color, SDL_FPoint, SDL_Point). It allows test binaries to
 * compile and link without the full SDL3 runtime library.
 *
 * NOT for use in production builds.
 */
#pragma once

#include <stdint.h>

// ---------------------------------------------------------------------------
// Basic SDL types
// ---------------------------------------------------------------------------
typedef uint8_t  Uint8;
typedef uint16_t Uint16;
typedef uint32_t Uint32;
typedef int8_t   Sint8;
typedef int16_t  Sint16;
typedef int32_t  Sint32;
typedef int64_t  Sint64;
typedef uint64_t Uint64;
typedef int      SDL_bool;

#define SDL_TRUE  1
#define SDL_FALSE 0

// ---------------------------------------------------------------------------
// SDL API decoration macros
// ---------------------------------------------------------------------------
#ifndef SDL_DECLSPEC
#  define SDL_DECLSPEC
#endif
#ifndef SDLCALL
#  define SDLCALL
#endif
#ifndef SDL_FORCE_INLINE
#  define SDL_FORCE_INLINE inline
#endif

// ---------------------------------------------------------------------------
// Colour
// ---------------------------------------------------------------------------
typedef struct SDL_Color
{
    Uint8 r;
    Uint8 g;
    Uint8 b;
    Uint8 a;
} SDL_Color;

// ---------------------------------------------------------------------------
// Geometry primitives (from SDL_rect.h)
// ---------------------------------------------------------------------------
typedef struct SDL_Point
{
    int x;
    int y;
} SDL_Point;

typedef struct SDL_FPoint
{
    float x;
    float y;
} SDL_FPoint;

typedef struct SDL_Rect
{
    int x, y;
    int w, h;
} SDL_Rect;

typedef struct SDL_FRect
{
    float x, y;
    float w, h;
} SDL_FRect;

// ---------------------------------------------------------------------------
// Stub declarations for functions that may appear in headers
// ---------------------------------------------------------------------------
SDL_FORCE_INLINE SDL_bool SDL_PointInRect(const SDL_Point *p, const SDL_Rect *r)
{
    return (p && r &&
            p->x >= r->x && p->x < (r->x + r->w) &&
            p->y >= r->y && p->y < (r->y + r->h)) ? SDL_TRUE : SDL_FALSE;
}

SDL_FORCE_INLINE SDL_bool SDL_RectEmpty(const SDL_Rect *r)
{
    return (r == 0 || r->w <= 0 || r->h <= 0) ? SDL_TRUE : SDL_FALSE;
}

// ---------------------------------------------------------------------------
// Minimal pixel format stub (SDL_PixelFormat value used in some headers)
// ---------------------------------------------------------------------------
#define SDL_PIXELFORMAT_UNKNOWN 0

// ---------------------------------------------------------------------------
// Opaque SDL types (pointers only; tests do not need the full definitions)
// SDL_Texture exposes w/h for legacy SDL2-style code in ECS_Components.h.
// ---------------------------------------------------------------------------
typedef struct SDL_Texture   { int w = 0; int h = 0; }  SDL_Texture;
typedef struct SDL_Renderer  SDL_Renderer;
typedef struct SDL_Surface   SDL_Surface;
typedef struct SDL_Window    SDL_Window;

// ---------------------------------------------------------------------------
// SDL_Scancode — minimal enum needed by ECS_Components.h
// ---------------------------------------------------------------------------
typedef enum SDL_Scancode
{
    SDL_SCANCODE_UNKNOWN    = 0,
    SDL_SCANCODE_A          = 4,
    SDL_SCANCODE_D          = 7,
    SDL_SCANCODE_E          = 8,
    SDL_SCANCODE_S          = 22,
    SDL_SCANCODE_W          = 26,
    SDL_SCANCODE_SPACE      = 44,
    SDL_SCANCODE_TAB        = 43,
    SDL_SCANCODE_LCTRL      = 224,
    SDL_SCANCODE_RIGHT      = 79,
    SDL_SCANCODE_LEFT       = 80,
    SDL_SCANCODE_DOWN       = 81,
    SDL_SCANCODE_UP         = 82,
    SDL_SCANCODE_KP_1       = 89,
    SDL_SCANCODE_KP_2       = 90,
    SDL_SCANCODE_KP_3       = 91,
    SDL_SCANCODE_KP_4       = 92,
    SDL_SCANCODE_KP_5       = 93,
    SDL_SCANCODE_KP_6       = 94,
    SDL_SCANCODE_KP_7       = 95,
    SDL_SCANCODE_KP_8       = 96,
    SDL_SCANCODE_KP_9       = 97,
    SDL_SCANCODE_KP_DIVIDE  = 84,
    SDL_SCANCODE_KP_MULTIPLY= 85,
    SDL_SCANCODE_KP_MINUS   = 86,
    SDL_SCANCODE_KP_PLUS    = 87
} SDL_Scancode;

// ---------------------------------------------------------------------------
// SDL_JoystickID — used by ECS_Components.h and ECS_Systems.h
// ---------------------------------------------------------------------------
typedef Uint32 SDL_JoystickID;
