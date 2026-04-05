#pragma once

#include "../vector.h"
#include "../third_party/imgui/imgui.h"

namespace Olympe
{
    /**
     * @brief Encapsule l'effet de sélection des nodes (glow + bordure épaisse)
     * 
     * Fournit un système standardisé de rendu de sélection réutilisable
     * dans tous les canvas (VisualScript, EntityPrefab, AIEditor, etc.)
     * 
     * Usage:
     *   SelectionEffectRenderer renderer;
     *   renderer.SetGlowColor({0.0f, 0.8f, 1.0f});
     *   renderer.SetGlowAlpha(0.3f);
     *   
     *   // During node rendering
     *   if (node.selected) {
     *       renderer.RenderSelectionGlow(minScreen, maxScreen, canvasZoom, nodeScale);
     *   }
     *   renderer.RenderSelectionBorder(minScreen, maxScreen, nodeColor, baseWidth, canvasZoom);
     */
    class SelectionEffectRenderer
    {
    public:
        SelectionEffectRenderer();
        ~SelectionEffectRenderer() = default;

        // ===== Glow Configuration =====
        
        /// @brief Définit la couleur du glow de sélection
        /// @param color Couleur RGB (valeurs 0.0-1.0)
        /// @default {0.0f, 0.8f, 1.0f} (Cyan)
        void SetGlowColor(const Vector& color) { m_glowColor = color; }
        Vector GetGlowColor() const { return m_glowColor; }

        /// @brief Définit l'alpha (transparence) du glow
        /// @param alpha Valeur 0.0-1.0 (0=invisible, 1=opaque)
        /// @default 0.3f
        void SetGlowAlpha(float alpha) { m_glowAlpha = (alpha > 1.0f) ? 1.0f : (alpha < 0.0f) ? 0.0f : alpha; }
        float GetGlowAlpha() const { return m_glowAlpha; }

        /// @brief Définit la taille de base du glow (avant zoom/scale)
        /// @param size Pixels non-zoomés
        /// @default 4.0f
        void SetBaseGlowSize(float size) { m_baseGlowSize = (size > 0.0f) ? size : 0.1f; }
        float GetBaseGlowSize() const { return m_baseGlowSize; }

        // ===== Border Configuration =====

        /// @brief Définit le multiplicateur d'épaisseur de bordure
        /// @param multiplier Épaisseur finale = baseWidth * multiplier
        /// @default 2.0f (la bordure double quand sélectionné)
        void SetBorderWidthMultiplier(float multiplier) { m_borderWidthMultiplier = (multiplier > 0.1f) ? multiplier : 0.1f; }
        float GetBorderWidthMultiplier() const { return m_borderWidthMultiplier; }

        /// @brief Active/désactive l'effet de glow (utile pour performance)
        /// @default true
        void SetGlowEnabled(bool enabled) { m_glowEnabled = enabled; }
        bool IsGlowEnabled() const { return m_glowEnabled; }

        // ===== Rendering =====

        /**
         * @brief Dessine le glow de sélection (fond lumineux)
         * 
         * Cette fonction doit être appelée AVANT de dessiner la boîte du node,
         * pour que le glow apparaisse en arrière-plan.
         * 
         * @param minScreen Position écran minimale (coin haut-gauche)
         * @param maxScreen Position écran maximale (coin bas-droit)
         * @param canvasZoom Niveau de zoom du canvas
         * @param nodeScale Multiplicateur d'échelle du node (défaut 1.0f)
         * @param cornerRadius Rayon des coins arrondis
         */
        void RenderSelectionGlow(
            const ImVec2& minScreen,
            const ImVec2& maxScreen,
            float canvasZoom = 1.0f,
            float nodeScale = 1.0f,
            float cornerRadius = 5.0f
        ) const;

        /**
         * @brief Rend la bordure de sélection (épaissie)
         * 
         * Appeler APRÈS la boîte principale du node.
         * Automatiquement multiplie l'épaisseur de base par le multiplicateur.
         * 
         * @param minScreen Position écran minimale
         * @param maxScreen Position écran maximale
         * @param borderColor Couleur de la bordure en ImU32
         * @param baseWidth Épaisseur de base (avant multiplication)
         * @param canvasZoom Niveau de zoom du canvas
         * @param cornerRadius Rayon des coins arrondis
         */
        void RenderSelectionBorder(
            const ImVec2& minScreen,
            const ImVec2& maxScreen,
            ImU32 borderColor,
            float baseWidth,
            float canvasZoom = 1.0f,
            float cornerRadius = 5.0f
        ) const;

        /**
         * @brief Rend l'ensemble de l'effet de sélection (glow + bordure)
         * 
         * Fonction de commodité qui appelle RenderSelectionGlow et RenderSelectionBorder.
         * À utiliser si vous voulez contrôler totalement le rendu.
         * 
         * @param minScreen Position écran minimale
         * @param maxScreen Position écran maximale
         * @param borderColor Couleur de la bordure
         * @param baseWidth Épaisseur de bordure de base
         * @param canvasZoom Niveau de zoom
         * @param nodeScale Échelle du node
         * @param cornerRadius Rayon des coins
         */
        void RenderCompleteSelection(
            const ImVec2& minScreen,
            const ImVec2& maxScreen,
            ImU32 borderColor,
            float baseWidth,
            float canvasZoom = 1.0f,
            float nodeScale = 1.0f,
            float cornerRadius = 5.0f
        ) const;

        // ===== Preset Styles =====

        /// @brief Style standard "Olympe Blue" - Cyan vif avec glow modéré
        void ApplyStyle_OlympeBlue();

        /// @brief Style "Gold Accent" - Orange/or pour éditeurs de spécialité
        void ApplyStyle_GoldAccent();

        /// @brief Style "Green Energy" - Vert lumineux pour emphasis
        void ApplyStyle_GreenEnergy();

        /// @brief Style "Purple Mystery" - Violet/magenta
        void ApplyStyle_PurpleMystery();

        /// @brief Style "Red Alert" - Rouge vif pour avertissements/erreurs
        void ApplyStyle_RedAlert();

    private:
        Vector m_glowColor = {0.0f, 0.8f, 1.0f};     // Cyan par défaut
        float m_glowAlpha = 0.3f;                    // 30% d'opacité
        float m_baseGlowSize = 4.0f;                 // 4 pixels de glow
        float m_borderWidthMultiplier = 2.0f;        // Bordure ×2 si sélectionné
        bool m_glowEnabled = true;
    };

} // namespace Olympe
