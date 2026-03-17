/**
 * @file DynamicDataPinManager.h
 * @brief Engine for generating and tracking dynamic data pins (Phase 24.3).
 * @author Olympe Engine
 * @date 2026-03-17
 *
 * @details
 * DynamicDataPinManager is responsible for keeping the set of DynamicDataPins
 * on a NodeBranch in sync with its NodeConditionRef list.  Whenever the node's
 * conditions change (preset added, removed, or updated), the manager
 * regenerates the pin set to match.
 *
 * Design constraints:
 *   - Pin UUIDs are stable between calls to SyncPins() as long as the
 *     condition-index and operand-side are the same.  This allows the host to
 *     persist pin IDs across saves.
 *   - O(1) pin lookup by UUID via an internal `std::map`.
 *   - Color coding: dynamic data pins are yellow (ImVec4(1,1,0,1)).
 *   - Labels use the format: "In #<N><L|R>: <condPreview>"
 *     e.g. "In #3L: [Pin:1] <= [Pin:2]"
 *
 * Usage:
 * @code
 *   DynamicDataPinManager mgr(registry);
 *   mgr.SyncPins(node.conditions);          // regenerate / keep existing
 *   node.dynamicPins = mgr.GetAllPins();    // write back to node
 *
 *   // When a preset is deleted:
 *   mgr.InvalidatePreset(deletedPresetID);
 *   node.dynamicPins = mgr.GetAllPins();
 * @endcode
 *
 * C++14 compliant — no std::optional, structured bindings, std::filesystem.
 */

#pragma once

#include <string>
#include <vector>
#include <map>

#include "ConditionPreset.h"
#include "ConditionPresetRegistry.h"
#include "NodeConditionRef.h"
#include "DynamicDataPin.h"

namespace Olympe {

/**
 * @class DynamicDataPinManager
 * @brief Generates, tracks, and invalidates DynamicDataPin objects for a node.
 *
 * @details
 * The manager is bound to a single NodeBranch (one per node).  It operates
 * on the node's `conditions` list and produces the matching `dynamicPins` list.
 *
 * Thread safety: single-threaded editor context only.
 */
class DynamicDataPinManager {
public:

    /**
     * @brief Constructs the manager bound to the global preset registry.
     * @param registry  Global ConditionPresetRegistry (must outlive this manager).
     */
    explicit DynamicDataPinManager(ConditionPresetRegistry& registry);

    ~DynamicDataPinManager() = default;

    // Non-copyable (manages unique IDs)
    DynamicDataPinManager(const DynamicDataPinManager&) = delete;
    DynamicDataPinManager& operator=(const DynamicDataPinManager&) = delete;

    // -----------------------------------------------------------------------
    // Synchronization
    // -----------------------------------------------------------------------

    /**
     * @brief Rebuilds the pin set to match the given condition list.
     *
     * The method:
     *   1. Walks every NodeConditionRef in `conditionRefs`.
     *   2. For each one, looks up the ConditionPreset in the registry.
     *   3. If the preset's left operand is Pin-mode → ensures a Left DynamicDataPin
     *      exists for that condition index.
     *   4. Same for the right operand.
     *   5. Removes any existing pins that no longer correspond to a
     *      Pin-mode operand in a valid preset.
     *
     * Pre-existing pin UUIDs are reused when condition-index + operand-side
     * match, so graph connections survive minor edits.
     *
     * Also updates the `leftPinID` and `rightPinID` fields inside each
     * NodeConditionRef in the provided vector.
     *
     * @param conditionRefs  Node's condition list (modified in-place to store
     *                       assigned pin IDs).
     */
    void SyncPins(std::vector<NodeConditionRef>& conditionRefs);

    // -----------------------------------------------------------------------
    // Query
    // -----------------------------------------------------------------------

    /**
     * @brief Returns all current DynamicDataPins in creation order.
     */
    const std::vector<DynamicDataPin>& GetAllPins() const;

    /**
     * @brief Returns a pointer to the pin with the given UUID, or nullptr.
     * @param pinID  UUID to look up (O(1) via internal map).
     */
    const DynamicDataPin* GetPinByID(const std::string& pinID) const;

    /**
     * @brief Returns a mutable pointer to the pin with the given UUID.
     * @param pinID  UUID to look up.
     */
    DynamicDataPin* GetPinByID(const std::string& pinID);

    /**
     * @brief Returns all pins associated with a specific condition index.
     * @param conditionIndex  Zero-based condition index.
     */
    std::vector<const DynamicDataPin*>
    GetPinsForCondition(int conditionIndex) const;

    /**
     * @brief Returns the total number of managed pins.
     */
    size_t GetPinCount() const;

    // -----------------------------------------------------------------------
    // Invalidation
    // -----------------------------------------------------------------------

    /**
     * @brief Removes all pins that belong to the given preset.
     *
     * Should be called when a preset is deleted from the registry so that
     * stale pins are cleaned up.  Also clears leftPinID / rightPinID in any
     * cached condition refs that pointed to the deleted preset's pins.
     *
     * @param deletedPresetID  UUID of the preset being deleted.
     */
    void InvalidatePreset(const std::string& deletedPresetID);

    /**
     * @brief Removes all managed pins.
     */
    void Clear();

    // -----------------------------------------------------------------------
    // ImGui color helper
    // -----------------------------------------------------------------------

    /**
     * @brief Returns the RGBA color used to render dynamic data pins.
     *
     * Always returns yellow: { 1.0f, 1.0f, 0.0f, 1.0f }.
     * The caller can pass this to ImGui::PushStyleColor(ImGuiCol_Text, …).
     */
    static void GetDynamicPinColor(float& r, float& g, float& b, float& a);

private:

    // -----------------------------------------------------------------------
    // Internal helpers
    // -----------------------------------------------------------------------

    /**
     * @brief Looks for an existing pin matching (conditionIndex, position).
     *
     * Returns a pointer into m_pins, or nullptr if not found.
     */
    DynamicDataPin* FindExistingPin(int conditionIndex, OperandPosition pos);

    /**
     * @brief Builds a unique key for the (conditionIndex, position) pair used
     *        in the lookup map.
     */
    static std::string MakePinKey(int conditionIndex, OperandPosition pos);

    // -----------------------------------------------------------------------
    // State
    // -----------------------------------------------------------------------

    ConditionPresetRegistry&         m_registry;  ///< Shared global registry
    std::vector<DynamicDataPin>      m_pins;       ///< Ordered pin list
    std::map<std::string, size_t>    m_idIndex;    ///< UUID → index in m_pins (O(1))
    std::map<std::string, size_t>    m_keyIndex;   ///< (condIdx,side) → index in m_pins
};

} // namespace Olympe
