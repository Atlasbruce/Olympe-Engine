#include "ECS_Systems_Rendering_Camera.h"

namespace Olympe
{
    glm::mat4 CameraTransform::ViewMatrix() const
    {
        glm::mat4 m(1.0f);
        m = glm::translate(m, glm::vec3(-position, 0.0f));
        m = glm::rotate(m, glm::radians(rotationDegrees), glm::vec3(0, 0, 1));
        m = glm::scale(m, glm::vec3(1.0f / zoom.x, 1.0f / zoom.y, 1.0f));
        return m;
    }

    glm::mat4 CameraTransform::ProjectionMatrix() const
    {
        glm::vec2 half = orthoHalfExtents;
        if (half.x <= 0.0f || half.y <= 0.0f)
        {
            // Derive half extents from viewport and zoom: assume 1 world unit == 1 pixel baseline.
            // Users can override by setting orthoHalfExtents.
            half = glm::vec2(viewportSize) * 0.5f;
        }

        const float l = -half.x;
        const float r =  half.x;
        const float b = -half.y;
        const float t =  half.y;
        return glm::ortho(l, r, b, t, -1.0f, 1.0f);
    }

    glm::mat4 CameraTransform::ViewProjectionMatrix() const
    {
        return ProjectionMatrix() * ViewMatrix();
    }

    glm::vec2 CameraTransform::WorldToScreen(const glm::vec2& world) const
    {
        glm::vec4 clip = ViewProjectionMatrix() * glm::vec4(world, 0.0f, 1.0f);
        glm::vec2 ndc = glm::vec2(clip) / clip.w;
        glm::vec2 screen;
        screen.x = (ndc.x * 0.5f + 0.5f) * float(viewportSize.x);
        screen.y = (1.0f - (ndc.y * 0.5f + 0.5f)) * float(viewportSize.y);
        return screen;
    }

    glm::vec2 CameraTransform::ScreenToWorld(const glm::vec2& screen) const
    {
        glm::vec2 ndc;
        ndc.x = (screen.x / float(viewportSize.x)) * 2.0f - 1.0f;
        ndc.y = -((screen.y / float(viewportSize.y)) * 2.0f - 1.0f);
        glm::mat4 inv = glm::inverse(ViewProjectionMatrix());
        glm::vec4 world = inv * glm::vec4(ndc, 0.0f, 1.0f);
        return glm::vec2(world) / world.w;
    }

    void CameraTransform::GetWorldAABB(glm::vec2& outMin, glm::vec2& outMax) const
    {
        // Compute AABB from 4 screen corners back-projected.
        const glm::vec2 corners[4] = {
            {0.0f, 0.0f},
            {float(viewportSize.x), 0.0f},
            {float(viewportSize.x), float(viewportSize.y)},
            {0.0f, float(viewportSize.y)}
        };

        outMin = glm::vec2(FLT_MAX);
        outMax = glm::vec2(-FLT_MAX);
        for (auto& c : corners)
        {
            glm::vec2 w = ScreenToWorld(c);
            outMin = glm::min(outMin, w);
            outMax = glm::max(outMax, w);
        }
    }
}
