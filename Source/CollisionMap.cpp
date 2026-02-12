/*
Olympe Engine V2 - 2025
CollisionMap & NavigationMap implementation
Multi-layer collision and A* pathfinding system
*/

#include "CollisionMap.h"
#include <limits>
#include <unordered_set>

// ============================================================================
// COLLISION MAP IMPLEMENTATION
// ============================================================================

// Static member initialization
const TileProperties CollisionMap::s_emptyTile = TileProperties();

void CollisionMap::Initialize(int width, int height, GridProjectionType projection,
							  float tileWidth, float tileHeight, int numLayers,
							  float tileOffsetX, float tileOffsetY)
{
	SYSTEM_LOG << "CollisionMap::Initialize(" << width << "x" << height << ", "
			   << numLayers << " layers, projection=" << static_cast<int>(projection) << ")\n";
	SYSTEM_LOG << "  -> Received tile dimensions: " << tileWidth << "x" << tileHeight << " px\n";
	
	m_width = width;
	m_height = height;
	m_projection = projection;
	m_tileWidth = tileWidth;
	m_tileHeight = tileHeight;
	m_numLayers = numLayers;
	m_activeLayer = CollisionLayer::Ground;
	m_tileOffsetX = tileOffsetX;
	m_tileOffsetY = tileOffsetY;
	
	SYSTEM_LOG << "  -> Stored tile dimensions: m_tileWidth=" << m_tileWidth 
	           << ", m_tileHeight=" << m_tileHeight << "\n";
	
	// Log tile offset if present
	if (m_tileOffsetX != 0.0f || m_tileOffsetY != 0.0f)
	{
		SYSTEM_LOG << "  -> Tile offset: (" << m_tileOffsetX << ", " << m_tileOffsetY << ")\n";
	}
	
	// Allocate layers
	m_layers.resize(numLayers);
	for (int layer = 0; layer < numLayers; ++layer)
	{
		m_layers[layer].resize(height);
		for (int y = 0; y < height; ++y)
		{
			m_layers[layer][y].resize(width);
			// Initialize tiles with default properties
			for (int x = 0; x < width; ++x)
			{
				m_layers[layer][y][x] = TileProperties();
				m_layers[layer][y][x].layer = static_cast<CollisionLayer>(layer);
			}
		}
	}
	
	SYSTEM_LOG << "  -> Allocated " << (width * height * numLayers) << " tiles\n";
	
	// Pre-calculate world coordinates for all tiles (performance optimization)
	for (int layer = 0; layer < numLayers; ++layer)
	{
		for (int y = 0; y < height; ++y)
		{
			for (int x = 0; x < width; ++x)
			{
				float worldX, worldY;
				GridToWorld(x, y, worldX, worldY);
				
				// Apply tile offset correction for isometric projection
				if (m_projection == GridProjectionType::Iso)
				{
					worldX -= m_tileOffsetX;
					worldY += m_tileOffsetY / 2.f;
				}
				
				m_layers[layer][y][x].worldX = worldX;
				m_layers[layer][y][x].worldY = worldY;
				
				#ifdef DEBUG_COLLISION_MAP_INIT
				// Debug first few tiles in first layer to verify correct calculations
				if (layer == 0 && y == 0 && x < 3)
				{
					SYSTEM_LOG << "    [DEBUG] Tile (" << x << "," << y << ") -> world (" 
					           << worldX << ", " << worldY << ")";
					if (m_tileOffsetX != 0.0f || m_tileOffsetY != 0.0f)
					{
						SYSTEM_LOG << " [offset: (" << m_tileOffsetX << ", " << m_tileOffsetY << ")]";
					}
					SYSTEM_LOG << "\n";
				}
				#endif
			}
		}
	}
	
	SYSTEM_LOG << "  -> Pre-calculated world coordinates for " << (width * height * numLayers) << " tiles\n";
}

void CollisionMap::SetActiveLayer(CollisionLayer layer)
{
	if (static_cast<int>(layer) < m_numLayers)
	{
		m_activeLayer = layer;
	}
}

void CollisionMap::SetTileProperties(int x, int y, const TileProperties& props)
{
	SetTileProperties(x, y, m_activeLayer, props);
}

const TileProperties& CollisionMap::GetTileProperties(int x, int y) const
{
	return GetTileProperties(x, y, m_activeLayer);
}

void CollisionMap::SetTileProperties(int x, int y, CollisionLayer layer, const TileProperties& props)
{
	if (IsValidGridPosition(x, y, layer))
	{
		int layerIdx = static_cast<int>(layer);
		m_layers[layerIdx][y][x] = props;
		m_layers[layerIdx][y][x].layer = layer;
	}
}

