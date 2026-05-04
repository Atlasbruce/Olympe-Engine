// Test runner that can be called from the ImGui menu
// Phase 2.1 Chunk 2 - E2E test for DocumentVersionManager + EntityPrefabEditorV2

#pragma once

#include "Chunk2_E2E_Test.h"
#include "../TabManager.h"
#include "../../system/system_utils.h"
#include <sstream>

namespace Olympe::Tests
{
    // Modal state for test result display
    static bool g_ShowChunk2TestModal = false;
    static std::string g_Chunk2TestResults = "";
    static bool g_Chunk2TestPassed = false;

    void ShowChunk2TestModalImGui()
    {
        if (!g_ShowChunk2TestModal)
            return;

        if (ImGui::BeginPopupModal("Chunk 2 E2E Test Results", &g_ShowChunk2TestModal, ImGuiWindowFlags_AlwaysAutoResize))
        {
            ImGui::TextUnformatted(g_Chunk2TestResults.c_str());
            ImGui::Separator();
            
            if (g_Chunk2TestPassed) {
                ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "✅ ALL TESTS PASSED");
            } else {
                ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "❌ TESTS FAILED");
            }

            if (ImGui::Button("OK", ImVec2(120, 0)))
            {
                g_ShowChunk2TestModal = false;
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }
    }

    void RunChunk2TestWithUI()
    {
        std::ostringstream oss;

        try {
            oss << "=== CHUNK 2 E2E TEST SUITE ===\n\n";
            oss << "Testing:\n";
            oss << "  1. DocumentVersionManager::CreateNewDocument()\n";
            oss << "  2. DocumentVersionManager::CreateRenderer()\n";
            oss << "  3. IGraphRenderer interface compliance\n";
            oss << "  4. Document persistence\n";
            oss << "  5. Strategy registration\n\n";

            // Run tests
            RunAllChunk2Tests();

            oss << "✅ SUCCESS - All tests passed!\n\n";
            oss << "Chunk 2 rendering pipeline is functional:\n";
            oss << "  ✓ DocumentVersionManager routing\n";
            oss << "  ✓ EntityPrefabGraphDocumentV2 creation\n";
            oss << "  ✓ EntityPrefabEditorV2 rendering\n";
            oss << "  ✓ IGraphRenderer interface\n";
            oss << "  ✓ Strategy registration\n\n";
            oss << "Ready for: Chunk 3 (TabManager Integration)";

            g_Chunk2TestPassed = true;
        }
        catch (const std::exception& e) {
            oss << "❌ FAILED\n\n";
            oss << "Exception: " << e.what() << "\n\n";
            oss << "Debugging information:\n";
            oss << "  • Check DocumentVersionManager::Get() is initialized\n";
            oss << "  • Check strategy registration completed\n";
            oss << "  • Check EntityPrefabGraphDocumentV2 creation\n";

            g_Chunk2TestPassed = false;
            SYSTEM_LOG << "[Chunk2Test] FAILED: " << e.what() << std::endl;
        }

        g_Chunk2TestResults = oss.str();
        g_ShowChunk2TestModal = true;

        // Also log to console
        if (g_Chunk2TestPassed) {
            SYSTEM_LOG << "[Chunk2Test] ✅ ALL TESTS PASSED" << std::endl;
        } else {
            SYSTEM_LOG << "[Chunk2Test] ❌ TESTS FAILED" << std::endl;
        }
    }

} // namespace Olympe::Tests
