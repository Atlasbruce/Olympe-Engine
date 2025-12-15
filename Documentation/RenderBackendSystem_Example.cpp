/*
Olympe Engine V2 - 2025
RenderBackendSystem Example Code

This file demonstrates how to use the RenderBackendSystem for
split-screen and multi-window rendering.

NOTE: This is example code for documentation purposes.
      Not meant to be compiled directly.
*/

#include "World.h"
#include "ECS_Systems.h"
#include "ECS_Components.h"

//=============================================================================
// Example 1: Simple 2-Player Split-Screen Setup
//=============================================================================
void Example1_TwoPlayerSplitScreen()
{
    // Get the render backend system
    RenderBackendSystem* renderBackend = World::Get().GetSystem<RenderBackendSystem>();
    
    if (!renderBackend)
    {
        SYSTEM_LOG << "RenderBackendSystem not available\n";
        return;
    }
    
    // Setup split-screen for 2 players (vertical split)
    std::vector<EntityID> viewports = renderBackend->SetupSplitScreen(2);
    
    // Create player entities with cameras
    for (int i = 0; i < 2; ++i)
    {
        // Create player entity
        EntityID player = World::Get().CreateEntity();
        Position_data& pos = World::Get().AddComponent<Position_data>(player);
        pos.position = Vector(100.f * i, 100.f, 0.f);
        
        // Add player components
        World::Get().AddComponent<PlayerBinding_data>(player).playerIndex = i;
        World::Get().AddComponent<PlayerController_data>(player);
        World::Get().AddComponent<Controller_data>(player);
        
        // Create camera for this player
        EntityID camera = World::Get().CreateEntity();
        Camera_data& cam = World::Get().AddComponent<Camera_data>(camera);
        cam.targetEntity = player;        // Camera follows player
        cam.viewportEntity = viewports[i]; // Camera renders to this viewport
        cam.zoomLevel = 1.0f;
        
        // Link camera to viewport
        Viewport_data& vp = World::Get().GetComponent<Viewport_data>(viewports[i]);
        vp.cameraEntity = camera;
        
        SYSTEM_LOG << "Player " << i << " setup complete\n";
    }
}

//=============================================================================
// Example 2: 4-Player Split-Screen with Custom Cameras
//=============================================================================
void Example2_FourPlayerSplitScreen()
{
    RenderBackendSystem* renderBackend = World::Get().GetSystem<RenderBackendSystem>();
    
    // Setup 4-player split-screen (2x2 grid)
    auto viewports = renderBackend->SetupSplitScreen(4);
    
    // Different zoom levels per player
    float zoomLevels[4] = {1.0f, 1.5f, 0.75f, 1.25f};
    
    for (int i = 0; i < 4; ++i)
    {
        // Create player
        EntityID player = World::Get().CreateEntity();
        World::Get().AddComponent<Position_data>(player).position = Vector(i * 200.f, i * 200.f, 0.f);
        World::Get().AddComponent<PlayerBinding_data>(player).playerIndex = i;
        World::Get().AddComponent<BoundingBox_data>(player);
        
        // Create camera with custom zoom
        EntityID camera = World::Get().CreateEntity();
        Camera_data& cam = World::Get().AddComponent<Camera_data>(camera);
        cam.targetEntity = player;
        cam.viewportEntity = viewports[i];
        cam.zoomLevel = zoomLevels[i];
        cam.offset = Vector(0.f, -50.f, 0.f); // Slightly offset upward
        
        // Link camera to viewport
        Viewport_data& vp = World::Get().GetComponent<Viewport_data>(viewports[i]);
        vp.cameraEntity = camera;
    }
}

//=============================================================================
// Example 3: Multi-Window Mode (Separate Windows Per Player)
//=============================================================================
void Example3_MultiWindow()
{
    RenderBackendSystem* renderBackend = World::Get().GetSystem<RenderBackendSystem>();
    
    // Create 2 separate windows (800x600 each)
    auto windowsAndViewports = renderBackend->SetupMultiWindow(2, 800, 600);
    
    for (size_t i = 0; i < windowsAndViewports.size(); ++i)
    {
        EntityID rtEntity = windowsAndViewports[i].first;
        EntityID vpEntity = windowsAndViewports[i].second;
        
        // Create player
        EntityID player = World::Get().CreateEntity();
        World::Get().AddComponent<Position_data>(player);
        World::Get().AddComponent<PlayerBinding_data>(player).playerIndex = static_cast<short>(i);
        
        // Create camera
        EntityID camera = World::Get().CreateEntity();
        Camera_data& cam = World::Get().AddComponent<Camera_data>(camera);
        cam.targetEntity = player;
        cam.viewportEntity = vpEntity;
        
        // Link camera to viewport
        Viewport_data& vp = World::Get().GetComponent<Viewport_data>(vpEntity);
        vp.cameraEntity = camera;
        
        SYSTEM_LOG << "Window " << i << " created for player " << i << "\n";
    }
}