const TileProperties& CollisionMap::GetTileProperties(int x, int y, CollisionLayer layer) const
{
	if (IsValidGridPosition(x, y, layer))
	{
		int layerIdx = static_cast<int>(layer);
		return m_layers[layerIdx][y][x];
	}
	return s_emptyTile;
}

void CollisionMap::SetCollision(int x, int y, bool hasCollision)
{
	if (IsValidGridPosition(x, y))
	{
		int layerIdx = static_cast<int>(m_activeLayer);
		m_layers[layerIdx][y][x].isBlocked = hasCollision;
		m_layers[layerIdx][y][x].isNavigable = !hasCollision;
	}
}

bool CollisionMap::HasCollision(int x, int y) const
{
	return HasCollision(x, y, m_activeLayer);
}

bool CollisionMap::HasCollision(int x, int y, CollisionLayer layer) const
{
	if (IsValidGridPosition(x, y, layer))
	{
		int layerIdx = static_cast<int>(layer);
		return m_layers[layerIdx][y][x].isBlocked;
	}
	return true; // Out of bounds = collision
}

void CollisionMap::UpdateTileState(int x, int y, TileUpdateFunc updateFunc)
{
	UpdateTileState(x, y, m_activeLayer, updateFunc);
}

void CollisionMap::UpdateTileState(int x, int y, CollisionLayer layer, TileUpdateFunc updateFunc)
{
	if (IsValidGridPosition(x, y, layer) && updateFunc)
	{
		int layerIdx = static_cast<int>(layer);
		updateFunc(m_layers[layerIdx][y][x]);
	}
}

void CollisionMap::WorldToGrid(float worldX, float worldY, int& outGridX, int& outGridY) const
{
	switch (m_projection)
	{
	case GridProjectionType::Ortho:
		// Orthogonal: direct mapping
		outGridX = static_cast<int>(std::floor(worldX / m_tileWidth));
		outGridY = static_cast<int>(std::floor(worldY / m_tileHeight));
		break;
		
	case GridProjectionType::Iso:
		// Isometric: diamond transformation
		// Convert world position to isometric grid coordinates
		// Standard isometric: divide by half tile dimensions for proper scaling
		{
			float isoX = worldX / (m_tileWidth * 0.5f);
			float isoY = worldY / (m_tileHeight * 0.5f);
			outGridX = static_cast<int>(std::floor((isoX + isoY) * 0.5f));
			outGridY = static_cast<int>(std::floor((isoY - isoX) * 0.5f));
		}
		break;
		
	case GridProjectionType::HexAxial:
		// Hexagonal (axial coordinates, pointy-top)
		{
			float q = (worldX * std::sqrt(3.0f) / 3.0f - worldY / 3.0f) / m_tileWidth;
			float r = (worldY * 2.0f / 3.0f) / m_tileHeight;
			
			// Cube coordinate conversion for rounding
			float x = q;
			float z = r;
			float y = -x - z;
			
			int rx = static_cast<int>(std::round(x));
			int ry = static_cast<int>(std::round(y));
			int rz = static_cast<int>(std::round(z));
			
			float x_diff = std::abs(rx - x);
			float y_diff = std::abs(ry - y);
			float z_diff = std::abs(rz - z);
			
			if (x_diff > y_diff && x_diff > z_diff)
				rx = -ry - rz;
			else if (y_diff > z_diff)
				ry = -rx - rz;
			else
				rz = -rx - ry;
			
			outGridX = rx;
			outGridY = rz;
		}
		break;
		
	default:
		outGridX = 0;
		outGridY = 0;
		break;
	}
}

void CollisionMap::GridToWorld(int gridX, int gridY, float& outWorldX, float& outWorldY) const
{
	switch (m_projection)
	{
	case GridProjectionType::Ortho:
		// Orthogonal: direct mapping (to tile center)
		outWorldX = (gridX + 0.5f) * m_tileWidth;
		outWorldY = (gridY + 0.5f) * m_tileHeight;
		break;
		
	case GridProjectionType::Iso:
		// Isometric: diamond transformation (to tile center)
		// Standard isometric formula with half tile dimensions
		outWorldX = (gridX - gridY) * (m_tileWidth * 0.5f);
		outWorldY = (gridX + gridY) * (m_tileHeight * 0.5f);
		break;
		
	case GridProjectionType::HexAxial:
		// Hexagonal (axial coordinates, pointy-top) (to tile center)
		{
			float q = static_cast<float>(gridX);
			float r = static_cast<float>(gridY);
			outWorldX = m_tileWidth * (std::sqrt(3.0f) * q + std::sqrt(3.0f) / 2.0f * r);
			outWorldY = m_tileHeight * (3.0f / 2.0f * r);
		}
		break;
		
	default:
		outWorldX = 0.0f;
		outWorldY = 0.0f;
		break;
	}
}

