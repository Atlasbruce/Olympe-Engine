/**
 * @file VSGraphExecutor.h
 * @brief Exécuteur de graphes ATS Visual Scripting (flowchart).
 * @author Olympe Engine
 * @date 2026-03-08
 *
 * @details
 * VSGraphExecutor implémente la logique d'exécution "par frame" d'un graphe
 * de type VisualScript (schema v4). Il est appelé par TaskSystem::Process()
 * pour chaque entité dont le TaskGraphTemplate a GraphType == "VisualScript".
 *
 * Modèle d'exécution :
 *   - Chaque frame, l'exécution repart depuis le node actif (CurrentNodeID).
 *   - Pour EntryPoint : avance immédiatement vers le premier successeur exec.
 *   - Pour Branch : évalue la condition via ConditionRegistry, suit True ou False.
 *   - Pour VSSequence : exécute les ChildrenIDs dans l'ordre (SequenceChildIndex).
 *   - Pour AtomicTask : délègue à IAtomicTask::ExecuteWithContext(), gère Running.
 *   - Pour Delay : accumule StateTimer, émet Completed quand >= DelaySeconds.
 *   - Pour DoOnce : vérifie DoOnceFlags[nodeID], bloque si déjà exécuté.
 *   - Pour GetBBValue / SetBBValue : lit/écrit dans LocalBlackboard via BBKey (scope:key).
 *   - Pour SubGraph : charge et délègue à un VSGraphExecutor récursif (depth-limited).
 *   - Pour While : évalue condition, suit Loop ou Completed.
 *   - Pour Switch : évalue valeur BB, route vers le case correspondant.
 *   - Pour MathOp : calcule résultat via DataPinCache, stocke dans output pin.
 *
 * Résolution des Data Pins :
 *   Avant d'exécuter un node, VSGraphExecutor résout toutes ses DataConnections
 *   entrantes en parcourant les DataConnections du template et en peuplant
 *   runner.DataPinCache avec les valeurs des output pins des nodes sources.
 *
 * C++14 compliant.
 */

#pragma once

#include <cstdint>
#include <string>

#include "TaskGraphTemplate.h"
#include "TaskGraphTypes.h"
#include "../ECS/Components/TaskRunnerComponent.h"
#include "AtomicTaskContext.h"
#include "LocalBlackboard.h"

namespace Olympe {

// Forward declarations
class World;

/**
 * @struct SubGraphCallStack
 * @brief Tracks the SubGraph call chain to detect cycles and enforce depth limits.
 *
 * @details
 * Each SubGraph invocation pushes its file path onto PathStack before executing
 * and pops it after.  This allows O(n) cycle detection (n = depth) and
 * guarantees that HandleSubGraph never recurses deeper than MAX_SUBGRAPH_DEPTH.
 */
struct SubGraphCallStack {
    std::vector<std::string> PathStack; ///< Stack of SubGraph file paths currently executing
    int32_t Depth = 0;                  ///< Current recursion depth

    bool Contains(const std::string& path) const {
        for (size_t i = 0; i < PathStack.size(); ++i)
        {
            if (PathStack[i] == path) return true;
        }
        return false;
    }

    void Push(const std::string& path) {
        PathStack.push_back(path);
        ++Depth;
    }

    void Pop() {
        if (!PathStack.empty()) {
            PathStack.pop_back();
            --Depth;
        }
    }
};

/**
 * @class VSGraphExecutor
 * @brief Exécute un graphe ATS Visual Scripting pour une entité sur un frame.
 */
class VSGraphExecutor {
public:

    /**
     * @brief Point d'entrée principal : exécute le graphe pour une entité.
     *
     * @param entity   ID de l'entité propriétaire du runner.
     * @param runner   Composant runtime mutable de l'entité (CurrentNodeID, BB, etc.).
     * @param tmpl     Template immutable du graphe.
     * @param localBB  Blackboard local de l'entité (initialisé depuis tmpl->Blackboard).
     * @param worldPtr Pointeur vers World (peut être nullptr en tests).
     * @param dt       Delta-time du frame courant.
     */
    static void ExecuteFrame(EntityID entity,
                             TaskRunnerComponent& runner,
                             const TaskGraphTemplate& tmpl,
                             LocalBlackboard& localBB,
                             World* worldPtr,
                             float dt);

private:

