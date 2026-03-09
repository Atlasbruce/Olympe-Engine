/**
 * @file AsyncJSONLoader.h
 * @brief Asynchronous JSON file loader (Phase 7).
 * @author Olympe Engine
 * @date 2026-03-10
 *
 * @details
 * AsyncJSONLoader launches a background thread (via std::future) to parse a
 * JSON file without blocking the editor UI thread.  Poll IsReady() / HasFailed()
 * each frame; when ready, call GetResult() to obtain the parsed JSON object.
 *
 * C++14 compliant — no std::optional, structured bindings, std::filesystem.
 */

#pragma once

#include <string>
#include <future>

#include "../../third_party/nlohmann/json.hpp"

namespace Olympe {

// ============================================================================
// AsyncLoadState
// ============================================================================

/**
 * @enum AsyncLoadState
 * @brief Lifecycle state of an AsyncJSONLoader.
 */
enum class AsyncLoadState {
    Idle,     ///< No load has been requested
    Loading,  ///< Background thread is running
    Ready,    ///< Result is available — call GetResult()
    Failed    ///< Load or parse failed — call GetLastError()
};

// ============================================================================
// AsyncJSONLoader
// ============================================================================

/**
 * @class AsyncJSONLoader
 * @brief Loads and parses a JSON file on a background thread.
 *
 * Typical usage:
 * @code
 *   AsyncJSONLoader loader;
 *   loader.LoadAsync("data/graph.json");
 *   // ... each frame ...
 *   if (loader.IsReady())
 *   {
 *       nlohmann::json j = loader.GetResult();
 *       loader.Reset();
 *   }
 * @endcode
 */
class AsyncJSONLoader {
public:

    AsyncJSONLoader();

    // -----------------------------------------------------------------------
    // Control
    // -----------------------------------------------------------------------

    /**
     * @brief Starts loading @p path on a background thread.
     *        Any previous result is discarded.
     */
    void LoadAsync(const std::string& path);

    /**
     * @brief Returns the current state of the loader.
     *        Transitions: Idle -> Loading -> Ready | Failed.
     */
    AsyncLoadState GetState() const;

    /**
     * @brief Returns true if the result is available (state == Ready).
     */
    bool IsReady()   const;

    /**
     * @brief Returns true if the load failed (state == Failed).
     */
    bool HasFailed() const;

    /**
     * @brief Returns the parsed JSON if ready; returns an empty object otherwise.
     *
     * @note Calling GetResult() does NOT reset the loader.  Call Reset()
     *       explicitly when you are done with the result.
     */
    nlohmann::json GetResult();

    /**
     * @brief Resets state to Idle and clears the result / error.
     */
    void Reset();

    /**
     * @brief Returns the last error message (non-empty when state == Failed).
     */
    const std::string& GetLastError() const;

private:

    /**
     * @brief Opens, reads, and parses the JSON file; called on the background thread.
     * @return Parsed JSON object; throws on error.
     */
    static nlohmann::json LoadFile(const std::string& path);

    // Mutable because GetState()/IsReady() need to poll the future.
    mutable AsyncLoadState m_State;
    std::string            m_LastError;
    nlohmann::json         m_Result;
    mutable std::future<nlohmann::json> m_Future;
};

} // namespace Olympe
