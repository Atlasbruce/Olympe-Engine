/*
    Olympe Engine - 2025
    Nicolas Chereau
    nchereau@gmail.com

    This file is part of Olympe Engine.
    Purpose: Singleton to track the currently active camera during rendering.
             This allows drawing functions to automatically apply camera transforms
             without requiring CameraTransform parameters in every function call.
*/

#pragma once

#include "ECS_Systems.h"

/// Singleton to track the currently active camera during rendering
/// This allows drawing functions to automatically apply camera transforms
/// without requiring CameraTransform parameters in every function call
class RenderContext
{
public:
    static RenderContext& Get() {
        static RenderContext instance;
        return instance;
    }

    /// Set the active camera for the current rendering pass
    /// Call this at the start of rendering for each player/viewport
    void SetActiveCamera(const CameraTransform& cam) {
        activeCamera_ = cam;
        hasCameraSet_ = true;
    }

    /// Get the currently active camera
    /// Returns identity transform (no transformation) if none is set
    const CameraTransform& GetActiveCamera() const {
        if (!hasCameraSet_) {
            // Return default identity transform (screen space rendering)
            return identityCamera_;
        }
        return activeCamera_;
    }

    /// Check if a camera is currently active
    bool HasActiveCamera() const {
        return hasCameraSet_;
    }

    /// Clear the active camera (e.g., at end of rendering pass)
    void ClearActiveCamera() {
        hasCameraSet_ = false;
    }

private:
    RenderContext() : hasCameraSet_(false) {
        // Initialize identity camera once (for screen-space rendering)
        identityCamera_.worldPosition = Vector(0.f, 0.f, 0.f);
        identityCamera_.screenOffset = Vector(0.f, 0.f, 0.f);
        identityCamera_.zoom = 1.0f;
        identityCamera_.rotation = 0.0f;
        identityCamera_.viewport = {0.f, 0.f, 1920.f, 1080.f};
        identityCamera_.isActive = false;
    }
    ~RenderContext() = default;

    CameraTransform activeCamera_;
    CameraTransform identityCamera_;  // Pre-initialized identity transform
    bool hasCameraSet_;

    // Prevent copying
    RenderContext(const RenderContext&) = delete;
    RenderContext& operator=(const RenderContext&) = delete;
};