    // -----------------------------------------------------------------------
    // Node handlers (un par TaskNodeType VS)
    // -----------------------------------------------------------------------

    /// Résout les data pins entrantes d'un node et peuple runner.DataPinCache.
    static void ResolveDataPins(int32_t nodeID,
                                TaskRunnerComponent& runner,
                                const TaskGraphTemplate& tmpl,
                                LocalBlackboard& localBB);

    /// Retourne l'ID du node cible sur un exec pin nommé `pinName` depuis `sourceNodeID`.
    /// Retourne NODE_INDEX_NONE si aucune connexion trouvée.
    static int32_t FindExecTarget(int32_t sourceNodeID,
                                  const std::string& pinName,
                                  const TaskGraphTemplate& tmpl);

    /// Lit une valeur BB depuis localBB avec support du scope "local:" et "global:".
    static TaskValue ReadBBValue(const std::string& scopedKey,
                                 LocalBlackboard& localBB);

    /// Écrit une valeur BB dans localBB avec support du scope "local:".
    static void WriteBBValue(const std::string& scopedKey,
                             const TaskValue& value,
                             LocalBlackboard& localBB);

    // Node-type handlers — retournent l'ID du prochain node à exécuter
    // (NODE_INDEX_NONE = fin de frame, garde currentNodeID inchangé pour frame suivant)

    static int32_t HandleEntryPoint(int32_t nodeID, const TaskGraphTemplate& tmpl);

    static int32_t HandleBranch(int32_t nodeID,
                                TaskRunnerComponent& runner,
                                const TaskGraphTemplate& tmpl,
                                LocalBlackboard& localBB);

    static int32_t HandleSwitch(int32_t nodeID,
                                TaskRunnerComponent& runner,
                                const TaskGraphTemplate& tmpl,
                                LocalBlackboard& localBB);

    static int32_t HandleVSSequence(int32_t nodeID,
                                    TaskRunnerComponent& runner,
                                    const TaskGraphTemplate& tmpl);

    static int32_t HandleWhile(int32_t nodeID,
                               TaskRunnerComponent& runner,
                               const TaskGraphTemplate& tmpl,
                               LocalBlackboard& localBB);

    static int32_t HandleDoOnce(int32_t nodeID,
                                TaskRunnerComponent& runner,
                                const TaskGraphTemplate& tmpl);

    static int32_t HandleDelay(int32_t nodeID,
                               TaskRunnerComponent& runner,
                               const TaskGraphTemplate& tmpl,
                               float dt);

    static int32_t HandleGetBBValue(int32_t nodeID,
                                    TaskRunnerComponent& runner,
                                    const TaskGraphTemplate& tmpl,
                                    LocalBlackboard& localBB);

    static int32_t HandleSetBBValue(int32_t nodeID,
                                    TaskRunnerComponent& runner,
                                    const TaskGraphTemplate& tmpl,
                                    LocalBlackboard& localBB);

    static int32_t HandleMathOp(int32_t nodeID,
                                TaskRunnerComponent& runner,
                                const TaskGraphTemplate& tmpl);

    static int32_t HandleAtomicTask(EntityID entity,
                                    int32_t nodeID,
                                    TaskRunnerComponent& runner,
                                    const TaskGraphTemplate& tmpl,
                                    LocalBlackboard& localBB,
                                    World* worldPtr,
                                    float dt);

    // SubGraph: depth-limited recursive execution with cycle detection.
    // callStack tracks the currently active SubGraph paths to detect cycles.
    static int32_t HandleSubGraph(EntityID entity,
                                  int32_t nodeID,
                                  TaskRunnerComponent& runner,
                                  const TaskGraphTemplate& tmpl,
                                  LocalBlackboard& localBB,
                                  World* worldPtr,
                                  float dt,
                                  SubGraphCallStack& callStack);

    // Limite de récursion pour SubGraph
    static const int MAX_SUBGRAPH_DEPTH = 4;
};

} // namespace Olympe