bool CollisionMap::IsValidGridPosition(int x, int y) const
{
	return x >= 0 && x < m_width && y >= 0 && y < m_height;
}

bool CollisionMap::IsValidGridPosition(int x, int y, CollisionLayer layer) const
{
	int layerIdx = static_cast<int>(layer);
	return IsValidGridPosition(x, y) && layerIdx >= 0 && layerIdx < m_numLayers;
}

const std::vector<std::vector<TileProperties>>& CollisionMap::GetLayer(CollisionLayer layer) const
{
	static const std::vector<std::vector<TileProperties>> emptyLayer;
	
	int layerIdx = static_cast<int>(layer);
	if (layerIdx >= 0 && layerIdx < static_cast<int>(m_layers.size()))
	{
		return m_layers[layerIdx];
	}
	return emptyLayer;
}

void CollisionMap::RegisterSector(int sectorX, int sectorY, int width, int height)
{
	Sector sector;
	sector.x = sectorX;
	sector.y = sectorY;
	sector.width = width;
	sector.height = height;
	sector.isLoaded = false;
	sector.isActive = false;
	m_sectors.push_back(sector);
}

void CollisionMap::LoadSector(int sectorX, int sectorY)
{
	for (Sector& sector : m_sectors)
	{
		if (sector.x == sectorX && sector.y == sectorY)
		{
			sector.isLoaded = true;
			sector.isActive = true;
			break;
		}
	}
}

void CollisionMap::UnloadSector(int sectorX, int sectorY)
{
	for (Sector& sector : m_sectors)
	{
		if (sector.x == sectorX && sector.y == sectorY)
		{
			sector.isLoaded = false;
			sector.isActive = false;
			break;
		}
	}
}

void CollisionMap::Clear()
{
	m_layers.clear();
	m_sectors.clear();
	m_width = 0;
	m_height = 0;
	m_numLayers = 1;
	m_activeLayer = CollisionLayer::Ground;
}

// ============================================================================
// NAVIGATION MAP IMPLEMENTATION
// ============================================================================

void NavigationMap::Initialize(int width, int height, GridProjectionType projection,
							   float tileWidth, float tileHeight, int numLayers)
{
	SYSTEM_LOG << "NavigationMap::Initialize(" << width << "x" << height << ", "
			   << numLayers << " layers)\n";
	
	m_width = width;
	m_height = height;
	m_projection = projection;
	m_tileWidth = tileWidth;
	m_tileHeight = tileHeight;
	m_numLayers = numLayers;
	m_activeLayer = CollisionLayer::Ground;
	
	// NavigationMap delegates to CollisionMap for tile storage
	// We just need to ensure CollisionMap is initialized
	SYSTEM_LOG << "  -> NavigationMap ready (delegates to CollisionMap)\n";
}

void NavigationMap::SetActiveLayer(CollisionLayer layer)
{
	if (static_cast<int>(layer) < m_numLayers)
	{
		m_activeLayer = layer;
	}
}

void NavigationMap::SetNavigable(int x, int y, bool isNavigable, float cost)
{
	CollisionMap& collMap = CollisionMap::Get();
	if (collMap.IsValidGridPosition(x, y, m_activeLayer))
	{
		TileProperties props = collMap.GetTileProperties(x, y, m_activeLayer);
		props.isNavigable = isNavigable;
		props.traversalCost = cost;
		collMap.SetTileProperties(x, y, m_activeLayer, props);
	}
}

bool NavigationMap::IsNavigable(int x, int y) const
{
	return IsNavigable(x, y, m_activeLayer);
}

float NavigationMap::GetTraversalCost(int x, int y) const
{
	return GetTraversalCost(x, y, m_activeLayer);
}

bool NavigationMap::IsNavigable(int x, int y, CollisionLayer layer) const
{
	CollisionMap& collMap = CollisionMap::Get();
	if (collMap.IsValidGridPosition(x, y, layer))
	{
		const TileProperties& props = collMap.GetTileProperties(x, y, layer);
		return props.isNavigable && !props.isBlocked;
	}
	return false;
}

float NavigationMap::GetTraversalCost(int x, int y, CollisionLayer layer) const
{
	CollisionMap& collMap = CollisionMap::Get();
	if (collMap.IsValidGridPosition(x, y, layer))
	{
		const TileProperties& props = collMap.GetTileProperties(x, y, layer);
		return props.traversalCost;
	}
	return std::numeric_limits<float>::max();
}

