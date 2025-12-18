#include "GridPanel.h"

#include <imgui.h>

namespace Olympe
{
    void GridPanel::Draw(GridSettings_data& s)
    {
        if (!ImGui::Begin("Grid"))
        {
            ImGui::End();
            return;
        }

        ImGui::Checkbox("Enabled", &s.enabled);

        const char* projItems[] = {"Orthographic", "Isometric", "Hex (Axial)"};
        int proj = (int)s.projection;
        if (ImGui::Combo("Projection", &proj, projItems, 3))
            s.projection = (GridProjection)proj;

        if (s.projection == GridProjection::HexAxial)
        {
            ImGui::DragFloat("Hex radius", &s.hexRadius, 0.25f, 1.0f, 256.0f);
        }
        else
        {
            ImGui::DragFloat2("Cell size", &s.cellSize.x, 0.5f, 1.0f, 512.0f);
        }

        ImGui::ColorEdit4("Color", &s.color.x);
        ImGui::DragFloat("Line thickness", &s.lineThickness, 0.1f, 0.5f, 8.0f);
        ImGui::DragInt("Max lines", &s.maxLines, 1.0f, 16, 5000);

        ImGui::Separator();
        ImGui::Checkbox("Cull to camera", &s.cullToCamera);

        ImGui::Separator();
        ImGui::Checkbox("Draw behind scene", &s.drawBehindScene);
        ImGui::SameLine();
        ImGui::Checkbox("Draw overlay", &s.drawOverlay);

        ImGui::Separator();
        const char* sortItems[] = {"None", "Y", "Iso", "Hex"};
        int sort = (int)s.sortMode;
        if (ImGui::Combo("Entity sort", &sort, sortItems, 4))
            s.sortMode = (GridSortMode)sort;

        ImGui::End();
    }
}
