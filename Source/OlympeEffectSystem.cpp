/*
Olympe Engine V2 - 2025
Nicolas Chereau
nchereau@gmail.com

This file is part of Olympe Engine V2.

OlympeEffectSystem: Plasma bloom background effect system
Autonomous visual effect system (no entity required)
*/

#include "ECS_Systems.h"
#include "GameEngine.h"
#include "DataManager.h"
#include "system/CameraManager.h"
#include "drawing.h"
#include "vector.h"
#include <SDL3/SDL.h>
#include <cmath>
#include <cstdlib>

// Configuration constants
#define NUM_ORBS 4
#define BLUR_PASSES 3
#define ORB_BASE_RADIUS 120.0f
#define ORB_SPEED_MIN 15.0f
#define ORB_SPEED_MAX 35.0f

// Mathematical constants
#ifndef M_PI
#define M_PI 3.14159265358979323846f
#endif

// Blur effect constants
#define BLUR_ALPHA_CENTER 100
#define BLUR_ALPHA_OFFSET 50

// Bloom effect constants
#define BLOOM_ALPHA_BLUR 180
#define BLOOM_ALPHA_PLASMA 120
#define BLOOM_ALPHA_LOGO 255

// Structure for glowing orbs
struct GlowOrb
{
    Vector position;
    Vector velocity;
    SDL_Color color;
    float radius;
    float pulsePhase;
    float pulseSpeed;
    float pulseAmplitude;
};

//-------------------------------------------------------------
// Implementation structure (pimpl pattern)
//-------------------------------------------------------------
struct OlympeEffectSystem::Implementation
{
    // Textures
    SDL_Texture* backgroundTexture;
    SDL_Texture* plasmaTexture;
    SDL_Texture* blurTexture1;
    SDL_Texture* blurTexture2;
    SDL_Texture* bloomTexture;
    SDL_Texture* logoTexture;
    
    // Orbs
    GlowOrb orbs[NUM_ORBS];
    
    // Screen dimensions
    int width;
    int height;
    
    // Constructor
    Implementation()
        : backgroundTexture(nullptr)
        , plasmaTexture(nullptr)
        , blurTexture1(nullptr)
        , blurTexture2(nullptr)
        , bloomTexture(nullptr)
        , logoTexture(nullptr)
        , width(640)
        , height(320)
    {}
    
    // Helper methods
    void UpdateOrbs(float deltaTime);
    void RenderBackground();
    void RenderPlasmaOrbs();
    void ApplyGaussianBlur();
    void ApplyBloom();
    void BounceOffEdges(GlowOrb& orb);
    SDL_Color GeneratePlasmaColor(int index);
};

//-------------------------------------------------------------
OlympeEffectSystem::OlympeEffectSystem()
    : pImpl(new Implementation())
{
    // No required signature - autonomous system
}

//-------------------------------------------------------------
OlympeEffectSystem::~OlympeEffectSystem()
{
    // Clean up all textures
    if (pImpl)
    {
        if (pImpl->backgroundTexture) SDL_DestroyTexture(pImpl->backgroundTexture);
        if (pImpl->plasmaTexture) SDL_DestroyTexture(pImpl->plasmaTexture);
        if (pImpl->blurTexture1) SDL_DestroyTexture(pImpl->blurTexture1);
        if (pImpl->blurTexture2) SDL_DestroyTexture(pImpl->blurTexture2);
        if (pImpl->bloomTexture) SDL_DestroyTexture(pImpl->bloomTexture);
        // Don't destroy logoTexture - managed by DataManager
        delete pImpl;
    }
}