void NavigationMap::WorldToGrid(float worldX, float worldY, int& outGridX, int& outGridY) const
{
	CollisionMap::Get().WorldToGrid(worldX, worldY, outGridX, outGridY);
}

void NavigationMap::GridToWorld(int gridX, int gridY, float& outWorldX, float& outWorldY) const
{
	CollisionMap::Get().GridToWorld(gridX, gridY, outWorldX, outWorldY);
}

bool NavigationMap::IsValidGridPosition(int x, int y) const
{
	return x >= 0 && x < m_width && y >= 0 && y < m_height;
}

float NavigationMap::Heuristic(int x1, int y1, int x2, int y2) const
{
	switch (m_projection)
	{
	case GridProjectionType::Ortho:
		// Manhattan distance (4-connected grid)
		return static_cast<float>(std::abs(x2 - x1) + std::abs(y2 - y1));
		
	case GridProjectionType::Iso:
		// Diamond/Chebyshev distance (isometric grid)
		return static_cast<float>(std::max(std::abs(x2 - x1), std::abs(y2 - y1)));
		
	case GridProjectionType::HexAxial:
		// Axial distance (hexagonal grid)
		{
			int dx = x2 - x1;
			int dy = y2 - y1;
			return static_cast<float>((std::abs(dx) + std::abs(dx + dy) + std::abs(dy)) / 2);
		}
		
	default:
		return 0.0f;
	}
}

void NavigationMap::GetNeighbors(int x, int y, std::vector<std::pair<int, int>>& outNeighbors) const
{
	outNeighbors.clear();
	
	switch (m_projection)
	{
	case GridProjectionType::Ortho:
		// 4-connected (up, down, left, right)
		outNeighbors.push_back(std::make_pair(x, y - 1)); // Up
		outNeighbors.push_back(std::make_pair(x, y + 1)); // Down
		outNeighbors.push_back(std::make_pair(x - 1, y)); // Left
		outNeighbors.push_back(std::make_pair(x + 1, y)); // Right
		break;
		
	case GridProjectionType::Iso:
		// 4-connected diamond (isometric)
		outNeighbors.push_back(std::make_pair(x - 1, y)); // NW
		outNeighbors.push_back(std::make_pair(x + 1, y)); // SE
		outNeighbors.push_back(std::make_pair(x, y - 1)); // NE
		outNeighbors.push_back(std::make_pair(x, y + 1)); // SW
		break;
		
	case GridProjectionType::HexAxial:
		// 6-connected (hexagonal, pointy-top)
		outNeighbors.push_back(std::make_pair(x + 1, y));     // E
		outNeighbors.push_back(std::make_pair(x + 1, y - 1)); // NE
		outNeighbors.push_back(std::make_pair(x, y - 1));     // NW
		outNeighbors.push_back(std::make_pair(x - 1, y));     // W
		outNeighbors.push_back(std::make_pair(x - 1, y + 1)); // SW
		outNeighbors.push_back(std::make_pair(x, y + 1));     // SE
		break;
		
	default:
		break;
	}
}

