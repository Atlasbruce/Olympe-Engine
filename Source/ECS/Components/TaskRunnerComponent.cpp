/**
 * @file TaskRunnerComponent.cpp
 * @brief Implementation of TaskRunnerComponent constructor and destructor.
 * @author Olympe Engine
 * @date 2026-02-22
 *
 * @details
 * This translation unit provides the out-of-line constructor and destructor
 * for TaskRunnerComponent.  Defining them here (rather than inline in the
 * header) allows the header to forward-declare IAtomicTask without including
 * the full IAtomicTask.h, since std::unique_ptr<IAtomicTask> requires the
 * complete type only at the point of deletion.
 *
 * C++14 compliant - no C++17/20 features.
 */

#include "TaskRunnerComponent.h"
#include "../../TaskSystem/IAtomicTask.h"

namespace Olympe {

TaskRunnerComponent::TaskRunnerComponent() = default;

TaskRunnerComponent::~TaskRunnerComponent() = default;

} // namespace Olympe
