#pragma once

#include <vector>
#include <unordered_map>
#include <string>
#include <memory>
#include <cmath>
#include <algorithm>
#include <queue>
#include <functional>
#include "vector.h"
#include "system/system_utils.h"

// Forward declarations
namespace Olympe { namespace Editor { struct LevelDefinition; } }

// ============================================================================
// TILE DATA STRUCTURES (Rich information per tile)
// ============================================================================

enum class TerrainType : uint8_t
{
	Invalid = 0,
	Ground,
	Water,
	Grass,
	Sand,
	Rock,
	Ice,
	Lava,
	Mud,
	Snow,
	Custom = 255
};

enum class CollisionLayer : uint8_t
{
	Ground = 0,       // Layer 0: Standard ground collision
	Sky = 1,          // Layer 1: Aerial navigation/flying
	Underground = 2,  // Layer 2: Underground/tunnels
	Volume = 3,       // Layer 3: 3D volumes (for stacked isometric)
	Custom1 = 4,
	Custom2 = 5,
	Custom3 = 6,
	Custom4 = 7,
	MaxLayers = 8
};

// Tile properties (rich data per tile)
struct TileProperties
{
	bool isBlocked = false;           // Hard collision (impassable wall)
	bool isNavigable = true;          // Can pathfind through
	float traversalCost = 1.0f;       // Cost for pathfinding (1.0 = normal, >1.0 = slow)
	TerrainType terrain = TerrainType::Ground;
	uint8_t customFlags = 0;          // 8 bits for custom gameplay flags
	
	// Multi-layer support
	CollisionLayer layer = CollisionLayer::Ground;
	
	// Dynamic state support (destructible walls, buildable bridges, openable doors)
	bool isDynamic = false;           // Can this tile change state?
	std::string onDestroyedState;     // State name after destruction (e.g., "Rubble")
	std::string onBuiltState;         // State name after construction (e.g., "Bridge")
	std::string metadata;             // JSON metadata for custom gameplay logic
	
	// Pre-calculated world coordinates (tile center) - for performance optimization
	float worldX = 0.0f;
	float worldY = 0.0f;
	
	TileProperties() = default;
};

// Grid projection types
enum class GridProjectionType
{
	Ortho = 0,
	Iso = 1,
	HexAxial = 2
};

// ============================================================================
// COLLISION MAP SINGLETON (with multi-layer support)
// ============================================================================

class CollisionMap
{
public:
	static CollisionMap& Get()
	{
		static CollisionMap instance;
		return instance;
	}

	// Initialize with layers
	void Initialize(int width, int height, GridProjectionType projection, 
				   float tileWidth, float tileHeight, int numLayers = 1,
				   float tileOffsetX = 0.0f, float tileOffsetY = 0.0f);
	
	// Layer management
	void SetActiveLayer(CollisionLayer layer);
	CollisionLayer GetActiveLayer() const { return m_activeLayer; }
	int GetNumLayers() const { return m_numLayers; }
	
	// Tile properties access (current layer)
	void SetTileProperties(int x, int y, const TileProperties& props);
	const TileProperties& GetTileProperties(int x, int y) const;
	
	// Tile properties access (specific layer)
	void SetTileProperties(int x, int y, CollisionLayer layer, const TileProperties& props);
	const TileProperties& GetTileProperties(int x, int y, CollisionLayer layer) const;
	
	// Quick collision checks (backward compatibility)
	void SetCollision(int x, int y, bool hasCollision);
	bool HasCollision(int x, int y) const;
	bool HasCollision(int x, int y, CollisionLayer layer) const;
	
	// Dynamic state transitions (NEW: for destructible walls, buildable bridges, openable doors)
	typedef std::function<void(TileProperties&)> TileUpdateFunc;
	void UpdateTileState(int x, int y, TileUpdateFunc updateFunc);
	void UpdateTileState(int x, int y, CollisionLayer layer, TileUpdateFunc updateFunc);
	
	// World-to-grid conversion
	void WorldToGrid(float worldX, float worldY, int& outGridX, int& outGridY) const;
	void GridToWorld(int gridX, int gridY, float& outWorldX, float& outWorldY) const;
	
	// Validation
	bool IsValidGridPosition(int x, int y) const;
	bool IsValidGridPosition(int x, int y, CollisionLayer layer) const;
	
	// Getters
	int GetWidth() const { return m_width; }
	int GetHeight() const { return m_height; }
	GridProjectionType GetProjection() const { return m_projection; }
	float GetTileWidth() const { return m_tileWidth; }
	float GetTileHeight() const { return m_tileHeight; }
	
