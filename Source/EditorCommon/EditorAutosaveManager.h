/*
 * Olympe Engine - EditorAutosaveManager
 *
 * Lightweight helper that persists node positions asynchronously.
 * Two save triggers are combined:
 *   1. Debounced immediate save: when ScheduleSave() is called, a debounce
 *      timer is (re)started.  Once the timer expires the save runs on a
 *      background thread so the UI is never blocked.
 *   2. Periodic flush: every m_periodicIntervalSec seconds a background save
 *      is forced regardless of the debounce state.
 *
 * Usage:
 *   // Construction (once):
 *   EditorAutosaveManager autosave;
 *   autosave.Init([this]{ DoSave(); }, 1.5f, 60.0f);
 *
 *   // Each frame:
 *   autosave.Tick(ImGui::GetTime());
 *
 *   // When a node moves:
 *   autosave.ScheduleSave(ImGui::GetTime());
 *
 *   // On shutdown:
 *   autosave.Flush();  // waits for any pending async task
 */

#pragma once

#include <functional>
#include <future>

namespace Olympe
{

class EditorAutosaveManager
{
public:
    EditorAutosaveManager() = default;
    ~EditorAutosaveManager() { Flush(); }

    /**
     * @brief Set the save callback and timing parameters.
     * @param saveFn            Callable executed on a background thread to persist data.
     * @param debounceSec       Seconds to wait after the last ScheduleSave() before saving.
     * @param periodicIntervalSec  Maximum seconds between forced flushes (0 = disabled).
     */
    void Init(std::function<void()> saveFn,
              float debounceSec       = 1.5f,
              float periodicIntervalSec = 60.0f);

    /**
     * @brief Notify the manager that a change occurred (e.g. node moved).
     *        Resets the debounce timer.
     * @param nowSec  Current time in seconds (e.g. ImGui::GetTime()).
     */
    void ScheduleSave(double nowSec);

    /**
     * @brief Must be called once per frame to advance timers and launch saves.
     * @param nowSec  Current time in seconds (e.g. ImGui::GetTime()).
     */
    void Tick(double nowSec);

    /**
     * @brief Block until any running async save finishes.
     *        Call from Shutdown() or destructor.
     */
    void Flush();

private:
    void LaunchAsync();

    std::function<void()> m_saveFn;

    float  m_debounceSec       = 1.5f;
    float  m_periodicIntervalSec = 60.0f;

    double m_debounceDeadline  = -1.0;   // time at which the debounce save fires
    double m_lastSaveTime      = -1.0;   // last time a save was launched

    bool   m_dirty             = false;  // a ScheduleSave() arrived since last save

    std::future<void> m_future; // running async task (if any)
};

} // namespace Olympe
