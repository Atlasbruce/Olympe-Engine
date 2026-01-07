/*
 * Olympe Blueprint Editor - History Panel
 * 
 * GUI panel for visualizing and managing undo/redo command history
 */

#pragma once

namespace Olympe
{
    /**
     * HistoryPanel - ImGui panel for command history visualization
     * Shows undo/redo stacks with command descriptions
     */
    class HistoryPanel
    {
    public:
        HistoryPanel();
        ~HistoryPanel();

        void Initialize();
        void Shutdown();
        void Render();

    private:
        bool m_ShowPanel;
    };
}
