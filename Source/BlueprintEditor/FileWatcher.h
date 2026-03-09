/**
 * @file FileWatcher.h
 * @brief Poll-based file change detection (Phase 7).
 * @author Olympe Engine
 * @date 2026-03-10
 *
 * @details
 * FileWatcher uses stat() / _stat() to sample a file's modification timestamp.
 * HasChanged() returns true the first time it detects a newer timestamp than
 * the one recorded at Watch() / Reset() time.
 *
 * No filesystem-watcher OS dependency — pure polling via <sys/stat.h>.
 *
 * C++14 compliant — no std::optional, structured bindings, std::filesystem.
 */

#pragma once

#include <string>
#include <ctime>

namespace Olympe {

// ============================================================================
// FileWatcher
// ============================================================================

/**
 * @class FileWatcher
 * @brief Polls a single file for modification-time changes.
 *
 * Typical usage:
 * @code
 *   FileWatcher watcher("data/graph.json");
 *   // ... later, every frame or on timer ...
 *   if (watcher.HasChanged())
 *   {
 *       ReloadGraph();
 *       watcher.Reset();
 *   }
 * @endcode
 */
class FileWatcher {
public:

    /**
     * @brief Constructs a FileWatcher that is not yet watching any file.
     */
    FileWatcher();

    /**
     * @brief Constructs a FileWatcher and immediately begins watching @p path.
     */
    explicit FileWatcher(const std::string& path);

    // -----------------------------------------------------------------------
    // Control
    // -----------------------------------------------------------------------

    /**
     * @brief Starts watching @p path, recording the current modification time.
     */
    void Watch(const std::string& path);

    /**
     * @brief Returns true if the file's modification time has changed since
     *        the last Watch() or Reset() call.
     */
    bool HasChanged();

    /**
     * @brief Resets the baseline modification timestamp to the current file time.
     */
    void Reset();

    // -----------------------------------------------------------------------
    // Accessors
    // -----------------------------------------------------------------------

    /**
     * @brief Returns the path currently being watched.
     */
    const std::string& GetPath()       const;

    /**
     * @brief Returns true if a path has been set via Watch() or the path constructor.
     */
    bool               IsWatching()    const;

private:

    /**
     * @brief Platform-portable wrapper around stat() / _stat().
     * @return Modification time, or 0 on error.
     */
    static time_t GetFileModTime(const std::string& path);

    std::string m_Path;
    time_t      m_LastModified;
    bool        m_IsWatching;
};

} // namespace Olympe
