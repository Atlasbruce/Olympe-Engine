/**
 * @file GraphCommand.h
 * @brief Base command class for undo/redo in graph operations
 * @author Olympe Engine
 * @details C++14 compliant - no C++17/20 features
 */

#pragma once

#include <string>
#include <memory>

namespace Olympe
{
    /**
     * @class GraphCommand
     * @brief Abstract base class for all graph modification commands
     * 
     * Implements the Command Pattern for undo/redo functionality.
     * Each command represents a single, undoable action on the graph.
     */
    class GraphCommand
    {
    public:
        virtual ~GraphCommand() = default;

        /**
         * @brief Execute the command (forward operation)
         * @return true if execution succeeded
         */
        virtual bool Execute() = 0;

        /**
         * @brief Undo the command (reverse operation)
         * @return true if undo succeeded
         */
        virtual bool Undo() = 0;

        /**
         * @brief Get a human-readable description of the command
         * @return Command description for UI display (e.g., "Create Action Node")
         */
        virtual std::string GetDescription() const = 0;

        /**
         * @brief Check if this command can be merged with another
         * Used for combining consecutive similar commands (e.g., multiple position changes)
         * @param other The other command to potentially merge with
         * @return true if commands can be merged
         */
        virtual bool CanMergeWith(const GraphCommand& other) const
        {
            return false;  // Default: no merging
        }

        /**
         * @brief Merge this command with another
         * Called only if CanMergeWith returned true
         * @param other The command to merge with
         * @return true if merge succeeded
         */
        virtual bool MergeWith(const GraphCommand& other)
        {
            return false;  // Default: no merging
        }
    };

    using GraphCommandPtr = std::unique_ptr<GraphCommand>;

}  // namespace Olympe