//-------------------------------------------------------------
void OlympeEffectSystem::Initialize()
{
    pImpl->width = GameEngine::screenWidth;
    pImpl->height = GameEngine::screenHeight;
    
    // Create render target textures
    pImpl->backgroundTexture = SDL_CreateTexture(GameEngine::renderer, SDL_PIXELFORMAT_RGBA8888, 
                                         SDL_TEXTUREACCESS_TARGET, pImpl->width, pImpl->height);
    pImpl->plasmaTexture = SDL_CreateTexture(GameEngine::renderer, SDL_PIXELFORMAT_RGBA8888, 
                                     SDL_TEXTUREACCESS_TARGET, pImpl->width, pImpl->height);
    pImpl->blurTexture1 = SDL_CreateTexture(GameEngine::renderer, SDL_PIXELFORMAT_RGBA8888, 
                                    SDL_TEXTUREACCESS_TARGET, pImpl->width, pImpl->height);
    pImpl->blurTexture2 = SDL_CreateTexture(GameEngine::renderer, SDL_PIXELFORMAT_RGBA8888, 
                                    SDL_TEXTUREACCESS_TARGET, pImpl->width, pImpl->height);
    pImpl->bloomTexture = SDL_CreateTexture(GameEngine::renderer, SDL_PIXELFORMAT_RGBA8888, 
                                    SDL_TEXTUREACCESS_TARGET, pImpl->width, pImpl->height);
    
    // Set blend modes
    SDL_SetTextureBlendMode(pImpl->plasmaTexture, SDL_BLENDMODE_BLEND);
    SDL_SetTextureBlendMode(pImpl->blurTexture1, SDL_BLENDMODE_BLEND);
    SDL_SetTextureBlendMode(pImpl->blurTexture2, SDL_BLENDMODE_BLEND);
    SDL_SetTextureBlendMode(pImpl->bloomTexture, SDL_BLENDMODE_BLEND);
    
    // Load logo texture
    pImpl->logoTexture = DataManager::Get().GetSprite("Olympe_Logo", "Resources/olympe_logo.png", ResourceCategory::GameEntity);
    if (!pImpl->logoTexture)
    {
        SYSTEM_LOG << "OlympeEffectSystem: Failed to load logo texture\n";
    }
    
    // Initialize orbs with random positions and velocities
    for (int i = 0; i < NUM_ORBS; ++i)
    {
        pImpl->orbs[i].position.x = SDL_randf() * static_cast<float>(pImpl->width);
        pImpl->orbs[i].position.y = SDL_randf() * static_cast<float>(pImpl->height);
        
        // Random velocity
        float speed = ORB_SPEED_MIN + SDL_randf() * (ORB_SPEED_MAX - ORB_SPEED_MIN);
        float angle = SDL_randf() * 2.0f * M_PI;
        pImpl->orbs[i].velocity.x = cosf(angle) * speed;
        pImpl->orbs[i].velocity.y = sinf(angle) * speed;
        
        // Generate plasma color
        pImpl->orbs[i].color = pImpl->GeneratePlasmaColor(i);
        
        // Pulse parameters
        pImpl->orbs[i].radius = ORB_BASE_RADIUS;
        pImpl->orbs[i].pulsePhase = SDL_randf() * 2.0f * M_PI;
        pImpl->orbs[i].pulseSpeed = 0.5f + SDL_randf() * 0.5f; // 0.5-1.0 Hz
        pImpl->orbs[i].pulseAmplitude = 0.15f + SDL_randf() * 0.10f; // 15-25%
    }
    
    SYSTEM_LOG << "OlympeEffectSystem initialized\n";
}

//-------------------------------------------------------------
SDL_Color OlympeEffectSystem::Implementation::GeneratePlasmaColor(int index)
{
    // Harmonious plasma colors: blue, violet, cyan, pink
    SDL_Color colors[NUM_ORBS] = {
        {60, 100, 255, 255},   // Blue
        {150, 80, 255, 255},   // Violet
        {80, 200, 255, 255},   // Cyan
        {255, 100, 200, 255}   // Pink
    };
    return colors[index % NUM_ORBS];
}

//-------------------------------------------------------------
void OlympeEffectSystem::Implementation::UpdateOrbs(float deltaTime)
{
    for (int i = 0; i < NUM_ORBS; ++i)
    {
        GlowOrb& orb = orbs[i];
        
        // Update position
        orb.position.x += orb.velocity.x * deltaTime;
        orb.position.y += orb.velocity.y * deltaTime;
        
        // Update pulse
        orb.pulsePhase += orb.pulseSpeed * 2.0f * M_PI * deltaTime;
        float pulseFactor = 1.0f + orb.pulseAmplitude * sinf(orb.pulsePhase);
        orb.radius = ORB_BASE_RADIUS * pulseFactor;
        
        // Bounce off edges
        BounceOffEdges(orb);
    }
}

//-------------------------------------------------------------
void OlympeEffectSystem::Implementation::BounceOffEdges(GlowOrb& orb)
{
    const float margin = orb.radius;
    
    // Left/Right edges
    if (orb.position.x < margin)
    {
        orb.position.x = margin;
        orb.velocity.x = fabsf(orb.velocity.x);
    }
    else if (orb.position.x > width - margin)
    {
        orb.position.x = width - margin;
        orb.velocity.x = -fabsf(orb.velocity.x);
    }
    
    // Top/Bottom edges
    if (orb.position.y < margin)
    {
        orb.position.y = margin;
        orb.velocity.y = fabsf(orb.velocity.y);
    }
    else if (orb.position.y > height - margin)
    {
        orb.position.y = height - margin;
        orb.velocity.y = -fabsf(orb.velocity.y);
    }
}

