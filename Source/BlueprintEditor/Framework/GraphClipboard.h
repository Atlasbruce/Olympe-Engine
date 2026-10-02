/**
 * @file GraphClipboard.h
 * @brief Type-safe clipboard contract shared by graph editors.
 */

#pragma once

#include <memory>
#include <string>

namespace Olympe {

/**
 * Graph types own their payload schema; the framework owns its lifetime and
 * exposes a common copy/paste hand-off point.
 */
class IGraphClipboardPayload {
public:
    virtual ~IGraphClipboardPayload() = default;
    virtual const char* GetGraphTypeId() const = 0;
};

using GraphClipboardPayloadPtr = std::shared_ptr<const IGraphClipboardPayload>;

} // namespace Olympe
