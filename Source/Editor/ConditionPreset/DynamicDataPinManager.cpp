/**
 * @file DynamicDataPinManager.cpp
 * @brief Implementation of DynamicDataPinManager (Phase 24.3).
 * @author Olympe Engine
 * @date 2026-03-17
 *
 * C++14 compliant — no std::optional, structured bindings, std::filesystem.
 */

#include "DynamicDataPinManager.h"

#include <algorithm>
#include <sstream>

namespace Olympe {

// ============================================================================
// Constructor
// ============================================================================

DynamicDataPinManager::DynamicDataPinManager(ConditionPresetRegistry& registry)
    : m_registry(registry)
{
}

// ============================================================================
// SyncPins
// ============================================================================

void DynamicDataPinManager::SyncPins(std::vector<NodeConditionRef>& conditionRefs)
{
    // Build the set of (conditionIndex, position) pairs that SHOULD exist
    // after this sync, and map each pair to the condition preview string.
    struct PinSpec {
        int            conditionIndex;
        OperandPosition position;
        std::string    condPreview;
        std::string    presetID;
    };

    std::vector<PinSpec> needed;

    for (int i = 0; i < static_cast<int>(conditionRefs.size()); ++i)
    {
        const NodeConditionRef& ref = conditionRefs[i];
        const ConditionPreset*  preset = m_registry.GetPreset(ref.presetID);
        if (!preset)
            continue;

        std::string preview = preset->GetPreview();

        if (preset->NeedsLeftPin())
            needed.push_back({i, OperandPosition::Left, preview, ref.presetID});

        if (preset->NeedsRightPin())
            needed.push_back({i, OperandPosition::Right, preview, ref.presetID});
    }

    // Build new ordered pin list, reusing existing UUIDs where possible.
    std::vector<DynamicDataPin> newPins;
    newPins.reserve(needed.size());

    for (const auto& spec : needed)
    {
        DynamicDataPin* existing = FindExistingPin(spec.conditionIndex, spec.position);

        DynamicDataPin pin;
        if (existing)
        {
            // Reuse UUID and other immutable fields; refresh label in case
            // the preset preview changed.
            pin = *existing;
            // Rebuild the label with the (possibly updated) preview
            std::string side = (spec.position == OperandPosition::Left) ? "L" : "R";
            std::ostringstream oss;
            oss << "In #" << (spec.conditionIndex + 1) << side
                << ": " << spec.condPreview;
            pin.label = oss.str();
        }
        else
        {
            pin = DynamicDataPin(spec.conditionIndex, spec.position,
                                 spec.condPreview);
        }

        newPins.push_back(pin);
    }

    // Replace stored pins and rebuild index maps.
    m_pins = std::move(newPins);
    m_idIndex.clear();
    m_keyIndex.clear();

    for (size_t idx = 0; idx < m_pins.size(); ++idx)
    {
        DynamicDataPin& p = m_pins[idx];
        // Assign 1-based sequence number for "Pin-in #N" short labels.
        p.sequenceNumber = static_cast<int>(idx) + 1;
        m_idIndex[p.id]                                           = idx;
        m_keyIndex[MakePinKey(p.conditionIndex, p.position)]     = idx;
    }

    // Write back leftPinID / rightPinID into the condition refs.
    for (auto& ref : conditionRefs)
    {
        ref.leftPinID  = "";
        ref.rightPinID = "";
    }

    for (const auto& pin : m_pins)
    {
        int ci = pin.conditionIndex;
        if (ci < 0 || ci >= static_cast<int>(conditionRefs.size()))
            continue;

        if (pin.position == OperandPosition::Left)
            conditionRefs[ci].leftPinID = pin.id;
        else
            conditionRefs[ci].rightPinID = pin.id;
    }
}

// ============================================================================
// RegeneratePinsFromConditions
// ============================================================================

void DynamicDataPinManager::RegeneratePinsFromConditions(
    std::vector<NodeConditionRef>& conditionRefs)
{
    SyncPins(conditionRefs);
}

// ============================================================================
// Query
// ============================================================================

const std::vector<DynamicDataPin>& DynamicDataPinManager::GetAllPins() const
{
    return m_pins;
}

const DynamicDataPin* DynamicDataPinManager::GetPinByID(const std::string& pinID) const
{
    auto it = m_idIndex.find(pinID);
    if (it == m_idIndex.end())
        return nullptr;
    return &m_pins[it->second];
}

DynamicDataPin* DynamicDataPinManager::GetPinByID(const std::string& pinID)
{
    auto it = m_idIndex.find(pinID);
    if (it == m_idIndex.end())
        return nullptr;
    return &m_pins[it->second];
}

std::vector<const DynamicDataPin*>
DynamicDataPinManager::GetPinsForCondition(int conditionIndex) const
{
    std::vector<const DynamicDataPin*> result;
    for (const auto& pin : m_pins)
    {
        if (pin.conditionIndex == conditionIndex)
            result.push_back(&pin);
    }
    return result;
}

size_t DynamicDataPinManager::GetPinCount() const
{
    return m_pins.size();
}

// ============================================================================
// Invalidation
// ============================================================================

void DynamicDataPinManager::InvalidatePreset(const std::string& deletedPresetID)
{
    // Find indices of pins that belong to conditions referencing deletedPresetID.
    // Since we don't store the presetID per-pin, we remove pins only if the
    // associated preset's NeedsLeftPin / NeedsRightPin is determined by looking
    // up the preset — but the preset is being deleted.  Instead, we track which
    // condition indices are affected and remove their pins.
    //
    // Because the caller will typically call SyncPins() with the updated
    // condition list immediately after InvalidatePreset(), we do a best-effort
    // removal of any pin whose UUID is not found in a valid preset any more.
    // In practice: remove ALL pins that would no longer be generated.

    // Simple strategy: remove all pins whose id appears in our map and whose
    // preset is gone from the registry.  Since the preset IS being deleted we
    // can't look it up; instead just clear everything — SyncPins() will rebuild.
    (void)deletedPresetID;

    // Remove all pins whose condition-index points to a condition whose preset
    // ID equals deletedPresetID.  Since we don't store presetID in DynamicDataPin
    // we clear all pins and let the next SyncPins() call rebuild from scratch.
    Clear();
}

void DynamicDataPinManager::Clear()
{
    m_pins.clear();
    m_idIndex.clear();
    m_keyIndex.clear();
}

// ============================================================================
// Color helper
// ============================================================================

/*static*/
void DynamicDataPinManager::GetDynamicPinColor(float& r, float& g,
                                                float& b, float& a)
{
    r = 1.0f;
    g = 1.0f;
    b = 0.0f;
    a = 1.0f;
}

// ============================================================================
// Internal helpers
// ============================================================================

DynamicDataPin*
DynamicDataPinManager::FindExistingPin(int conditionIndex, OperandPosition pos)
{
    auto it = m_keyIndex.find(MakePinKey(conditionIndex, pos));
    if (it == m_keyIndex.end())
        return nullptr;
    return &m_pins[it->second];
}

/*static*/
std::string DynamicDataPinManager::MakePinKey(int conditionIndex,
                                               OperandPosition pos)
{
    std::ostringstream oss;
    oss << conditionIndex << '_'
        << ((pos == OperandPosition::Left) ? 'L' : 'R');
    return oss.str();
}

} // namespace Olympe