bool NavigationMap::FindPath(int startX, int startY, int goalX, int goalY,
							 std::vector<Vector>& outPath, CollisionLayer layer, int maxIterations)
{
	outPath.clear();
	
	// Validate positions
	if (!IsValidGridPosition(startX, startY) || !IsValidGridPosition(goalX, goalY))
	{
		return false;
	}
	
	if (!IsNavigable(startX, startY, layer) || !IsNavigable(goalX, goalY, layer))
	{
		return false;
	}
	
	// Early exit if start == goal
	if (startX == goalX && startY == goalY)
	{
		float worldX, worldY;
		GridToWorld(startX, startY, worldX, worldY);
		outPath.push_back(Vector(worldX, worldY, 0.0f));
		return true;
	}
	
	// A* pathfinding with priority queue (min-heap)
	struct NodeCompare
	{
		bool operator()(const PathNode* a, const PathNode* b) const
		{
			return a->fCost() > b->fCost(); // Min-heap (lower fCost = higher priority)
		}
	};
	
	std::priority_queue<PathNode*, std::vector<PathNode*>, NodeCompare> openSet;
	std::unordered_set<int> closedSet;
	std::unordered_map<int, PathNode*> allNodes;
	
	// Helper: encode position to unique int
	auto encodePos = [this](int x, int y) -> int {
		return y * m_width + x;
	};
	
	// Create start node
	PathNode* startNode = new PathNode(startX, startY);
	startNode->gCost = 0.0f;
	startNode->hCost = Heuristic(startX, startY, goalX, goalY);
	startNode->parent = nullptr;
	
	openSet.push(startNode);
	allNodes[encodePos(startX, startY)] = startNode;
	
	PathNode* goalNode = nullptr;
	int iterations = 0;
	
	while (!openSet.empty() && iterations < maxIterations)
	{
		++iterations;
		
		// Get node with lowest fCost
		PathNode* current = openSet.top();
		openSet.pop();
		
		int currentKey = encodePos(current->x, current->y);
		
		// Skip if already processed
		if (closedSet.find(currentKey) != closedSet.end())
		{
			continue;
		}
		
		closedSet.insert(currentKey);
		
		// Check if goal reached
		if (current->x == goalX && current->y == goalY)
		{
			goalNode = current;
			break;
		}
		
		// Explore neighbors
		std::vector<std::pair<int, int>> neighbors;
		GetNeighbors(current->x, current->y, neighbors);
		
		for (const std::pair<int, int>& neighbor : neighbors)
		{
			int nx = neighbor.first;
			int ny = neighbor.second;
			int neighborKey = encodePos(nx, ny);
			
			// Skip if invalid or already processed
			if (!IsValidGridPosition(nx, ny) || !IsNavigable(nx, ny, layer))
			{
				continue;
			}
			
			if (closedSet.find(neighborKey) != closedSet.end())
			{
				continue;
			}
			
			// Calculate tentative gCost
			float moveCost = GetTraversalCost(nx, ny, layer);
			float tentativeG = current->gCost + moveCost;
			
			// Check if we found a better path to this neighbor
			PathNode* neighborNode = nullptr;
			if (allNodes.find(neighborKey) != allNodes.end())
			{
				neighborNode = allNodes[neighborKey];
				if (tentativeG >= neighborNode->gCost)
				{
					continue; // Not a better path
				}
			}
			else
			{
				neighborNode = new PathNode(nx, ny);
				allNodes[neighborKey] = neighborNode;
			}
			
			// Update node
			neighborNode->gCost = tentativeG;
			neighborNode->hCost = Heuristic(nx, ny, goalX, goalY);
			neighborNode->parent = current;
			
			openSet.push(neighborNode);
		}
	}
	
	// Reconstruct path
	bool pathFound = false;
	if (goalNode != nullptr)
	{
		std::vector<PathNode*> pathNodes;
		PathNode* current = goalNode;
		while (current != nullptr)
		{
			pathNodes.push_back(current);
			current = current->parent;
		}
		
		// Reverse to get start->goal order
		std::reverse(pathNodes.begin(), pathNodes.end());
		
		// Convert to world coordinates
		for (PathNode* node : pathNodes)
		{
			float worldX, worldY;
			GridToWorld(node->x, node->y, worldX, worldY);
			outPath.push_back(Vector(worldX, worldY, 0.0f));
		}
		
		pathFound = true;
	}
	
	// Cleanup
	for (std::pair<const int, PathNode*>& entry : allNodes)
	{
		delete entry.second;
	}
	
	return pathFound;
}

void NavigationMap::Clear()
{
	m_width = 0;
	m_height = 0;
	m_numLayers = 1;
	m_activeLayer = CollisionLayer::Ground;
}

bool NavigationMap::GetRandomNavigablePoint(float centerX, float centerY, float radius,
                                             int maxAttempts, float& outX, float& outY,
                                             CollisionLayer layer) const
{
    for (int attempt = 0; attempt < maxAttempts; ++attempt)
    {
        // Générer angle aléatoire (0 à 2π)
        float angle = (static_cast<float>(rand()) / static_cast<float>(RAND_MAX)) * 2.0f * 3.14159265f;
        
        // Générer distance aléatoire (0 à radius)
        // Utiliser sqrt pour distribution uniforme
        float randomRadius = std::sqrt(static_cast<float>(rand()) / static_cast<float>(RAND_MAX)) * radius;
        
        // Calculer position mondiale
        float worldX = centerX + randomRadius * std::cos(angle);
        float worldY = centerY + randomRadius * std::sin(angle);
        
        // Convertir en coordonnées grille
        int gridX, gridY;
        WorldToGrid(worldX, worldY, gridX, gridY);
        
        // Vérifier si navigable
        if (IsValidGridPosition(gridX, gridY) && IsNavigable(gridX, gridY, layer))
        {
            outX = worldX;
            outY = worldY;
            return true;
        }
    }
    
    // Échec après maxAttempts tentatives
    return false;
}