//-------------------------------------------------------------
void OlympeEffectSystem::Implementation::RenderBackground()
{
    SDL_SetRenderTarget(GameEngine::renderer, backgroundTexture);
    
    // Dark gradient background (black → night blue)
   /* for (int y = 0; y < height; ++y)
    {
        float t = static_cast<float>(y) / static_cast<float>(height);
        Uint8 r = static_cast<Uint8>(0 + t * 10);
        Uint8 g = static_cast<Uint8>(0 + t * 15);
        Uint8 b = static_cast<Uint8>(0 + t * 30);
        
        SDL_SetRenderDrawColor(GameEngine::renderer, r, g, b, 255);
        SDL_RenderLine(GameEngine::renderer, 0, y, width, y);
    }/**/
    SDL_SetRenderDrawColor(GameEngine::renderer, 230, 255, 230, 255);
	SDL_RenderFillRect(GameEngine::renderer, nullptr);

    SDL_SetRenderTarget(GameEngine::renderer, nullptr);
}

//-------------------------------------------------------------
void OlympeEffectSystem::Implementation::RenderPlasmaOrbs()
{
    SDL_SetRenderTarget(GameEngine::renderer, plasmaTexture);
    SDL_SetRenderDrawColor(GameEngine::renderer, 0, 0, 0, 255);
    SDL_RenderClear(GameEngine::renderer);
    
    // Render each orb as concentric circles with fading alpha
    for (int i = 0; i < NUM_ORBS; ++i)
    {
        const GlowOrb& orb = orbs[i];
        int cx = static_cast<int>(orb.position.x);
        int cy = static_cast<int>(orb.position.y);
        
        // Draw 30 concentric circles with decreasing alpha
        /*const int numCircles = 4;
        for (int j = 0; j < numCircles; ++j)
        {
            float t = static_cast<float>(j) / static_cast<float>(numCircles - 1);
            float radiusFactor = 1.0f - t;
            int currentRadius = static_cast<int>(orb.radius * (1.0f - radiusFactor * radiusFactor));
            
            // Quadratic falloff for smooth edges
            float alphaFactor = radiusFactor * radiusFactor;
            Uint8 alpha = static_cast<Uint8>(180.0f * alphaFactor);
            
            SDL_SetRenderDrawColor(GameEngine::renderer, orb.color.r, orb.color.g, orb.color.b, alpha);
            Draw_FilledCircle(GameEngine::renderer, cx, cy, currentRadius);
        }/**/
        // debug to disply the circles outline
        
        SDL_SetRenderDrawColor(GameEngine::renderer, 0, 255, 0, 255);
        Draw_Circle(GameEngine::renderer, cx, cy, (int)orb.radius);
    }
    
    SDL_SetRenderTarget(GameEngine::renderer, nullptr);
}

