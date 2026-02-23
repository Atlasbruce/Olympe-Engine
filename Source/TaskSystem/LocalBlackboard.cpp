/**
 * @file LocalBlackboard.cpp
 * @brief Implementation of LocalBlackboard for the Atomic Task System
 * @author Olympe Engine
 * @date 2026-02-20
 */

#include "LocalBlackboard.h"
#include "TaskGraphTemplate.h"

#include <stdexcept>
#include <string>
#include <cstring>

#include "../system/system_utils.h"

namespace Olympe {

// ============================================================================
// Constructor
// ============================================================================

LocalBlackboard::LocalBlackboard()
{
}

// ============================================================================
// Lifecycle
// ============================================================================

void LocalBlackboard::Initialize(const TaskGraphTemplate& tmpl)
{
    m_variables.clear();
    m_defaults.clear();
    m_types.clear();

    for (size_t i = 0; i < tmpl.LocalVariables.size(); ++i)
    {
        const VariableDefinition& def = tmpl.LocalVariables[i];
        m_variables[def.Name]  = def.DefaultValue;
        m_defaults[def.Name]   = def.DefaultValue;
        m_types[def.Name]      = def.Type;
    }

    SYSTEM_LOG << "[LocalBlackboard] Initialized with " << tmpl.LocalVariables.size()
               << " variables from template '" << tmpl.Name << "'" << std::endl;
}

void LocalBlackboard::Reset()
{
    for (auto it = m_defaults.begin(); it != m_defaults.end(); ++it)
    {
        m_variables[it->first] = it->second;
    }

    SYSTEM_LOG << "[LocalBlackboard] Reset to defaults" << std::endl;
}

// ============================================================================
// Variable access
// ============================================================================

TaskValue LocalBlackboard::GetValue(const std::string& varName) const
{
    auto it = m_variables.find(varName);
    if (it == m_variables.end())
    {
        throw std::runtime_error("[LocalBlackboard] Unknown variable: " + varName);
    }
    return it->second;
}

void LocalBlackboard::SetValue(const std::string& varName, const TaskValue& value)
{
    auto typeIt = m_types.find(varName);
    if (typeIt == m_types.end())
    {
        throw std::runtime_error("[LocalBlackboard] Unknown variable: " + varName);
    }

    if (value.GetType() != typeIt->second)
    {
        throw std::runtime_error("[LocalBlackboard] Type mismatch for variable: " + varName);
    }

    m_variables[varName] = value;
}

// ============================================================================
// Queries
// ============================================================================

bool LocalBlackboard::HasVariable(const std::string& varName) const
{
    return m_variables.find(varName) != m_variables.end();
}

std::vector<std::string> LocalBlackboard::GetVariableNames() const
{
    std::vector<std::string> names;
    names.reserve(m_variables.size());

    for (auto it = m_variables.begin(); it != m_variables.end(); ++it)
    {
        names.push_back(it->first);
    }

    return names;
}

// ============================================================================
// Persistence helpers
// ============================================================================

namespace {

template<typename T>
static void WriteBytes(std::vector<uint8_t>& buf, const T& value)
{
    const uint8_t* p = reinterpret_cast<const uint8_t*>(&value);
    buf.insert(buf.end(), p, p + sizeof(T));
}

static bool ReadBytes(const std::vector<uint8_t>& buf, size_t& pos, void* dst, size_t n)
{
    if (pos + n > buf.size()) return false;
    std::memcpy(dst, buf.data() + pos, n);
    pos += n;
    return true;
}

} // anonymous namespace

// ============================================================================
// Serialize
// ============================================================================

void LocalBlackboard::Serialize(std::vector<uint8_t>& outBytes) const
{
    outBytes.clear();

    uint32_t count = static_cast<uint32_t>(m_variables.size());
    WriteBytes(outBytes, count);

    for (auto it = m_variables.begin(); it != m_variables.end(); ++it)
    {
        const std::string& name = it->first;
        const TaskValue&   val  = it->second;

        uint32_t nameLen = static_cast<uint32_t>(name.size());
        WriteBytes(outBytes, nameLen);
        outBytes.insert(outBytes.end(), name.begin(), name.end());

        uint8_t type = static_cast<uint8_t>(val.GetType());
        WriteBytes(outBytes, type);

        switch (val.GetType())
        {
            case VariableType::Bool:
            {
                uint8_t bv = val.AsBool() ? 1u : 0u;
                WriteBytes(outBytes, bv);
                break;
            }
            case VariableType::Int:
            {
                int32_t iv = static_cast<int32_t>(val.AsInt());
                WriteBytes(outBytes, iv);
                break;
            }
            case VariableType::Float:
            {
                float fv = val.AsFloat();
                WriteBytes(outBytes, fv);
                break;
            }
            case VariableType::Vector:
            {
                ::Vector vv = val.AsVector();
                WriteBytes(outBytes, vv.x);
                WriteBytes(outBytes, vv.y);
                WriteBytes(outBytes, vv.z);
                break;
            }
            case VariableType::EntityID:
            {
                uint64_t eid = static_cast<uint64_t>(val.AsEntityID());
                WriteBytes(outBytes, eid);
                break;
            }
            case VariableType::String:
            {
                const std::string& sv   = val.AsString();
                uint32_t           slen = static_cast<uint32_t>(sv.size());
                WriteBytes(outBytes, slen);
                outBytes.insert(outBytes.end(), sv.begin(), sv.end());
                break;
            }
            default:
                break;
        }
    }

    SYSTEM_LOG << "[LocalBlackboard] Serialized " << count << " variables ("
               << outBytes.size() << " bytes)\n";
}

// ============================================================================
// Deserialize
// ============================================================================

void LocalBlackboard::Deserialize(const std::vector<uint8_t>& inBytes)
{
    size_t pos = 0;

    uint32_t count = 0;
    if (!ReadBytes(inBytes, pos, &count, sizeof(count)))
    {
        SYSTEM_LOG << "[LocalBlackboard] Deserialize: buffer too short for count\n";
        return;
    }

    for (uint32_t i = 0; i < count; ++i)
    {
        // --- name ---
        uint32_t nameLen = 0;
        if (!ReadBytes(inBytes, pos, &nameLen, sizeof(nameLen)))
        {
            SYSTEM_LOG << "[LocalBlackboard] Deserialize: buffer truncated at entry " << i << "\n";
            return;
        }
        if (pos + nameLen > inBytes.size())
        {
            SYSTEM_LOG << "[LocalBlackboard] Deserialize: name bytes truncated at entry " << i << "\n";
            return;
        }
        std::string name(reinterpret_cast<const char*>(inBytes.data() + pos), nameLen);
        pos += nameLen;

        // --- type tag ---
        uint8_t typeTag = 0;
        if (!ReadBytes(inBytes, pos, &typeTag, sizeof(typeTag)))
        {
            SYSTEM_LOG << "[LocalBlackboard] Deserialize: buffer truncated at type for '" << name << "'\n";
            return;
        }
        VariableType storedType = static_cast<VariableType>(typeTag);

        // --- value ---
        bool      parseOk = true;
        TaskValue val;

        switch (storedType)
        {
            case VariableType::Bool:
            {
                uint8_t bv = 0;
                parseOk = ReadBytes(inBytes, pos, &bv, sizeof(bv));
                if (parseOk) val = TaskValue(bv != 0);
                break;
            }
            case VariableType::Int:
            {
                int32_t iv = 0;
                parseOk = ReadBytes(inBytes, pos, &iv, sizeof(iv));
                if (parseOk) val = TaskValue(static_cast<int>(iv));
                break;
            }
            case VariableType::Float:
            {
                float fv = 0.0f;
                parseOk = ReadBytes(inBytes, pos, &fv, sizeof(fv));
                if (parseOk) val = TaskValue(fv);
                break;
            }
            case VariableType::Vector:
            {
                float x = 0.0f, y = 0.0f, z = 0.0f;
                parseOk = ReadBytes(inBytes, pos, &x, sizeof(x))
                       && ReadBytes(inBytes, pos, &y, sizeof(y))
                       && ReadBytes(inBytes, pos, &z, sizeof(z));
                if (parseOk) val = TaskValue(::Vector(x, y, z));
                break;
            }
            case VariableType::EntityID:
            {
                uint64_t eid = 0;
                parseOk = ReadBytes(inBytes, pos, &eid, sizeof(eid));
                if (parseOk) val = TaskValue(static_cast<EntityID>(eid));
                break;
            }
            case VariableType::String:
            {
                uint32_t slen = 0;
                parseOk = ReadBytes(inBytes, pos, &slen, sizeof(slen));
                if (parseOk)
                {
                    if (pos + slen > inBytes.size()) { parseOk = false; break; }
                    std::string sv(reinterpret_cast<const char*>(inBytes.data() + pos), slen);
                    pos += slen;
                    val = TaskValue(sv);
                }
                break;
            }
            default:
                SYSTEM_LOG << "[LocalBlackboard] Deserialize: unknown type tag "
                           << static_cast<int>(typeTag) << " for '" << name << "' - aborting\n";
                return;
        }

        if (!parseOk)
        {
            SYSTEM_LOG << "[LocalBlackboard] Deserialize: buffer truncated while reading value for '"
                       << name << "'\n";
            return;
        }

        // --- schema check ---
        auto typeIt = m_types.find(name);
        if (typeIt == m_types.end())
        {
            SYSTEM_LOG << "[LocalBlackboard] Deserialize: unknown variable '" << name << "' - skipping\n";
            continue;
        }
        if (typeIt->second != storedType)
        {
            SYSTEM_LOG << "[LocalBlackboard] Deserialize: type mismatch for '" << name << "' - skipping\n";
            continue;
        }

        m_variables[name] = val;
    }

    SYSTEM_LOG << "[LocalBlackboard] Deserialized " << count << " entries\n";
}

} // namespace Olympe
