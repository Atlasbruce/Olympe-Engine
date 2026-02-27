/*
 * Olympe Engine - EditorAutosaveManager implementation
 */

#include "EditorAutosaveManager.h"
#include <iostream>
#include <fstream>
#include <chrono>

namespace Olympe
{

void EditorAutosaveManager::Init(std::function<void()> saveFn,
                                  float debounceSec,
                                  float periodicIntervalSec)
{
    m_saveFn              = std::move(saveFn);
    m_debounceSec         = debounceSec;
    m_periodicIntervalSec = periodicIntervalSec;
}

void EditorAutosaveManager::ScheduleSave(double nowSec)
{
    m_dirty            = true;
    m_debounceDeadline = nowSec + static_cast<double>(m_debounceSec);
}

void EditorAutosaveManager::ScheduleSave(double nowSec,
                                          std::function<std::string()> serializeFn,
                                          std::string filePath,
                                          std::string fallbackPrefix)
{
    m_dirty                  = true;
    m_debounceDeadline       = nowSec + static_cast<double>(m_debounceSec);
    m_pendingSerializeFn     = std::move(serializeFn);
    m_pendingFilePath        = std::move(filePath);
    m_pendingFallbackPrefix  = std::move(fallbackPrefix);
}

void EditorAutosaveManager::Tick(double nowSec)
{
    // Require at least one save path to be configured.
    if (!m_saveFn && !m_pendingSerializeFn)
        return;

    // If a previous async task is still running, do not start another one.
    if (m_future.valid() &&
        m_future.wait_for(std::chrono::seconds(0)) != std::future_status::ready)
        return;

    bool shouldSave = false;

    // Debounce: fire once the deadline has passed and there is a pending change.
    if (m_dirty && m_debounceDeadline > 0.0 && nowSec >= m_debounceDeadline)
    {
        shouldSave         = true;
        m_debounceDeadline = -1.0;
    }

    // Periodic flush.
    if (!shouldSave && m_dirty && m_periodicIntervalSec > 0.0f && m_lastSaveTime >= 0.0)
    {
        double elapsed = nowSec - m_lastSaveTime;
        if (elapsed >= static_cast<double>(m_periodicIntervalSec))
            shouldSave = true;
    }

    if (shouldSave)
    {
        m_dirty        = false;
        m_lastSaveTime = nowSec;

        if (m_pendingSerializeFn)
        {
            // Serialize on the calling (UI) thread before handing off to background.
            // The serializer is cleared after a single invocation: callers are
            // expected to supply a fresh lambda with each ScheduleSave() call.
            // If the serializer returns an empty string (e.g. tree has validation
            // errors, or nothing is dirty), this save cycle is silently skipped;
            // the next ScheduleSave() will provide a new serializer to retry.
            std::string data = m_pendingSerializeFn();
            m_pendingSerializeFn = nullptr;

            if (!data.empty())
            {
                std::string path = m_pendingFilePath;
                if (path.empty())
                {
                    path = m_pendingFallbackPrefix.empty()
                               ? "GameData/AI/autosave_"
                               : m_pendingFallbackPrefix;
                    path += std::to_string(++m_fallbackCounter);
                }
                m_pendingFilePath = std::string();
                LaunchAsyncWrite(std::move(data), std::move(path));
            }
        }
        else
        {
            LaunchAsync();
        }
    }
}

void EditorAutosaveManager::Flush()
{
    if (m_future.valid())
    {
        m_future.get(); // wait for completion
    }
}

void EditorAutosaveManager::LaunchAsync()
{
    // Capture a copy of the save function so the lambda is self-contained.
    auto fn = m_saveFn;
    m_future = std::async(std::launch::async, [fn]()
    {
        try
        {
            fn();
        }
        catch (const std::exception& e)
        {
            std::cerr << "[EditorAutosaveManager] Save failed: " << e.what() << std::endl;
        }
        catch (...)
        {
            std::cerr << "[EditorAutosaveManager] Save failed with unknown error." << std::endl;
        }
    });
}

void EditorAutosaveManager::LaunchAsyncWrite(std::string data, std::string path)
{
    m_future = std::async(std::launch::async, [data = std::move(data), path = std::move(path)]()
    {
        try
        {
            std::ofstream out(path, std::ios::out | std::ios::trunc);
            if (!out.is_open())
            {
                std::cerr << "[EditorAutosaveManager] Cannot open: " << path << std::endl;
                return;
            }
            out << data;
        }
        catch (const std::exception& e)
        {
            std::cerr << "[EditorAutosaveManager] Write failed: " << e.what() << std::endl;
        }
        catch (...)
        {
            std::cerr << "[EditorAutosaveManager] Write failed with unknown error." << std::endl;
        }
    });
}

} // namespace Olympe
