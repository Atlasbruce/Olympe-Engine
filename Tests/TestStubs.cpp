/**
 * @file TestStubs.cpp
 * @brief Minimal stubs for symbols required by the TaskSystem sources but not
 *        exercised during unit tests (e.g. UI log window sink).
 */

#include <string>

namespace SystemLogSink {
    // No-op implementation: unit tests do not need a UI log window.
    void AppendToLogWindow(const std::string& /*text*/) {}
}
