#pragma once

#include "ECS_Grid.h"
#include "ECS_Systems_Rendering_Camera.h"

namespace Olympe
{
    class RenderingSystem;

    // Responsible for generating debug grid line primitives and submitting them to renderer.
    class GridSystem
    {
    public:
        void RenderGrid(const CameraTransform& camera, const GridSettings_data& settings, RenderingSystem& rendering);

    private:
        void RenderOrtho(const CameraTransform& camera, const GridSettings_data& settings, RenderingSystem& rendering);
        void RenderIso(const CameraTransform& camera, const GridSettings_data& settings, RenderingSystem& rendering);
        void RenderHex(const CameraTransform& camera, const GridSettings_data& settings, RenderingSystem& rendering);
    };
}
