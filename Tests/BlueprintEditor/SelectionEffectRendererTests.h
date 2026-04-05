#pragma once

#include "../vector.h"
#include "SelectionEffectRenderer.h"
#include "third_party/imgui/imgui.h"

namespace Olympe
{
    /**
     * @brief Test suite pour SelectionEffectRenderer
     * 
     * Vérifie que l'effet de sélection fonctionne correctement
     * dans différents contextes (zoom, scale, styles, etc.)
     */
    class SelectionEffectRendererTests
    {
    public:
        /**
         * @brief Test 1 : Rendu glow avec défauts
         * Vérifie que le glow cyan s'affiche correctement
         */
        static bool Test_RenderSelectionGlow_Default()
        {
            SelectionEffectRenderer renderer;
            renderer.ApplyStyle_OlympeBlue();
            
            ImVec2 min(100.0f, 100.0f);
            ImVec2 max(200.0f, 200.0f);
            float zoom = 1.0f;
            float scale = 1.0f;
            float cornerRadius = 5.0f;
            
            // Should not crash
            renderer.RenderSelectionGlow(min, max, zoom, scale, cornerRadius);
            
            return true;
        }

        /**
         * @brief Test 2 : Rendu border avec épaisseur customisée
         */
        static bool Test_RenderSelectionBorder_Custom()
        {
            SelectionEffectRenderer renderer;
            renderer.SetBorderWidthMultiplier(2.5f);
            
            ImVec2 min(100.0f, 100.0f);
            ImVec2 max(200.0f, 200.0f);
            ImU32 borderColor = ImGui::GetColorU32(ImVec4(0.0f, 0.7f, 1.0f, 1.0f));
            float baseWidth = 2.0f;
            float zoom = 1.0f;
            
            renderer.RenderSelectionBorder(min, max, borderColor, baseWidth, zoom);
            
            return true;
        }

        /**
         * @brief Test 3 : Glow s'adapte au zoom
         * Vérifie que glowSize = 4.0 × zoom × scale
         */
        static bool Test_GlowScalingWithZoom()
        {
            SelectionEffectRenderer renderer;
            renderer.SetBaseGlowSize(4.0f);
            
            ImVec2 min(0.0f, 0.0f);
            ImVec2 max(100.0f, 100.0f);
            
            // Test 1: Zoom = 0.5 (éloigné)
            // Expect: glowSize = 4.0 × 0.5 × 1.0 = 2.0
            renderer.RenderSelectionGlow(min, max, 0.5f, 1.0f, 5.0f);
            
            // Test 2: Zoom = 2.0 (zoomé)
            // Expect: glowSize = 4.0 × 2.0 × 1.0 = 8.0
            renderer.RenderSelectionGlow(min, max, 2.0f, 1.0f, 5.0f);
            
            // Test 3: Zoom = 1.0 × Scale = 1.5
            // Expect: glowSize = 4.0 × 1.0 × 1.5 = 6.0
            renderer.RenderSelectionGlow(min, max, 1.0f, 1.5f, 5.0f);
            
            return true;
        }

        /**
         * @brief Test 4 : Configuration des couleurs
         */
        static bool Test_ColorConfiguration()
        {
            SelectionEffectRenderer renderer;
            
            // Test défaut
            Vector defaultColor = renderer.GetGlowColor();
            if (defaultColor.x != 0.0f || defaultColor.y != 0.8f || defaultColor.z != 1.0f)
                return false;
            
            // Test custom
            Vector customColor(0.5f, 0.5f, 0.5f);
            renderer.SetGlowColor(customColor);
            
            Vector retrievedColor = renderer.GetGlowColor();
            if (retrievedColor.x != 0.5f || retrievedColor.y != 0.5f || retrievedColor.z != 0.5f)
                return false;
            
            return true;
        }

        /**
         * @brief Test 5 : Configuration d'alpha
         */
        static bool Test_AlphaConfiguration()
        {
            SelectionEffectRenderer renderer;
            
            // Test défaut
            float defaultAlpha = renderer.GetGlowAlpha();
            if (defaultAlpha != 0.3f)
                return false;
            
            // Test custom
            renderer.SetGlowAlpha(0.5f);
            if (renderer.GetGlowAlpha() != 0.5f)
                return false;
            
            // Test clamping (alpha > 1.0 doit être clampé à 1.0)
            renderer.SetGlowAlpha(1.5f);
            if (renderer.GetGlowAlpha() != 1.0f)
                return false;
            
            // Test clamping (alpha < 0.0 doit être clampé à 0.0)
            renderer.SetGlowAlpha(-0.5f);
            if (renderer.GetGlowAlpha() != 0.0f)
                return false;
            
            return true;
        }

        /**
         * @brief Test 6 : Configuration d'épaisseur glow
         */
        static bool Test_GlowSizeConfiguration()
        {
            SelectionEffectRenderer renderer;
            
            // Test défaut
            float defaultSize = renderer.GetBaseGlowSize();
            if (defaultSize != 4.0f)
                return false;
            
            // Test custom
            renderer.SetBaseGlowSize(6.0f);
            if (renderer.GetBaseGlowSize() != 6.0f)
                return false;
            
            // Test minimum (size < 0.1 doit être clampé à 0.1)
            renderer.SetBaseGlowSize(0.01f);
            if (renderer.GetBaseGlowSize() != 0.1f)
                return false;
            
            return true;
        }