//=============================================================================
// Example 4: Dynamic Player Addition/Removal
//=============================================================================
class MultiplayerManager
{
private:
    std::vector<EntityID> players;
    std::vector<EntityID> cameras;
    std::vector<EntityID> viewports;
    bool isMultiWindow;
    
public:
    MultiplayerManager() : isMultiWindow(false) {}
    
    void AddPlayer()
    {
        RenderBackendSystem* renderBackend = World::Get().GetSystem<RenderBackendSystem>();
        int newPlayerCount = static_cast<int>(players.size()) + 1;
        
        if (newPlayerCount > 8)
        {
            SYSTEM_LOG << "Maximum players (8) reached\n";
            return;
        }
        
        // Rebuild viewports with new player count
        if (isMultiWindow)
        {
            auto result = renderBackend->SetupMultiWindow(newPlayerCount, 800, 600);
            viewports.clear();
            for (const auto& pair : result)
            {
                viewports.push_back(pair.second);
            }
        }
        else
        {
            viewports = renderBackend->SetupSplitScreen(newPlayerCount);
        }
        
        // Create new player entity
        int playerIndex = newPlayerCount - 1;
        EntityID player = World::Get().CreateEntity();
        World::Get().AddComponent<Position_data>(player);
        World::Get().AddComponent<PlayerBinding_data>(player).playerIndex = playerIndex;
        players.push_back(player);
        
        // Create camera for new player
        EntityID camera = World::Get().CreateEntity();
        Camera_data& cam = World::Get().AddComponent<Camera_data>(camera);
        cam.targetEntity = player;
        cam.viewportEntity = viewports[playerIndex];
        cameras.push_back(camera);
        
        // Link camera to viewport
        Viewport_data& vp = World::Get().GetComponent<Viewport_data>(viewports[playerIndex]);
        vp.cameraEntity = camera;
        
        SYSTEM_LOG << "Player " << playerIndex << " added. Total players: " << newPlayerCount << "\n";
    }
    
    void RemovePlayer()
    {
        if (players.empty())
        {
            SYSTEM_LOG << "No players to remove\n";
            return;
        }
        
        // Destroy last player and camera
        World::Get().DestroyEntity(players.back());
        World::Get().DestroyEntity(cameras.back());
        players.pop_back();
        cameras.pop_back();
        
        int newPlayerCount = static_cast<int>(players.size());
        
        if (newPlayerCount == 0)
        {
            // Clear everything
            RenderBackendSystem* renderBackend = World::Get().GetSystem<RenderBackendSystem>();
            renderBackend->ClearAllViewportsAndTargets();
            viewports.clear();
        }
        else
        {
            // Rebuild viewports
            RenderBackendSystem* renderBackend = World::Get().GetSystem<RenderBackendSystem>();
            if (isMultiWindow)
            {
                auto result = renderBackend->SetupMultiWindow(newPlayerCount, 800, 600);
                viewports.clear();
                for (const auto& pair : result)
                {
                    viewports.push_back(pair.second);
                }
            }
            else
            {
                viewports = renderBackend->SetupSplitScreen(newPlayerCount);
            }
            
            // Re-link remaining cameras to new viewports
            for (size_t i = 0; i < cameras.size(); ++i)
            {
                Camera_data& cam = World::Get().GetComponent<Camera_data>(cameras[i]);
                cam.viewportEntity = viewports[i];
                
                Viewport_data& vp = World::Get().GetComponent<Viewport_data>(viewports[i]);
                vp.cameraEntity = cameras[i];
            }
        }
        
        SYSTEM_LOG << "Player removed. Total players: " << newPlayerCount << "\n";
    }
    
