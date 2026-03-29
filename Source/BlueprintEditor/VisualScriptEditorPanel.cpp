/**
 * @file VisualScriptEditorPanel.cpp
 * @brief ImNodes graph editor implementation for ATS VS graphs (Phase 5).
 * @author Olympe Engine
 * @date 2026-03-09
 *
 * @details C++14 compliant — no std::optional, structured bindings, std::filesystem.
 * 
 * Implementation split into specialized files:
 * - VisualScriptEditorPanel_Core.cpp        (lifecycle, render dispatch)
 * - VisualScriptEditorPanel_Canvas.cpp      (ImNodes canvas, sync)
 * - VisualScriptEditorPanel_Connections.cpp (links, pins)
 * - VisualScriptEditorPanel_Rendering.cpp   (toolbar, palette, menus)
 * - VisualScriptEditorPanel_NodeProperties.cpp (properties panel)
 * - VisualScriptEditorPanel_Blackboard.cpp  (variables)
 * - VisualScriptEditorPanel_Verification.cpp (validation)
 * - VisualScriptEditorPanel_FileOps.cpp     (save/load)
 * - VisualScriptEditorPanel_Interaction.cpp (undo/redo, node ops)
 * - VisualScriptEditorPanel_Presets.cpp     (Phase 24 presets)
 */

#include "VisualScriptEditorPanel.h"
#include "DebugController.h"
#include "AtomicTaskUIRegistry.h"
#include "ConditionRegistry.h"
#include "OperatorRegistry.h"
#include "BBVariableRegistry.h"
#include "MathOpOperand.h"
#include "../system/system_utils.h"
#include "../system/system_consts.h"
#include "../NodeGraphCore/GlobalTemplateBlackboard.h"

#include "../third_party/imgui/imgui.h"
#include "../third_party/imnodes/imnodes.h"
#include "../json_helper.h"
#include "../TaskSystem/TaskGraphLoader.h"

#include <fstream>
#include <iostream>
#include <algorithm>
#include <cmath>
#include <cstring>
#include <sstream>
#include <iomanip>
#include <cstdlib>
#include <unordered_set>

namespace Olympe {
} // namespace Olympe
