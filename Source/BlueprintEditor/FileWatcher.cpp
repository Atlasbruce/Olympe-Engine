/**
 * @file FileWatcher.cpp
 * @brief Poll-based file change detection implementation (Phase 7).
 * @author Olympe Engine
 * @date 2026-03-10
 */

#include "FileWatcher.h"

#ifdef _WIN32
#  include <sys/stat.h>
// Use _stat on Windows
#else
#  include <sys/stat.h>
#endif

namespace Olympe {

// ============================================================================
// Construction
// ============================================================================

FileWatcher::FileWatcher()
    : m_LastModified(0)
    , m_IsWatching(false)
{
}

FileWatcher::FileWatcher(const std::string& path)
    : m_LastModified(0)
    , m_IsWatching(false)
{
    Watch(path);
}

// ============================================================================
// Control
// ============================================================================

void FileWatcher::Watch(const std::string& path)
{
    m_Path         = path;
    m_IsWatching   = !path.empty();
    m_LastModified = m_IsWatching ? GetFileModTime(path) : 0;
}

bool FileWatcher::HasChanged()
{
    if (!m_IsWatching)
        return false;

    time_t current = GetFileModTime(m_Path);
    return current > m_LastModified;
}

void FileWatcher::Reset()
{
    if (m_IsWatching)
        m_LastModified = GetFileModTime(m_Path);
}

// ============================================================================
// Accessors
// ============================================================================

const std::string& FileWatcher::GetPath() const
{
    return m_Path;
}

bool FileWatcher::IsWatching() const
{
    return m_IsWatching;
}

// ============================================================================
// GetFileModTime
// ============================================================================

time_t FileWatcher::GetFileModTime(const std::string& path)
{
    if (path.empty())
        return 0;

#ifdef _WIN32
    struct _stat st;
    if (_stat(path.c_str(), &st) != 0)
        return 0;
    return static_cast<time_t>(st.st_mtime);
#else
    struct stat st;
    if (stat(path.c_str(), &st) != 0)
        return 0;
    return st.st_mtime;
#endif
}

} // namespace Olympe
