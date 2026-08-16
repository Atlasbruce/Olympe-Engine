#pragma once

#include "../../system/system_utils.h"
#include <string>

namespace Olympe {

struct BlueprintDropContext
{
    std::string graphType;
    std::string payloadType;
    std::string payloadValue;
    float screenX = 0.0f;
    float screenY = 0.0f;
};

struct BlueprintDropResult
{
    bool handled = false;
    std::string createdNodeId;
};

inline void LogBlueprintDropBegin(const BlueprintDropContext& ctx)
{
    SYSTEM_LOG << "[Blueprint Framework] Begin drag item=" << ctx.payloadValue
               << " payloadType=" << ctx.payloadType
               << " graphType=" << ctx.graphType << "\n";
}

inline void LogBlueprintDropReceived(const BlueprintDropContext& ctx)
{
    SYSTEM_LOG << "[Blueprint Framework] Canvas drop received item=" << ctx.payloadValue
               << " payloadType=" << ctx.payloadType
               << " graphType=" << ctx.graphType
               << " screen=(" << ctx.screenX << "," << ctx.screenY << ")\n";
}

inline void LogBlueprintDropCreate(const BlueprintDropContext& ctx)
{
    SYSTEM_LOG << "[Blueprint Framework] Creating node from drop item=" << ctx.payloadValue
               << " graphType=" << ctx.graphType << "\n";
}

inline void LogBlueprintDropCreated(const BlueprintDropContext& ctx, const std::string& nodeIdText)
{
    SYSTEM_LOG << "[Blueprint Framework] Node created id=" << nodeIdText
               << " item=" << ctx.payloadValue
               << " graphType=" << ctx.graphType << "\n";
}

inline void LogBlueprintDropIgnored(const BlueprintDropContext& ctx)
{
    SYSTEM_LOG << "[Blueprint Framework] Drop ignored item=" << ctx.payloadValue
               << " payloadType=" << ctx.payloadType
               << " graphType=" << ctx.graphType << "\n";
}

} // namespace Olympe

