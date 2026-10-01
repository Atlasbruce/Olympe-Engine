#pragma once

#include "GraphCommand.h"
#include <functional>
#include <utility>

namespace Olympe
{
    /** Generic command adapter for graph-specific forward and reverse operations. */
    class CallbackGraphCommand final : public GraphCommand
    {
    public:
        using Operation = std::function<bool()>;

        CallbackGraphCommand(std::string description, Operation execute, Operation undo)
            : m_description(std::move(description))
            , m_execute(std::move(execute))
            , m_undo(std::move(undo))
        {
        }

        bool Execute() override { return m_execute && m_execute(); }
        bool Undo() override { return m_undo && m_undo(); }
        std::string GetDescription() const override { return m_description; }

    private:
        std::string m_description;
        Operation m_execute;
        Operation m_undo;
    };
}
