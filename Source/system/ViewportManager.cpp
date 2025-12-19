#include "ViewportManager.h"
#include "../GameEngine.h"

// Implementation file intentionally empty: all behavior implemented inline in header for simplicity.
// If later needed, complex logic can be moved here.

ViewportManager::ViewportManager()
{ 
	Initialize(GameEngine::screenWidth, GameEngine::screenHeight); 
}

ViewportManager::~ViewportManager()
{
	Shutdown();
}
//-------------------------------------------------------------
void ViewportManager::Render()
{
    // For debug purposes, render viewport rectangles

    SDL_SetRenderDrawColor(GameEngine::renderer, 255, 0, 0, SDL_ALPHA_OPAQUE); // red
    for (const auto &rect : m_viewRects)
    {
		SDL_RenderRect(GameEngine::renderer, &rect);
	}
}
//-------------------------------------------------------------
void ViewportManager::Initialize(int w, int h)
{
    m_width = w; m_height = h;
    m_players.clear();
    m_playerIndexMap.clear();
    m_viewRects.clear();
    // default single viewport
    UpdateViewports();
    SYSTEM_LOG << "Viewport Initialized\n";
}
//-------------------------------------------------------------
void ViewportManager::Shutdown()
{
    m_players.clear();
    m_viewRects.clear();
    m_playerIndexMap.clear();
    SYSTEM_LOG << "Viewport Shutdown\n";
}
//-------------------------------------------------------------
// Add a player id. Returns true if added, false if already present or full
bool ViewportManager::AddPlayer(short playerID, ViewportLayout viewportLayout = ViewportLayout::ViewportLayout_Grid1x1)
{
    if (m_playerIndexMap.find(playerID) != m_playerIndexMap.end()) 
        return false;

    if (m_players.size() >= MAX_PLAYERS) 
        return false;

    m_players.push_back(playerID);
    m_playerIndexMap[playerID] = static_cast<int>(m_players.size() - 1);
    UpdateViewports();
    return true;
}
//-------------------------------------------------------------
bool ViewportManager::RemovePlayer(short playerID)
{
    auto it = std::find(m_players.begin(), m_players.end(), playerID);
    if (it == m_players.end()) return false;
    m_players.erase(it);
    m_playerIndexMap.clear();
    for (size_t i = 0; i < m_players.size(); ++i) m_playerIndexMap[m_players[i]] = static_cast<int>(i);
    UpdateViewports();
    return true;
}
//-------------------------------------------------------------
void ViewportManager::SetSize(int w, int h)
{
    m_width = w; m_height = h; UpdateViewports();
}
//-------------------------------------------------------------
// If a player exists, fill out outRect and return true
bool ViewportManager::GetViewRectForPlayer(short playerID, SDL_FRect& outRect) const
{
    auto it = m_playerIndexMap.find(playerID);
    if (it == m_playerIndexMap.end()) return false;
    int idx = it->second;
    if (idx < 0 || idx >= static_cast<int>(m_viewRects.size())) return false;
    outRect = m_viewRects[idx];
    return true;
}
//  -------------------------------------------------------------
void ViewportManager::UpdateViewports()
{
    m_viewRects.clear();
    int n = static_cast<int>(m_players.size());
    
    // If no players, create a single full-screen viewport but don't treat it as a player
    if (n == 0)
    {
        SDL_FRect r{ 0,0,static_cast<float>(m_width),static_cast<float>(m_height) };
        m_viewRects.push_back(r);
        return; // Exit early - no player mapping needed
    }

    switch (n)
	{
	    case 1:
	    {
		    SDL_FRect r{ 0,0,static_cast<float>(m_width),static_cast<float>(m_height) };
		    m_viewRects.push_back(r);
		    break;
	    }
	    case 2:
	    {
		    int w = m_width / 2;
		    SDL_FRect r1{ 0,0,static_cast<float>(w),static_cast<float>(m_height) };
		    SDL_FRect r2{ static_cast<float>(w),0,static_cast<float>(m_width - w),static_cast<float>(m_height) };
		    m_viewRects.push_back(r1);
		    m_viewRects.push_back(r2);
		    break;
	    }
	    case 3:
	    {
		    int w = m_width / 3;
		    SDL_FRect r1{ 0,0,static_cast<float>(w),static_cast<float>(m_height) };
		    SDL_FRect r2{ static_cast<float>(w),0,static_cast<float>(w),static_cast<float>(m_height) };
		    SDL_FRect r3{ static_cast<float>(2 * w),0,static_cast<float>(m_width - 2 * w),static_cast<float>(m_height) };
		    m_viewRects.push_back(r1);
		    m_viewRects.push_back(r2);
		    m_viewRects.push_back(r3);
		    break;
	    }
	    case 4:
	    {
		    int w = m_width / 2;
		    int h = m_height / 2;
		    SDL_FRect r1{ 0,0,static_cast<float>(w),static_cast<float>(h) };
		    SDL_FRect r2{ static_cast<float>(w),0,static_cast<float>(m_width - w),static_cast<float>(h) };
		    SDL_FRect r3{ 0,static_cast<float>(h),static_cast<float>(w),static_cast<float>(m_height - h) };
		    SDL_FRect r4{ static_cast<float>(w),static_cast<float>(h),static_cast<float>(m_width - w),static_cast<float>(m_height - h) };
		    m_viewRects.push_back(r1);
		    m_viewRects.push_back(r2);
		    m_viewRects.push_back(r3);
		    m_viewRects.push_back(r4);
		    break;
	    }
	    case 5:
            {
            int w = m_width / 3;
            int h = m_height / 2;
            SDL_FRect r1{ 0,0,static_cast<float>(w),static_cast<float>(h) };
            SDL_FRect r2{ static_cast<float>(w),0,static_cast<float>(w),static_cast<float>(h) };
            SDL_FRect r3{ static_cast<float>(2 * w),0,static_cast<float>(m_width - 2 * w),static_cast<float>(h) };
            SDL_FRect r4{ 0,static_cast<float>(h),static_cast<float>(w),static_cast<float>(m_height - h) };
            SDL_FRect r5{ static_cast<float>(w),static_cast<float>(h),static_cast<float>(w),static_cast<float>(m_height - h) };
            m_viewRects.push_back(r1);
            m_viewRects.push_back(r2);
            m_viewRects.push_back(r3);
            m_viewRects.push_back(r4);
            m_viewRects.push_back(r5);
            break;
	    }
        case 6:
		{
            int w = m_width / 3;
            int h = m_height / 2;
            SDL_FRect r1{ 0,0,static_cast<float>(w),static_cast<float>(h) };
            SDL_FRect r2{ static_cast<float>(w),0,static_cast<float>(w),static_cast<float>(h) };
            SDL_FRect r3{ static_cast<float>(2 * w),0,static_cast<float>(m_width - 2 * w),static_cast<float>(h) };
            SDL_FRect r4{ 0,static_cast<float>(h),static_cast<float>(w),static_cast<float>(m_height - h) };
            SDL_FRect r5{ static_cast<float>(w),static_cast<float>(h),static_cast<float>(w),static_cast<float>(m_height - h) };
            SDL_FRect r6{ static_cast<float>(2 * w),static_cast<float>(h),static_cast<float>(m_width - 2 * w),static_cast<float>(m_height - h) };
            m_viewRects.push_back(r1);
            m_viewRects.push_back(r2);
            m_viewRects.push_back(r3);
            m_viewRects.push_back(r4);
            m_viewRects.push_back(r5);
            m_viewRects.push_back(r6);
            break;
		}
		case 7:
            {
            int w = m_width / 4;
            int h = m_height / 2;
            SDL_FRect r1{ 0,0,static_cast<float>(w),static_cast<float>(h) };
            SDL_FRect r2{ static_cast<float>(w),0,static_cast<float>(w),static_cast<float>(h) };
            SDL_FRect r3{ static_cast<float>(2 * w),0,static_cast<float>(w),static_cast<float>(h) };
            SDL_FRect r4{ static_cast<float>(3 * w),0,static_cast<float>(m_width - 3 * w),static_cast<float>(h) };
            SDL_FRect r5{ 0,static_cast<float>(h),static_cast<float>(w),static_cast<float>(m_height - h) };
            SDL_FRect r6{ static_cast<float>(w),static_cast<float>(h),static_cast<float>(w),static_cast<float>(m_height - h) };
            SDL_FRect r7{ static_cast<float>(2 * w),static_cast<float>(h),static_cast<float>(w),static_cast<float>(m_height - h) };
            m_viewRects.push_back(r1);
            m_viewRects.push_back(r2);
            m_viewRects.push_back(r3);
            m_viewRects.push_back(r4);
            m_viewRects.push_back(r5);
            m_viewRects.push_back(r6);
            m_viewRects.push_back(r7);
            break;
        }
        case 8:
        {
            int w = m_width / 4;
            int h = m_height / 2;
            SDL_FRect r1{ 0,0,static_cast<float>(w),static_cast<float>(h) };
            SDL_FRect r2{ static_cast<float>(w),0,static_cast<float>(w),static_cast<float>(h) };
            SDL_FRect r3{ static_cast<float>(2 * w),0,static_cast<float>(w),static_cast<float>(h) };
            SDL_FRect r4{ static_cast<float>(3 * w),0,static_cast<float>(m_width - 3 * w),static_cast<float>(h) };
            SDL_FRect r5{ 0,static_cast<float>(h),static_cast<float>(w),static_cast<float>(m_height - h) };
			SDL_FRect r6{ static_cast<float>(w),static_cast<float>(h),static_cast<float>(w),static_cast<float>(m_height - h) };
            SDL_FRect r7{ static_cast<float>(2 * w),static_cast<float>(h),static_cast<float>(w),static_cast<float>(m_height - h) };
            SDL_FRect r8{ static_cast<float>(3 * w),static_cast<float>(h),static_cast<float>(m_width - 3 * w),static_cast<float>(m_height - h) };
            m_viewRects.push_back(r1);
            m_viewRects.push_back(r2);
            m_viewRects.push_back(r3);
            m_viewRects.push_back(r4);
            m_viewRects.push_back(r5);
            m_viewRects.push_back(r6);
            m_viewRects.push_back(r7);
            m_viewRects.push_back(r8);
            break;
		}
	default:
        break;
	}


    // Rebuild playerIndexMap to match players to their viewport indices
    m_playerIndexMap.clear();
    for (size_t i = 0; i < m_players.size() && i < m_viewRects.size(); ++i)
        m_playerIndexMap[m_players[i]] = static_cast<int>(i);
}