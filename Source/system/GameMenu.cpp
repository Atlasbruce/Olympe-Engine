#include "GameMenu.h"
#include "../VideoGame.h"
#include <iostream>
#include "system_utils.h"


void GameMenu::Activate()
{
    if (m_active) return;
    m_active = true;

    // Pause the game when the menu is activated
    VideoGame::Get().Pause();

    SYSTEM_LOG << "GameMenu: activated\n";
}

void GameMenu::Deactivate()
{
    if (!m_active) return;
    m_active = false;

    // Resume the game when the menu is closed
    VideoGame::Get().Resume();

    SYSTEM_LOG << "GameMenu: deactivated\n";
}

void GameMenu::Render()
{
    if (!m_active) return;
    SYSTEM_LOG << "--- GameMenu ---\n";
    for (size_t i = 0; i < m_entries.size(); ++i)
    {
        if (static_cast<int>(i) == m_selected) SYSTEM_LOG << "> "; else SYSTEM_LOG << "  ";
        SYSTEM_LOG << m_entries[i] << "\n";
    }
}
