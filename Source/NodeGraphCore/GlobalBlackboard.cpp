/**
 * @file GlobalBlackboard.cpp
 * @brief GlobalBlackboard implementation (singleton body).
 * @author Olympe Engine
 * @date 2026-03-08
 *
 * @details
 * All non-inline methods of GlobalBlackboard are implemented inline in the
 * header (Meyers singleton pattern). This translation unit exists to provide
 * a compilation target and to allow future non-inline methods (e.g. JSON
 * persistence helpers) to be added without modifying existing include graphs.
 *
 * C++14 compliant - no C++17/20 features.
 */

#include "GlobalBlackboard.h"

// All inline implementations are in GlobalBlackboard.h.
// This translation unit intentionally contains no additional definitions.
