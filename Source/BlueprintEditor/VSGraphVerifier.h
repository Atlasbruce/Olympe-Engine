/**
 * @file VSGraphVerifier.h
 * @brief Global graph verifier for ATS Visual Script graphs (Phase 21-A).
 * @author Olympe Engine
 * @date 2026-03-14
 *
 * @details
 * Stateless global validator that checks the full TaskGraphTemplate for
 * structural, type-safety and blackboard consistency issues.
 * Produces a VSVerificationResult with Error/Warning/Info issues.
 *
 * C++14 compliant — no std::optional, structured bindings, std::filesystem.
 */

#pragma once

#include <string>
#include <vector>
#include "../TaskSystem/TaskGraphTemplate.h"

namespace Olympe {

// ============================================================================
// VSVerificationIssue
// ============================================================================

enum class VSVerificationSeverity {
    Error,
    Warning,
    Info
};

struct VSVerificationIssue {
    VSVerificationSeverity severity;
    int                    nodeID;    ///< -1 if not node-specific
    std::string            ruleID;    ///< e.g. "E001_NoEntryPoint"
    std::string            message;
};

// ============================================================================
// VSVerificationResult
// ============================================================================

struct VSVerificationResult {
    std::vector<VSVerificationIssue> issues;

    bool HasErrors()   const;
    bool HasWarnings() const;
    bool IsValid()     const; ///< true if no Error issues
};

// ============================================================================
// VSGraphVerifier
// ============================================================================

/**
 * @class VSGraphVerifier
 * @brief Stateless global verifier for ATS Visual Script graphs.
 *
 * @details
 * Call VSGraphVerifier::Verify(graph) to run all 14 rules on the graph.
 * The result contains zero or more VSVerificationIssue entries.
 *
 * Rules implemented:
 *   E001 — Exactly one EntryPoint node required
 *   E002 — Dangling node (no exec in or out, except EntryPoint)
 *   E003 — Exec cycle detected (iterative DFS)
 *   E004 — Circular SubGraph reference
 *   E005 — Exec connection references unknown node
 *   E006 — Incompatible data pin types
 *   E007 — Inverted pin direction (output connected to output, or input to input)
 *   E008 — Unknown Blackboard key in GetBBValue/SetBBValue
 *   E009 — Blackboard type mismatch
 *   E010 — Switch node missing switchVariable
 *   E011 — Switch node has duplicate case values
 *   E012 — Switch node has a case with empty pin name
 *   W001 — AtomicTask with empty AtomicTaskID
 *   W002 — Delay with DelaySeconds <= 0
 *   W003 — SubGraph with empty SubGraphPath
 *   W004 — MathOp with empty MathOperator
 *   I001 — Node not reachable from EntryPoint
 */
class VSGraphVerifier {
public:
    /**
     * @brief Run all verification rules on the given graph.
     * @param graph  The graph to verify (read-only).
     * @return VSVerificationResult containing all issues found.
     */
    static VSVerificationResult Verify(const TaskGraphTemplate& graph);

private:
    // Structural rules
    static void CheckEntryPoint(const TaskGraphTemplate& g, VSVerificationResult& r);
    static void CheckDanglingNodes(const TaskGraphTemplate& g, VSVerificationResult& r);
    static void CheckExecCycles(const TaskGraphTemplate& g, VSVerificationResult& r);
    static void CheckSubGraphCircular(const TaskGraphTemplate& g, VSVerificationResult& r);

    // Type-safety rules
    static void CheckExecPinTypes(const TaskGraphTemplate& g, VSVerificationResult& r);
    static void CheckDataPinTypes(const TaskGraphTemplate& g, VSVerificationResult& r);
    static void CheckPinDirections(const TaskGraphTemplate& g, VSVerificationResult& r);

    // Blackboard rules
    static void CheckBlackboardKeys(const TaskGraphTemplate& g, VSVerificationResult& r);
    static void CheckBlackboardTypes(const TaskGraphTemplate& g, VSVerificationResult& r);

    // Switch rules (Phase 22-A)
    static void CheckSwitchNodes(const TaskGraphTemplate& g, VSVerificationResult& r);

    // Warning rules
    static void CheckNodeParameterWarnings(const TaskGraphTemplate& g, VSVerificationResult& r);

    // Info rules
    static void CheckReachability(const TaskGraphTemplate& g, VSVerificationResult& r);

    // Helpers
    static void AddIssue(VSVerificationResult& r,
                         VSVerificationSeverity sev,
                         int nodeID,
                         const std::string& ruleID,
                         const std::string& message);

    VSGraphVerifier() = delete;
};

} // namespace Olympe
