/**
 * @file DynamicDataPin.cpp
 * @brief Implementation of DynamicDataPin — construction, helpers, serialization, UUID gen.
 * @author Olympe Engine
 * @date 2026-03-16
 *
 * C++14 compliant — no std::optional, structured bindings, std::filesystem.
 */

#include "DynamicDataPin.h"

#include <cstdint>
#include <ctime>
#include <sstream>
#include <iomanip>
#include <string>

// ---------------------------------------------------------------------------
// Pseudo-random seed helpers (C++14 compatible, no <random> state objects
// stored in static locals to keep thread-safety simple for a single-threaded
// editor context).
// ---------------------------------------------------------------------------
#include <cstdlib>

namespace Olympe {

// ---------------------------------------------------------------------------
// Default constructor
// ---------------------------------------------------------------------------

DynamicDataPin::DynamicDataPin()
    : id(GenerateUniqueID())
    , conditionIndex(0)
    , position(OperandPosition::Left)
    , label("")
    , nodePinID("")
    , dataValue(0.0f)
{
}

// ---------------------------------------------------------------------------
// Convenience constructor
// ---------------------------------------------------------------------------

DynamicDataPin::DynamicDataPin(int condIdx, OperandPosition pos,
                               const std::string& condPreview)
    : id(GenerateUniqueID())
    , conditionIndex(condIdx)
    , position(pos)
    , nodePinID("")
    , dataValue(0.0f)
{
    // Build the label using the same format that GetDisplayLabel() produces.
    std::string side = (pos == OperandPosition::Left) ? "L" : "R";
    std::ostringstream oss;
    oss << "In #" << (condIdx + 1) << side << ": " << condPreview;
    label = oss.str();
}

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

std::string DynamicDataPin::GetDisplayLabel() const
{
    return label;
}

/*static*/
std::string DynamicDataPin::GenerateUniqueID()
{
    // Simple UUID v4-style generator using stdlib rand().
    // In a production context a proper UUID library would be preferred, but
    // this suffices for C++14 in a single-threaded editor tool.
    static bool seeded = false;
    if (!seeded)
    {
        std::srand(static_cast<unsigned int>(std::time(nullptr)));
        seeded = true;
    }

    // Generate 16 random bytes and format as xxxxxxxx-xxxx-4xxx-yxxx-xxxxxxxxxxxx
    unsigned int b[16];
    for (int i = 0; i < 16; ++i)
        b[i] = static_cast<unsigned int>(std::rand()) & 0xFF;

    // Set version (4) and variant bits
    b[6] = (b[6] & 0x0F) | 0x40;
    b[8] = (b[8] & 0x3F) | 0x80;

    std::ostringstream oss;
    oss << std::hex << std::setfill('0');

    for (int i = 0; i < 16; ++i)
    {
        if (i == 4 || i == 6 || i == 8 || i == 10)
            oss << '-';
        oss << std::setw(2) << b[i];
    }

    return "pin_" + oss.str();
}

// ---------------------------------------------------------------------------
// Serialization
// ---------------------------------------------------------------------------

nlohmann::json DynamicDataPin::ToJson() const
{
    nlohmann::json j = nlohmann::json::object();
    j["id"]             = id;
    j["conditionIndex"] = conditionIndex;
    j["position"]       = (position == OperandPosition::Left) ? "Left" : "Right";
    j["label"]          = label;
    j["nodePinID"]      = nodePinID;
    j["dataValue"]      = dataValue;
    return j;
}

/*static*/
DynamicDataPin DynamicDataPin::FromJson(const nlohmann::json& data)
{
    DynamicDataPin pin;

    if (!data.is_object())
        return pin;

    if (data.contains("id") && data["id"].is_string())
        pin.id = data["id"].get<std::string>();

    if (data.contains("conditionIndex") && data["conditionIndex"].is_number_integer())
        pin.conditionIndex = data["conditionIndex"].get<int>();

    if (data.contains("position") && data["position"].is_string())
    {
        std::string pos = data["position"].get<std::string>();
        pin.position = (pos == "Right") ? OperandPosition::Right : OperandPosition::Left;
    }

    if (data.contains("label") && data["label"].is_string())
        pin.label = data["label"].get<std::string>();

    if (data.contains("nodePinID") && data["nodePinID"].is_string())
        pin.nodePinID = data["nodePinID"].get<std::string>();

    if (data.contains("dataValue") && data["dataValue"].is_number())
        pin.dataValue = data["dataValue"].get<float>();

    return pin;
}

} // namespace Olympe