	// Access raw grid (for visualization)
	const std::vector<std::vector<TileProperties>>& GetLayer(CollisionLayer layer) const;
	
	// Sectorization support (for future dynamic loading)
	struct Sector
	{
		int x, y;              // Sector grid position
		int width, height;     // Sector size in tiles
		bool isLoaded = false;
		bool isActive = false;
	};
	
	void RegisterSector(int sectorX, int sectorY, int width, int height);
	void LoadSector(int sectorX, int sectorY);
	void UnloadSector(int sectorX, int sectorY);
	const std::vector<Sector>& GetSectors() const { return m_sectors; }
	
	// Clear
	void Clear();


	CollisionMap() = default;
	~CollisionMap() = default;
	CollisionMap(const CollisionMap&) = delete;
	CollisionMap& operator=(const CollisionMap&) = delete;

	// Multi-layer storage: layers[layer][y][x]
	std::vector<std::vector<std::vector<TileProperties>>> m_layers;
	int m_numLayers = 1;
	CollisionLayer m_activeLayer = CollisionLayer::Ground;
	
	int m_width = 0;
	int m_height = 0;
	GridProjectionType m_projection = GridProjectionType::Ortho;
	float m_tileWidth = 32.0f;
	float m_tileHeight = 32.0f;
	
	// Tile offset for coordinate calculations (isometric alignment)
	float m_tileOffsetX = 0.0f;
	float m_tileOffsetY = 0.0f;
	
	// Sectorization (for future dynamic loading)
	std::vector<Sector> m_sectors;
	
	// Default empty tile (returned for invalid queries)
	static const TileProperties s_emptyTile;
};

// ============================================================================
// NAVIGATION MAP SINGLETON (pathfinding-optimized)
// ============================================================================

class NavigationMap
{
public:
	static NavigationMap& Get()
	{
		static NavigationMap instance;
		return instance;
	}

	// Initialize
	void Initialize(int width, int height, GridProjectionType projection,
				   float tileWidth, float tileHeight, int numLayers = 1);
	
	// Layer management
	void SetActiveLayer(CollisionLayer layer);
	CollisionLayer GetActiveLayer() const { return m_activeLayer; }
	
	// Tile properties access (delegates to CollisionMap for consistency)
	void SetNavigable(int x, int y, bool isNavigable, float cost = 1.0f);
	bool IsNavigable(int x, int y) const;
	float GetTraversalCost(int x, int y) const;
	
	// Layer-specific access
	bool IsNavigable(int x, int y, CollisionLayer layer) const;
	float GetTraversalCost(int x, int y, CollisionLayer layer) const;
	
	// World-to-grid conversion
	void WorldToGrid(float worldX, float worldY, int& outGridX, int& outGridY) const;
	void GridToWorld(int gridX, int gridY, float& outWorldX, float& outWorldY) const;
	
	// Validation
	bool IsValidGridPosition(int x, int y) const;
	
	// Getters
	int GetWidth() const { return m_width; }
	int GetHeight() const { return m_height; }
	GridProjectionType GetProjection() const { return m_projection; }
	
	// Pathfinding support (A*)
	struct PathNode
	{
		int x, y;
		float gCost;  // Cost from start
		float hCost;  // Heuristic to goal
		float fCost() const { return gCost + hCost; }
		PathNode* parent;
		
		PathNode() : x(0), y(0), gCost(0), hCost(0), parent(nullptr) {}
		PathNode(int _x, int _y) : x(_x), y(_y), gCost(0), hCost(0), parent(nullptr) {}
	};
	
	// A* pathfinding
	bool FindPath(int startX, int startY, int goalX, int goalY, 
				 std::vector<Vector>& outPath, CollisionLayer layer = CollisionLayer::Ground, int maxIterations = 10000);
	
	// Clear
	void Clear();


	NavigationMap() = default;
	~NavigationMap() = default;
	NavigationMap(const NavigationMap&) = delete;
	NavigationMap& operator=(const NavigationMap&) = delete;

	int m_width = 0;
	int m_height = 0;
	GridProjectionType m_projection = GridProjectionType::Ortho;
	float m_tileWidth = 32.0f;
	float m_tileHeight = 32.0f;
	CollisionLayer m_activeLayer = CollisionLayer::Ground;
	int m_numLayers = 1;
	
	// Helper: calculate heuristic for A*
	float Heuristic(int x1, int y1, int x2, int y2) const;
	
	// Helper: get neighbors (handles ortho/iso/hex)
	void GetNeighbors(int x, int y, std::vector<std::pair<int, int>>& outNeighbors) const;
};
