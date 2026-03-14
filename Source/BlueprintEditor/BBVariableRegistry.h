/**
 * @file BBVariableRegistry.h
 * @brief Wrapper around the graph blackboard entries for dropdown editors.
 * @author Olympe Engine
 * @date 2026-03-14
 *
 * @details
 * BBVariableRegistry wraps the Blackboard vector from a TaskGraphTemplate
 * and exposes filtered, formatted views suitable for populating dropdown
 * menus in the properties panel (GetBBValue, SetBBValue, Switch, etc.).
 *
 * Call LoadFromTemplate() each time the active template changes.
 *
 * C++14 compliant — no std::optional, structured bindings, std::filesystem.
 */

#pragma once

#include <string>
#include <vector>

#include "../TaskSystem/TaskGraphTypes.h"
#include "../TaskSystem/TaskGraphTemplate.h"

namespace Olympe {

/**
 * @struct VarSpec
 * @brief Metadata for a single blackboard variable entry.
 */
struct VarSpec {
    std::string  name;         ///< Variable key (e.g. "health", "target")
    VariableType type;         ///< Declared type
    bool         isGlobal;     ///< true = global scope, false = local scope
    std::string  displayLabel; ///< Formatted label, e.g. "health (Float, local)"
};

/**
 * @class BBVariableRegistry
 * @brief Non-singleton registry populated from the active TaskGraphTemplate.
 *
 * @details
 * Intentionally non-singleton because the variable set depends on which
 * graph is currently open in the editor.  Create one instance per editor
 * panel and call LoadFromTemplate() whenever the template changes.
 */
class BBVariableRegistry {
public:

    BBVariableRegistry() = default;

    /**
     * @brief Rebuilds the registry from the blackboard entries of a template.
     * @param tmpl Source template whose Blackboard vector is read.
     */
    void LoadFromTemplate(const TaskGraphTemplate& tmpl);

    /**
     * @brief Returns all variables (local and global), sorted by name.
     */
    const std::vector<VarSpec>& GetAllVariables() const;

    /**
     * @brief Returns variables whose type matches @p type, sorted by name.
     * @param type VariableType filter.
     */
    std::vector<VarSpec> GetVariablesByType(VariableType type) const;

    /**
     * @brief Returns only local-scope variables.
     */
    std::vector<VarSpec> GetLocalVariables() const;

    /**
     * @brief Returns only global-scope variables.
     */
    std::vector<VarSpec> GetGlobalVariables() const;

    /**
     * @brief Formats a display label for a variable.
     *
     * Format: "<name> (<TypeName>, <scope>)"
     * Example: "health (Float, local)"
     *
     * @param name     Variable key.
     * @param type     Variable type.
     * @param isGlobal Scope flag.
     * @return Formatted display label.
     */
    static std::string FormatDisplayLabel(const std::string& name,
                                          VariableType type,
                                          bool isGlobal);

    /**
     * @brief Returns true if a variable with the given name is registered.
     * @param name Variable key to look up.
     */
    bool HasVariable(const std::string& name) const;

    /**
     * @brief Returns the number of registered variables.
     */
    size_t GetCount() const;

private:

    std::vector<VarSpec> m_vars;
};

} // namespace Olympe
