#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace Olympe
{
    // Camera transform and projection settings used by rendering + culling.
    // Intentionally header-only interface; implementation lives in ECS_Systems_Rendering_Camera.cpp
    struct CameraTransform
    {
        glm::vec2 position{0.0f, 0.0f};
        float rotationDegrees{0.0f};
        glm::vec2 zoom{1.0f, 1.0f};

        // viewport in pixels
        glm::ivec2 viewportSize{1280, 720};

        // Ortho extents in world units (half extents). If zero, derived from viewport/zoom.
        glm::vec2 orthoHalfExtents{0.0f, 0.0f};

        glm::mat4 ViewMatrix() const;
        glm::mat4 ProjectionMatrix() const;
        glm::mat4 ViewProjectionMatrix() const;

        glm::vec2 WorldToScreen(const glm::vec2& world) const;
        glm::vec2 ScreenToWorld(const glm::vec2& screen) const;

        // Axis-aligned world rect visible by this camera (min,max)
        void GetWorldAABB(glm::vec2& outMin, glm::vec2& outMax) const;
    };
}
