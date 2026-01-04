#pragma once
#include <vector>
#include <unordered_map>
#include <SDL3/SDL.h>


// Simple Viewport manager singleton that computes up to 4 view rectangles
// according to the number of players. Viewports are laid out as follows:
// 1 player: full screen
// 2 players: vertical split (2 columns)
// 3 players: vertical split (3 columns)
// 4 players: 2x2 grid
enum class ViewportLayout
{
    ViewportLayout_Grid1x1,
    ViewportLayout_Grid2x1,
    ViewportLayout_Grid3x1,
    ViewportLayout_Grid2x2,
	ViewportLayout_Grid3x2,
	ViewportLayout_Grid4x2
};

class ViewportManager 
{
public:
    ViewportManager();
    ~ViewportManager();

    static ViewportManager& GetInstance()
    {
        static ViewportManager instance;
        return instance;
    }
    static ViewportManager& Get() { return GetInstance(); }

	virtual void Render() ; // Render viewports (for debug purposes)

    void Initialize(int w, int h);
    void Shutdown();
    bool AddPlayer(short playerID, ViewportLayout viewportLayout);
    bool RemovePlayer(short playerID);
    void SetSize(int w, int h);
    int GetPlayerCount() const { return static_cast<int>(m_players.size() == 0 ? 0 : m_players.size()); }

    // Return view rects in the same order as m_players (if no players defined returns single full-screen rect)
    const std::vector<SDL_FRect>& GetViewRects() const { return m_viewRects; }


    bool GetViewRectForPlayer(short playerID, SDL_FRect& outRect) const;
    const std::vector<short>& GetPlayers() const { return m_players; }
    void UpdateViewports();

private:
    int m_width = 400;
    int m_height = 320;
    std::vector<SDL_FRect> m_viewRects;
    std::vector<short> m_players; // ordered list of player ids
    std::unordered_map<short,int> m_playerIndexMap;

	ViewportLayout m_currentLayout = ViewportLayout::ViewportLayout_Grid1x1;
};
