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
 * Per-save lambda usage (preferred – serialization on UI thread):
 *   // Construction (once, no legacy saveFn needed):
 *   EditorAutosaveManager autosave;
 *   autosave.Init(nullptr, 1.5f, 60.0f);
 *
 *   // Each frame:
 *   autosave.Tick(ImGui::GetTime());
 *
 *   // When a node moves, supply a serializer + path:
 *   autosave.ScheduleSave(ImGui::GetTime(),
 *       [this]{ return SerializeToString(); },
 *       "path/to/file.json");
 *
 *   // On shutdown:
 *   autosave.Flush();  // waits for any pending async task
 *
 * Legacy usage (saveFn runs entirely on background thread):
 *   autosave.Init([this]{ DoSave(); }, 1.5f, 60.0f);
 *   autosave.ScheduleSave(ImGui::GetTime());
 */

#pragma once

#include <functional>
#include <future>
#include <string>

namespace Olympe
{

class EditorAutosaveManager
{
public:
    EditorAutosaveManager() = default;
    ~EditorAutosaveManager() { Flush(); }

    /**
     * @brief Set the timing parameters and an optional legacy save callback.
     * @param saveFn            Legacy callable executed on a background thread.
     *                          Pass nullptr when using the per-save lambda overload.
     * @param debounceSec       Seconds to wait after the last ScheduleSave() before saving.
     * @param periodicIntervalSec  Maximum seconds between forced flushes (0 = disabled).
     */
    void Init(std::function<void()> saveFn,
              float debounceSec       = 1.5f,
              float periodicIntervalSec = 60.0f);

    /**
     * @brief Notify the manager that a change occurred (legacy overload).
     *        Resets the debounce timer; uses the saveFn supplied to Init().
     * @param nowSec  Current time in seconds (e.g. ImGui::GetTime()).
     */
    void ScheduleSave(double nowSec);

    /**
     * @brief Notify the manager that a change occurred (per-save lambda overload).
     *        Serialization runs on the calling (UI) thread inside Tick() just
     *        before the background write is launched.
     *
     * @param nowSec          Current time in seconds (e.g. ImGui::GetTime()).
     * @param serializeFn     Called on the UI thread to produce the data to write.
     *                        Return an empty string to skip writing.
     * @param filePath        Destination path.  If empty, the fallback path is used.
     * @param fallbackPrefix  Prefix for the fallback filename when filePath is empty.
     *                        A monotonically-increasing counter is appended.
     *                        Defaults to "GameData/AI/autosave_".
     */
    void ScheduleSave(double nowSec,
                      std::function<std::string()> serializeFn,
                      std::string filePath,
                      std::string fallbackPrefix = "GameData/AI/autosave_");

    /**
     * @brief Must be called once per frame to advance timers and launch saves.
     *        Must be called on the UI thread so that the per-save serializer
     *        runs on the correct thread.
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
    void LaunchAsyncWrite(std::string data, std::string path);

    // Legacy path
    std::function<void()> m_saveFn;

    // Per-save lambda path
    std::function<std::string()> m_pendingSerializeFn;
    std::string m_pendingFilePath;
    std::string m_pendingFallbackPrefix;
    int         m_fallbackCounter = 0;

    float  m_debounceSec       = 1.5f;
    float  m_periodicIntervalSec = 60.0f;

    double m_debounceDeadline  = -1.0;   // time at which the debounce save fires
    double m_lastSaveTime      = -1.0;   // last time a save was launched

    bool   m_dirty             = false;  // a ScheduleSave() arrived since last save

    std::future<void> m_future; // running async task (if any)
};

} // namespace Olympe
