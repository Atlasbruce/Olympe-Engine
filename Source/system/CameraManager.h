/*
 Olympe Engine V2 2025
 Nicolas Chereau
 nchereau@gmail.com

 Purpose: 
 - CameraManager class to handle multiple camera instances for players

*/
#pragma once

#include <SDL3/SDL.h>
#include <unordered_map>
#include "message.h"
#include "../object.h"
#include "../vector.h"
#include "../GameEngine.h"

enum class CameraType
{
    CameraType_2D = 0,
    CameraType_2_5D,
    CameraType_Isometric,
	CameraType_Count
};

enum class CameraMode
{
    CameraMode_Standard_Fixed = 0, // camera fixed at position on main renderer
	CameraMode_Standard_Follow,    // camera follows an entity (player or other) on main renderer
	CameraMode_Viewport_Fixed,      // camera fixed at position on a viewport
	CameraMode_Viewport_Follow,     // camera follows an entity (player or other) on a viewport
    CameraMode_Count
};

class CameraManager 
{
public:
    struct CameraInstance // To be changed into component data
    {
        short playerId = 0;
        Vector position;
        Vector offset = { -GameEngine::screenWidth / 2.f, -GameEngine::screenHeight / 2.f, 0.0f };
        float zoom = 1.0f;
        SDL_Rect bounds{INT_MIN, INT_MIN, INT_MAX, INT_MAX};
		bool followTargetEntity = false;
		uint64_t targetEntity = 0; // optional follow target EntityID
		CameraMode mode = CameraMode::CameraMode_Standard_Fixed;
		CameraType type = CameraType::CameraType_2D;
    };

public:
    CameraManager() { Initialize(); }
    ~CameraManager() { Shutdown(); }

    static CameraManager& GetInstance()
    {
        static CameraManager instance;
        return instance;
    }
    static CameraManager& Get() { return GetInstance(); }

    void Initialize();
    void Shutdown();

    // Backward-compatible single-apply: sets renderer viewport to first view and leaves it.
    void Apply(SDL_Renderer* renderer);

    void Apply(SDL_Renderer* renderer, short playerID);

    // Utility: create/remove camera instance for a player
    void CreateCameraForPlayer(short playerID);
    void RemoveCameraForPlayer(short playerID);

    // Get camera instance for a player. If not present returns default camera (player 0).
    CameraInstance GetCameraForPlayer(short playerID) const;

	Vector GetCameraPositionForActivePlayer(short playerID = 0) const;

	void Process(); // per-frame processing (if needed)

    // Handle incoming engine messages for camera control
    void OnEvent(const Message& msg);

	//update the camera rectangles according to the viewports
	void UpdateCameraRectsInstances();

    // D�finit quel joueur est "actif" pour le rendu actuel
    void SetActivePlayerID(short playerID) { m_activePlayerID = playerID; }
	short GetActivePlayerID() const { return m_activePlayerID; }

    // Set active camera from ECS Camera_data (for ECS-based rendering)
    void SetActiveCameraFromECS(const Camera_data& camera, short playerID);
    
    // Get camera position for a specific camera (from ECS or legacy)
    Vector GetCameraPosition(short playerID) const;

    // R�cup�re l'offset de la cam�ra active (� utiliser dans VisualComponent::Render)
    Vector GetActiveCameraOffset() const
    {
        if (m_cameraInstances.find(m_activePlayerID) != m_cameraInstances.end()) 
        {
            Vector _v = m_cameraInstances.at(m_activePlayerID).offset;
            _v -= m_cameraInstances.at(m_activePlayerID).position;
            return _v;
        }
        // Fallback sur la cam�ra 0 ou identit� si introuvable
        return m_cameraInstances.count(0) ? m_cameraInstances.at(0).offset : Vector(0, 0, 0);
    }

private:
    std::unordered_map<short, CameraInstance> m_cameraInstances;
	short m_activePlayerID = 0; // currently active player for camera and viewport rendering
};