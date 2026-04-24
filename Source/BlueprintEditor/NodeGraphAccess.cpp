/**
 * @file NodeGraphAccess.cpp
 * @brief Implementation of safe graph access wrapper
 */

#include "NodeGraphAccess.h"
#include "BTNodeGraphManager.h"
#include <vector>

namespace Olympe {

std::vector<GraphNode*> GraphAccessor::GetActiveGraphNodes()
{
    std::vector<GraphNode*> result;
    
    // This would need the inner NodeGraph class definition
    // For now, return empty to avoid including problematic headers
    // Phase 50.4: Implement proper graph data access
    
    return result;
}

std::vector<GraphLink> GraphAccessor::GetActiveGraphLinks()
{
    std::vector<GraphLink> result;
    
    // Phase 50.4: Implement proper graph link access
    
    return result;
}

int GraphAccessor::GetActiveGraphId()
{
    auto& mgr = NodeGraphManager::Get();
    return mgr.GetActiveGraphId();
}

bool GraphAccessor::HasActiveGraph()
{
    auto& mgr = NodeGraphManager::Get();
    return mgr.GetActiveGraphId() >= 0;
}

}
