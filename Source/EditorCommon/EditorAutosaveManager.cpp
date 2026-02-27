/*
 * Olympe Engine - EditorAutosaveManager implementation
 */

#include "EditorAutosaveManager.h"
#include <iostream>
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
    m_dirty           = true;
    m_debounceDeadline = nowSec + static_cast<double>(m_debounceSec);
}

void EditorAutosaveManager::Tick(double nowSec)
{
    if (!m_saveFn)
        return;

    // If a previous async task is still running, do not start another one.
    if (m_future.valid() &&
        m_future.wait_for(std::chrono::seconds(0)) != std::future_status::ready)
        return;

    bool shouldSave = false;

    // Debounce: fire once the deadline has passed and there is a pending change.
    if (m_dirty && m_debounceDeadline > 0.0 && nowSec >= m_debounceDeadline)
    {
        shouldSave        = true;
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
        m_dirty       = false;
        m_lastSaveTime = nowSec;
        LaunchAsync();
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

} // namespace Olympe
