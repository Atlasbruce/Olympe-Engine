/**
 * @file BTGraphCompiler.h
 * @brief Compiler from GraphDocument to BehaviorTreeAsset
 * @author Olympe Engine
 * @date 2026-02-18
 *
 * @details
 * Compiles a NodeGraphCore GraphDocument into a runtime BehaviorTreeAsset.
 * Performs validation before compilation and maps node types appropriately.
 */

#pragma once

#include "../../NodeGraphCore/GraphDocument.h"
#include "../BehaviorTree.h"
#include <string>

namespace Olympe {
namespace AI {

/**
 * @class BTGraphCompiler
 * @brief Static compiler for BT graphs
 *
 * @details
 * Compiles GraphDocument (editor format) to BehaviorTreeAsset (runtime format).
 * Validates graph before compilation and provides error messages on failure.
 */
class BTGraphCompiler {
public:
    /**
     * @brief Compile GraphDocument to BehaviorTreeAsset
     * @param graph Source graph document
     * @param outAsset Output asset (cleared and populated)
     * @param errorMsg Error message if compilation fails
     * @return true if successful, false otherwise
     */
    static bool Compile(
        const NodeGraph::GraphDocument* graph,
        BehaviorTreeAsset& outAsset,
        std::string& errorMsg
    );
    
private:
    /**
     * @brief Compile a single node from graph to asset
     * @param graphNode Source node data
     * @param outNode Output BT node
     * @return true if successful
     */
    static bool CompileNode(
        const NodeGraph::NodeData& graphNode,
        BTNode& outNode
    );
    
    /**
     * @brief Map graph node type string to BTNodeType enum
     * @param typeString Type name from graph
     * @return Corresponding BTNodeType
     */
    static BTNodeType MapNodeType(const std::string& typeString);
};

} // namespace AI
} // namespace Olympe
