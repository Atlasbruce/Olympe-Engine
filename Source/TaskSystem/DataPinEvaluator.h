/**
 * @file DataPinEvaluator.h
 * @brief Stack-based recursive evaluation of data pin networks for "data pure" nodes.
 * @author Olympe Engine
 * @date 2026-03-20
 *
 * @details
 * Implements depth-first stack-based evaluation of connected data pins.
 * All implementations are inline to ensure proper compilation.
 *
 * C++14 compliant - no C++17/20 features.
 */

#pragma once

#include <string>
#include <vector>
#include <unordered_set>
#include <unordered_map>
#include <stdexcept>
#include <sstream>
#include <cmath>
#include <limits>

#include "TaskGraphTypes.h"

namespace Olympe {

// Forward declarations
class TaskGraphTemplate;
class LocalBlackboard;
struct TaskRunnerComponent;
struct DataPinConnection;
struct TaskNodeDefinition;

/**
 * @enum DataPinEvalStatus
 * @brief Result status of data pin evaluation.
 */
enum class DataPinEvalStatus {
    Success,        ///< Evaluation completed successfully
    CycleDetected,  ///< Circular dependency detected in data pin network
    InvalidNode,    ///< Node ID not found in template
    EvaluationError ///< Runtime error during evaluation (e.g., type mismatch)
};

/**
 * @struct DataPinEvaluationResult
 * @brief Result of a single data pin evaluation attempt.
 */
struct DataPinEvaluationResult {
    DataPinEvalStatus Status = DataPinEvalStatus::Success;
    TaskValue         Value;
    std::string       ErrorMessage;
};

/**
 * @class DataPinEvaluator
 * @brief Evaluates data pin networks using stack-based depth-first recursion.
 * Inline implementation to avoid linker issues.
 */
class DataPinEvaluator {
public:

    /// Maximum recursion depth to prevent stack overflow
    static constexpr int32_t MAX_RECURSION_DEPTH = 32;

    /**
     * @brief Evaluate all input data pins for a given node before execution.
     * Inline implementation - integrates with VSGraphExecutor.
     */
    static inline bool EvaluateNodeInputPins(
        int32_t nodeID,
        const TaskGraphTemplate& tmpl,
        TaskRunnerComponent& runner,
        LocalBlackboard& localBB);
};

} // namespace Olympe

// Include inline implementation
#include "DataPinEvaluator_inline.h"