//-------------------------------------------------------------
void OlympeEffectSystem::Implementation::ApplyGaussianBlur()
{
    // Multi-pass gaussian blur
    for (int pass = 0; pass < BLUR_PASSES; ++pass)
    {
        int offset = (pass + 1) * 2; // 2, 4, 6 pixels
        
        // Horizontal blur pass: plasmaTexture → blurTexture1
        SDL_SetRenderTarget(GameEngine::renderer, blurTexture1);
        SDL_SetRenderDrawColor(GameEngine::renderer, 0, 0, 0, 0);
        SDL_RenderClear(GameEngine::renderer);
        
        SDL_SetTextureBlendMode(plasmaTexture, SDL_BLENDMODE_BLEND);
        SDL_SetTextureAlphaMod(plasmaTexture, BLUR_ALPHA_CENTER); // Center
        SDL_RenderTexture(GameEngine::renderer, plasmaTexture, nullptr, nullptr);
        
        SDL_SetTextureAlphaMod(plasmaTexture, BLUR_ALPHA_OFFSET); // Left offset
        SDL_FRect leftRect = {-static_cast<float>(offset), 0, static_cast<float>(width), static_cast<float>(height)};
        SDL_RenderTexture(GameEngine::renderer, plasmaTexture, nullptr, &leftRect);
        
        SDL_SetTextureAlphaMod(plasmaTexture, BLUR_ALPHA_OFFSET); // Right offset
        SDL_FRect rightRect = {static_cast<float>(offset), 0, static_cast<float>(width), static_cast<float>(height)};
        SDL_RenderTexture(GameEngine::renderer, plasmaTexture, nullptr, &rightRect);
        
        // Vertical blur pass: blurTexture1 → blurTexture2
        SDL_SetRenderTarget(GameEngine::renderer, blurTexture2);
        SDL_SetRenderDrawColor(GameEngine::renderer, 0, 0, 0, 0);
        SDL_RenderClear(GameEngine::renderer);
        
        SDL_SetTextureBlendMode(blurTexture1, SDL_BLENDMODE_BLEND);
        SDL_SetTextureAlphaMod(blurTexture1, BLUR_ALPHA_CENTER); // Center
        SDL_RenderTexture(GameEngine::renderer, blurTexture1, nullptr, nullptr);
        
        SDL_SetTextureAlphaMod(blurTexture1, BLUR_ALPHA_OFFSET); // Top offset
        SDL_FRect topRect = {0, -static_cast<float>(offset), static_cast<float>(width), static_cast<float>(height)};
        SDL_RenderTexture(GameEngine::renderer, blurTexture1, nullptr, &topRect);
        
        SDL_SetTextureAlphaMod(blurTexture1, BLUR_ALPHA_OFFSET); // Bottom offset
        SDL_FRect bottomRect = {0, static_cast<float>(offset), static_cast<float>(width), static_cast<float>(height)};
        SDL_RenderTexture(GameEngine::renderer, blurTexture1, nullptr, &bottomRect);
        
        // Copy back to plasmaTexture for next pass
        SDL_SetRenderTarget(GameEngine::renderer, plasmaTexture);
        SDL_SetTextureAlphaMod(blurTexture2, 255);
        SDL_SetTextureBlendMode(blurTexture2, SDL_BLENDMODE_NONE);
        SDL_RenderTexture(GameEngine::renderer, blurTexture2, nullptr, nullptr);
    }
    
    SDL_SetRenderTarget(GameEngine::renderer, nullptr);
}

//-------------------------------------------------------------
void OlympeEffectSystem::Implementation::ApplyBloom()
{
    SDL_SetRenderTarget(GameEngine::renderer, bloomTexture);
 
    // Step 1: Draw background (no blending)
    SDL_SetTextureBlendMode(backgroundTexture, SDL_BLENDMODE_NONE);
    SDL_RenderTexture(GameEngine::renderer, backgroundTexture, nullptr, nullptr);
   /* 
    // Step 2: Add blurred glow (additive blending)
    SDL_SetTextureBlendMode(blurTexture2, SDL_BLENDMODE_ADD);
    SDL_SetTextureAlphaMod(blurTexture2, BLOOM_ALPHA_BLUR);
    SDL_RenderTexture(GameEngine::renderer, blurTexture2, nullptr, nullptr);
    
    // Step 3: Add original plasma orbs (additive blending)
    SDL_SetTextureBlendMode(plasmaTexture, SDL_BLENDMODE_ADD);
    SDL_SetTextureAlphaMod(plasmaTexture, BLOOM_ALPHA_PLASMA);
    SDL_RenderTexture(GameEngine::renderer, plasmaTexture, nullptr, nullptr);
    /**/
    // Step 4: Render logo (optional)
    if (logoTexture)
    {
        SDL_SetTextureBlendMode(logoTexture, SDL_BLENDMODE_BLEND);
        SDL_SetTextureAlphaMod(logoTexture, BLOOM_ALPHA_LOGO);
        SDL_FRect logoRect = {(width - 300.0f) / 2.0f, (height - 121.0f) / 2.0f, 300.0f, 121.0f};
        SDL_RenderTexture(GameEngine::renderer, logoTexture, nullptr, &logoRect);
    }
    
    SDL_SetRenderTarget(GameEngine::renderer, nullptr);
}

//-------------------------------------------------------------
void OlympeEffectSystem::Process()
{
    // Update orbs
    pImpl->UpdateOrbs(GameEngine::fDt);
    
    // Render background gradient
    pImpl->RenderBackground();
    
    // Render plasma orbs
 //   pImpl->RenderPlasmaOrbs();
    
    // Apply gaussian blur
 //   pImpl->ApplyGaussianBlur();
    
    // Compose final bloom effect
    pImpl->ApplyBloom();
}

//-------------------------------------------------------------
void OlympeEffectSystem::Render()
{
    // Get camera offset for rendering
    Vector vPos = -CameraManager::Get().GetCameraPositionForActivePlayer();
    SDL_FRect destRect = {vPos.x, vPos.y, static_cast<float>(pImpl->width), static_cast<float>(pImpl->height)};
    
    // Render the final bloom composite to screen
    SDL_RenderTexture(GameEngine::renderer, pImpl->bloomTexture, nullptr, &destRect);
}