    void ToggleDisplayMode()
    {
        isMultiWindow = !isMultiWindow;
        int playerCount = static_cast<int>(players.size());
        
        if (playerCount == 0)
            return;
        
        RenderBackendSystem* renderBackend = World::Get().GetSystem<RenderBackendSystem>();
        
        if (isMultiWindow)
        {
            auto result = renderBackend->SetupMultiWindow(playerCount, 800, 600);
            viewports.clear();
            for (const auto& pair : result)
            {
                viewports.push_back(pair.second);
            }
            SYSTEM_LOG << "Switched to multi-window mode\n";
        }
        else
        {
            viewports = renderBackend->SetupSplitScreen(playerCount);
            SYSTEM_LOG << "Switched to split-screen mode\n";
        }
        
        // Re-link cameras
        for (size_t i = 0; i < cameras.size() && i < viewports.size(); ++i)
        {
            Camera_data& cam = World::Get().GetComponent<Camera_data>(cameras[i]);
            cam.viewportEntity = viewports[i];
            
            Viewport_data& vp = World::Get().GetComponent<Viewport_data>(viewports[i]);
            vp.cameraEntity = cameras[i];
        }
    }
};

//=============================================================================
// Example 5: Advanced - Custom Viewport Layout
//=============================================================================
void Example5_CustomLayout()
{
    RenderBackendSystem* renderBackend = World::Get().GetSystem<RenderBackendSystem>();
    
    // Get primary render target
    auto targets = renderBackend->GetActiveRenderTargets();
    if (targets.empty())
        return;
        
    EntityID primaryTarget = targets[0];
    const RenderTarget_data& target = World::Get().GetComponent<RenderTarget_data>(primaryTarget);
    
    // Create custom layout: Large main view + 2 smaller picture-in-picture views
    float w = static_cast<float>(target.width);
    float h = static_cast<float>(target.height);
    
    // Main viewport (player 0) - full screen
    SDL_FRect mainRect = {0.f, 0.f, w, h};
    EntityID mainViewport = renderBackend->CreateViewport(0, mainRect, primaryTarget);
    
    // PIP viewport 1 (player 1) - top right corner
    SDL_FRect pip1Rect = {w * 0.7f, 0.f, w * 0.3f, h * 0.3f};
    EntityID pip1Viewport = renderBackend->CreateViewport(1, pip1Rect, primaryTarget);
    
    // PIP viewport 2 (player 2) - bottom right corner
    SDL_FRect pip2Rect = {w * 0.7f, h * 0.7f, w * 0.3f, h * 0.3f};
    EntityID pip2Viewport = renderBackend->CreateViewport(2, pip2Rect, primaryTarget);
    
    // Set render order (main view first, then PIPs on top)
    World::Get().GetComponent<Viewport_data>(mainViewport).renderOrder = 0;
    World::Get().GetComponent<Viewport_data>(pip1Viewport).renderOrder = 1;
    World::Get().GetComponent<Viewport_data>(pip2Viewport).renderOrder = 2;
    
    SYSTEM_LOG << "Custom layout created: 1 main + 2 PIP viewports\n";
}

//=============================================================================
// Example 6: Integration with Game Events
//=============================================================================
void OnPlayerJoined(int playerIndex)
{
    static MultiplayerManager manager;
    manager.AddPlayer();
    
    // Game continues seamlessly
    // No need to restart or reload
}

void OnPlayerLeft(int playerIndex)
{
    static MultiplayerManager manager;
    manager.RemovePlayer();
}

void OnDisplayModeToggle()
{
    static MultiplayerManager manager;
    manager.ToggleDisplayMode();
}

//=============================================================================
// Usage in Main Game Loop
//=============================================================================
void GameInitialization()
{
    // The RenderBackendSystem is automatically initialized by World
    // Primary render target is created in SDL_AppInit
    
    // For simple 2-player game:
    Example1_TwoPlayerSplitScreen();
    
    // Or for 4-player:
    // Example2_FourPlayerSplitScreen();
    
    // Or for multi-window:
    // Example3_MultiWindow();
}

void GameUpdate(float deltaTime)
{
    // Process input, update entities, etc.
    // RenderBackendSystem automatically handles all rendering
    
    // Example: Toggle display mode on F1 key
    static bool f1WasPressed = false;
    if (/* F1 key pressed */ false)
    {
        if (!f1WasPressed)
        {
            OnDisplayModeToggle();
            f1WasPressed = true;
        }
    }
    else
    {
        f1WasPressed = false;
    }
}