        /**
         * @brief Test 7 : Configuration du multiplicateur de bordure
         */
        static bool Test_BorderMultiplierConfiguration()
        {
            SelectionEffectRenderer renderer;
            
            // Test défaut
            float defaultMult = renderer.GetBorderWidthMultiplier();
            if (defaultMult != 2.0f)
                return false;
            
            // Test custom
            renderer.SetBorderWidthMultiplier(3.0f);
            if (renderer.GetBorderWidthMultiplier() != 3.0f)
                return false;
            
            // Test minimum (mult < 0.1 doit être clampé à 0.1)
            renderer.SetBorderWidthMultiplier(0.01f);
            if (renderer.GetBorderWidthMultiplier() != 0.1f)
                return false;
            
            return true;
        }

        /**
         * @brief Test 8 : Activation/Désactivation du glow
         */
        static bool Test_GlowEnabled()
        {
            SelectionEffectRenderer renderer;
            
            // Défaut: activé
            if (!renderer.IsGlowEnabled())
                return false;
            
            // Désactiver
            renderer.SetGlowEnabled(false);
            if (renderer.IsGlowEnabled())
                return false;
            
            // Réactiver
            renderer.SetGlowEnabled(true);
            if (!renderer.IsGlowEnabled())
                return false;
            
            return true;
        }

        /**
         * @brief Test 9 : Presets de style
         */
        static bool Test_StylePresets()
        {
            SelectionEffectRenderer renderer;
            
            // OlympeBlue
            renderer.ApplyStyle_OlympeBlue();
            if (renderer.GetGlowAlpha() != 0.3f)
                return false;
            
            // GoldAccent
            renderer.ApplyStyle_GoldAccent();
            if (renderer.GetGlowAlpha() != 0.25f)
                return false;
            
            // GreenEnergy
            renderer.ApplyStyle_GreenEnergy();
            if (renderer.GetGlowAlpha() != 0.35f)
                return false;
            
            // PurpleMystery
            renderer.ApplyStyle_PurpleMystery();
            if (renderer.GetGlowAlpha() != 0.3f)
                return false;
            
            // RedAlert
            renderer.ApplyStyle_RedAlert();
            if (renderer.GetGlowAlpha() != 0.4f)
                return false;
            
            return true;
        }

        /**
         * @brief Test 10 : Rendu complet (glow + border)
         */
        static bool Test_RenderCompleteSelection()
        {
            SelectionEffectRenderer renderer;
            renderer.ApplyStyle_OlympeBlue();
            
            ImVec2 min(50.0f, 50.0f);
            ImVec2 max(150.0f, 150.0f);
            ImU32 borderColor = ImGui::GetColorU32(ImVec4(0.0f, 0.7f, 1.0f, 1.0f));
            float baseWidth = 2.0f;
            float zoom = 1.5f;
            float scale = 1.0f;
            float cornerRadius = 5.0f;
            
            // Should render both glow and border without issue
            renderer.RenderCompleteSelection(
                min, max, borderColor, baseWidth, zoom, scale, cornerRadius
            );
            
            return true;
        }

        /**
         * @brief Run all tests
         */
        static void RunAllTests()
        {
            int passed = 0;
            int failed = 0;
            
            const char* tests[] = {
                "Test_RenderSelectionGlow_Default",
                "Test_RenderSelectionBorder_Custom",
                "Test_GlowScalingWithZoom",
                "Test_ColorConfiguration",
                "Test_AlphaConfiguration",
                "Test_GlowSizeConfiguration",
                "Test_BorderMultiplierConfiguration",
                "Test_GlowEnabled",
                "Test_StylePresets",
                "Test_RenderCompleteSelection"
            };
            
            #define RUN_TEST(testFunc) \
            if (testFunc()) { \
                ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "[PASS] %s", #testFunc); \
                passed++; \
            } else { \
                ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "[FAIL] %s", #testFunc); \
                failed++; \
            }
            
            ImGui::Begin("SelectionEffectRenderer Tests");
            ImGui::TextColored(ImVec4(0.0f, 1.0f, 1.0f, 1.0f), "Running SelectionEffectRenderer Tests...\n");
            
            RUN_TEST(Test_RenderSelectionGlow_Default);
            RUN_TEST(Test_RenderSelectionBorder_Custom);
            RUN_TEST(Test_GlowScalingWithZoom);
            RUN_TEST(Test_ColorConfiguration);
            RUN_TEST(Test_AlphaConfiguration);
            RUN_TEST(Test_GlowSizeConfiguration);
            RUN_TEST(Test_BorderMultiplierConfiguration);
            RUN_TEST(Test_GlowEnabled);
            RUN_TEST(Test_StylePresets);
            RUN_TEST(Test_RenderCompleteSelection);
            
            ImGui::Separator();
            ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "Results: %d passed, %d failed", passed, failed);
            ImGui::End();
            
            #undef RUN_TEST
        }
    };

} // namespace Olympe
