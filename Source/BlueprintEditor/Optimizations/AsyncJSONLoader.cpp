/**
 * @file AsyncJSONLoader.cpp
 * @brief Asynchronous JSON file loader implementation (Phase 7).
 * @author Olympe Engine
 * @date 2026-03-10
 */

#include "AsyncJSONLoader.h"

#include <fstream>
#include <stdexcept>

#include "../../system/system_utils.h"

namespace Olympe {

// ============================================================================
// Construction
// ============================================================================

AsyncJSONLoader::AsyncJSONLoader()
    : m_State(AsyncLoadState::Idle)
{
}

// ============================================================================
// LoadAsync
// ============================================================================

void AsyncJSONLoader::LoadAsync(const std::string& path)
{
    // If a previous load is still in flight, detach it (fire-and-forget).
    // The background thread will finish on its own; we just discard the result.
    if (m_Future.valid())
        m_Future.wait();

    m_Result    = nlohmann::json();
    m_LastError.clear();
    m_State     = AsyncLoadState::Loading;

    m_Future = std::async(std::launch::async,
                          &AsyncJSONLoader::LoadFile,
                          path);
}

// ============================================================================
// GetState
// ============================================================================

AsyncLoadState AsyncJSONLoader::GetState() const
{
    if (m_State == AsyncLoadState::Loading)
    {
        if (!m_Future.valid())
        {
            m_State = AsyncLoadState::Failed;
            return m_State;
        }

        // Poll without blocking.
        std::future_status status =
            m_Future.wait_for(std::chrono::seconds(0));

        if (status == std::future_status::ready)
        {
            // Retrieve result — const_cast needed because future::get() is not const.
            AsyncJSONLoader* self = const_cast<AsyncJSONLoader*>(this);
            try
            {
                self->m_Result = m_Future.get();
                self->m_State  = AsyncLoadState::Ready;
            }
            catch (const std::exception& e)
            {
                self->m_LastError = e.what();
                self->m_State     = AsyncLoadState::Failed;
                SYSTEM_LOG << "[AsyncJSONLoader] Load failed: " << e.what() << std::endl;
            }
            catch (...)
            {
                self->m_LastError = "Unknown error during JSON load.";
                self->m_State     = AsyncLoadState::Failed;
                SYSTEM_LOG << "[AsyncJSONLoader] Load failed with unknown error." << std::endl;
            }
        }
    }

    return m_State;
}

// ============================================================================
// Convenience state queries
// ============================================================================

bool AsyncJSONLoader::IsReady() const
{
    return GetState() == AsyncLoadState::Ready;
}

bool AsyncJSONLoader::HasFailed() const
{
    return GetState() == AsyncLoadState::Failed;
}

// ============================================================================
// GetResult
// ============================================================================

nlohmann::json AsyncJSONLoader::GetResult()
{
    if (GetState() == AsyncLoadState::Ready)
        return m_Result;
    return nlohmann::json();
}

// ============================================================================
// Reset
// ============================================================================

void AsyncJSONLoader::Reset()
{
    if (m_Future.valid())
        m_Future.wait();

    m_State     = AsyncLoadState::Idle;
    m_LastError.clear();
    m_Result    = nlohmann::json();
}

// ============================================================================
// GetLastError
// ============================================================================

const std::string& AsyncJSONLoader::GetLastError() const
{
    return m_LastError;
}

// ============================================================================
// LoadFile (background thread)
// ============================================================================

nlohmann::json AsyncJSONLoader::LoadFile(const std::string& path)
{
    std::ifstream file(path);
    if (!file.is_open())
        throw std::runtime_error("Cannot open file: " + path);

    nlohmann::json j;
    file >> j;
    return j;
}

} // namespace Olympe
