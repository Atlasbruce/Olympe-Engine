#include "ECS_Systems_Grid.h"

#include <algorithm>
#include <cmath>

namespace Olympe
{
    // This file depends on RenderingSystem having a way to submit debug lines.
    // If your engine uses a different API, adapt SubmitDebugLine() accordingly.

    void GridSystem::RenderGrid(const CameraTransform& camera, const GridSettings_data& settings, RenderingSystem& rendering)
    {
        if (!settings.enabled) return;

        switch (settings.projection)
        {
        case GridProjection::Orthographic: RenderOrtho(camera, settings, rendering); break;
        case GridProjection::Isometric:    RenderIso(camera, settings, rendering); break;
        case GridProjection::HexAxial:     RenderHex(camera, settings, rendering); break;
        default: RenderOrtho(camera, settings, rendering); break;
        }
    }

    static inline void GetCameraAABB(const CameraTransform& cam, glm::vec2& mn, glm::vec2& mx)
    {
        cam.GetWorldAABB(mn, mx);
        // Expand a bit for line thickness
        mn -= glm::vec2(2.0f);
        mx += glm::vec2(2.0f);
    }

    void GridSystem::RenderOrtho(const CameraTransform& camera, const GridSettings_data& s, RenderingSystem& rendering)
    {
        glm::vec2 mn(-1000.0f), mx(1000.0f);
        if (s.cullToCamera) GetCameraAABB(camera, mn, mx);

        float csx = std::max(1e-3f, s.cellSize.x);
        float csy = std::max(1e-3f, s.cellSize.y);

        int lines = 0;

        float startX = std::floor(mn.x / csx) * csx;
        float endX   = std::ceil(mx.x / csx) * csx;
        for (float x = startX; x <= endX && lines < s.maxLines; x += csx)
        {
            rendering.SubmitDebugLine({x, mn.y}, {x, mx.y}, s.color, s.lineThickness, s.drawBehindScene, s.drawOverlay);
            ++lines;
        }

        float startY = std::floor(mn.y / csy) * csy;
        float endY   = std::ceil(mx.y / csy) * csy;
        for (float y = startY; y <= endY && lines < s.maxLines; y += csy)
        {
            rendering.SubmitDebugLine({mn.x, y}, {mx.x, y}, s.color, s.lineThickness, s.drawBehindScene, s.drawOverlay);
            ++lines;
        }
    }

    void GridSystem::RenderIso(const CameraTransform& camera, const GridSettings_data& s, RenderingSystem& rendering)
    {
        glm::vec2 mn(-1000.0f), mx(1000.0f);
        if (s.cullToCamera) GetCameraAABB(camera, mn, mx);

        const float w = std::max(1e-3f, s.cellSize.x);
        const float h = std::max(1e-3f, s.cellSize.y);

        // For iso diamonds, basis vectors
        const glm::vec2 u(w * 0.5f, -h * 0.5f);
        const glm::vec2 v(w * 0.5f,  h * 0.5f);

        // Choose a conservative range based on AABB.
        const float span = std::max(mx.x - mn.x, mx.y - mn.y);
        int range = int(std::ceil(span / std::min(w, h))) + 4;

        int lines = 0;
        for (int i = -range; i <= range && lines < s.maxLines; ++i)
        {
            // Lines parallel to u: p = i*v + t*u
            glm::vec2 p0 = glm::vec2(0) + float(i) * v - float(range) * u;
            glm::vec2 p1 = glm::vec2(0) + float(i) * v + float(range) * u;
            rendering.SubmitDebugLine(p0, p1, s.color, s.lineThickness, s.drawBehindScene, s.drawOverlay);
            ++lines;

            if (lines >= s.maxLines) break;
            // Lines parallel to v
            p0 = glm::vec2(0) + float(i) * u - float(range) * v;
            p1 = glm::vec2(0) + float(i) * u + float(range) * v;
            rendering.SubmitDebugLine(p0, p1, s.color, s.lineThickness, s.drawBehindScene, s.drawOverlay);
            ++lines;
        }
    }

    void GridSystem::RenderHex(const CameraTransform& camera, const GridSettings_data& s, RenderingSystem& rendering)
    {
        glm::vec2 mn(-1000.0f), mx(1000.0f);
        if (s.cullToCamera) GetCameraAABB(camera, mn, mx);

        const float r = std::max(1e-3f, s.hexRadius);
        const float dx = r * 1.5f;
        const float dy = r * std::sqrt(3.0f);

        // Estimate visible axial bounds
        int qMin = int(std::floor(mn.x / dx)) - 2;
        int qMax = int(std::ceil(mx.x / dx)) + 2;
        int rMin = int(std::floor(mn.y / dy)) - 2;
        int rMax = int(std::ceil(mx.y / dy)) + 2;

        auto hexCenter = [&](int q, int rr)
        {
            float x = dx * float(q);
            float y = dy * (float(rr) + 0.5f * float(q & 1));
            return glm::vec2(x, y);
        };

        auto drawHex = [&](const glm::vec2& c)
        {
            glm::vec2 pts[6];
            for (int i = 0; i < 6; ++i)
            {
                float a = glm::radians(60.0f * float(i) + 30.0f);
                pts[i] = c + glm::vec2(std::cos(a), std::sin(a)) * r;
            }
            for (int i = 0; i < 6; ++i)
            {
                rendering.SubmitDebugLine(pts[i], pts[(i + 1) % 6], s.color, s.lineThickness, s.drawBehindScene, s.drawOverlay);
            }
        };

        int cells = 0;
        const int maxCells = std::max(1, s.maxLines / 6);
        for (int q = qMin; q <= qMax && cells < maxCells; ++q)
        {
            for (int rr = rMin; rr <= rMax && cells < maxCells; ++rr)
            {
                glm::vec2 c = hexCenter(q, rr);
                if (s.cullToCamera)
                {
                    if (c.x + r < mn.x || c.x - r > mx.x || c.y + r < mn.y || c.y - r > mx.y)
                        continue;
                }
                drawHex(c);
                ++cells;
            }
        }
    }
}
