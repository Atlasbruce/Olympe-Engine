#include "ParameterSchemaRegistry.h"

namespace Olympe
{
    ParameterSchemaRegistry* ParameterSchemaRegistry::s_instance = nullptr;

    ParameterSchemaRegistry::ParameterSchemaRegistry() { }
    ParameterSchemaRegistry::~ParameterSchemaRegistry() { }

    ParameterSchemaRegistry& ParameterSchemaRegistry::Get()
    {
        if (s_instance == nullptr) { s_instance = new ParameterSchemaRegistry(); }
        return *s_instance;
    }

    void ParameterSchemaRegistry::RegisterSchema(const ComponentSchema& schema)
    { m_schemas.push_back(schema); UpdateIndices(); }

    void ParameterSchemaRegistry::UnregisterSchema(const std::string& componentName)
    { 
        for (size_t i = 0; i < m_schemas.size(); ++i)
        { if (m_schemas[i].componentName == componentName) { m_schemas.erase(m_schemas.begin() + i); break; } }
        UpdateIndices();
    }

    bool ParameterSchemaRegistry::HasSchema(const std::string& componentName) const
    { return m_schemaIndex.find(componentName) != m_schemaIndex.end(); }

    const ComponentSchema* ParameterSchemaRegistry::GetSchema(const std::string& componentName) const
    { 
        auto it = m_schemaIndex.find(componentName);
        if (it != m_schemaIndex.end()) { return &m_schemas[it->second]; }
        return nullptr;
    }

    const std::vector<ComponentSchema>& ParameterSchemaRegistry::GetAllSchemas() const
    { return m_schemas; }

    void ParameterSchemaRegistry::LoadSchemasFromFile(const std::string& filePath) { (void)filePath; }
    void ParameterSchemaRegistry::LoadSchemasFromDirectory(const std::string& directoryPath) { (void)directoryPath; }

    void ParameterSchemaRegistry::ClearAllSchemas()
    { m_schemas.clear(); m_schemaIndex.clear(); m_categoryIndex.clear(); }

    std::vector<std::string> ParameterSchemaRegistry::GetSchemaNames() const
    { std::vector<std::string> names; for (size_t i = 0; i < m_schemas.size(); ++i) { names.push_back(m_schemas[i].componentName); } return names; }

    std::vector<ComponentSchema> ParameterSchemaRegistry::GetSchemasByCategory(const std::string& category) const
    { std::vector<ComponentSchema> result; for (size_t i = 0; i < m_schemas.size(); ++i) { if (m_schemas[i].category == category) { result.push_back(m_schemas[i]); } } return result; }

    std::vector<ComponentSchema> ParameterSchemaRegistry::SearchSchemas(const std::string& query) const
    { std::vector<ComponentSchema> result; for (size_t i = 0; i < m_schemas.size(); ++i) { if (m_schemas[i].componentName.find(query) != std::string::npos) { result.push_back(m_schemas[i]); } } return result; }

    std::vector<std::string> ParameterSchemaRegistry::GetCategories() const
    { std::vector<std::string> categories; for (auto it = m_categoryIndex.begin(); it != m_categoryIndex.end(); ++it) { categories.push_back(it->first); } return categories; }

    std::vector<ComponentSchema> ParameterSchemaRegistry::GetSchemasInCategory(const std::string& category) const
    { return GetSchemasByCategory(category); }

    const ParameterDefinition* ParameterSchemaRegistry::GetParameterDefinition(const std::string& componentName, const std::string& parameterName) const
    { 
        const ComponentSchema* schema = GetSchema(componentName);
        if (schema != nullptr) { for (size_t i = 0; i < schema->parameters.size(); ++i) { if (schema->parameters[i].name == parameterName) { return &schema->parameters[i]; } } }
        return nullptr;
    }

    std::vector<ParameterDefinition> ParameterSchemaRegistry::GetParametersForComponent(const std::string& componentName) const
    { 
        const ComponentSchema* schema = GetSchema(componentName);
        if (schema != nullptr) { return schema->parameters; }
        return std::vector<ParameterDefinition>();
    }

    bool ParameterSchemaRegistry::ValidateComponent(const ComponentData& component) const
    { (void)component; return true; }

    bool ParameterSchemaRegistry::ValidateParameter(const std::string& componentName, const std::string& parameterName, const std::string& value) const
    { (void)componentName; (void)parameterName; (void)value; return true; }

    void ParameterSchemaRegistry::RebuildCategoryIndex()
    { m_categoryIndex.clear(); for (size_t i = 0; i < m_schemas.size(); ++i) { m_categoryIndex[m_schemas[i].category].push_back(i); } }

    void ParameterSchemaRegistry::ClearCache() { }

    size_t ParameterSchemaRegistry::GetSchemaCount() const { return m_schemas.size(); }
    size_t ParameterSchemaRegistry::GetCategoryCount() const { return m_categoryIndex.size(); }
    size_t ParameterSchemaRegistry::GetTotalParameterCount() const
    { size_t count = 0; for (size_t i = 0; i < m_schemas.size(); ++i) { count += m_schemas[i].parameters.size(); } return count; }

    bool ParameterSchemaRegistry::IsSchemaDeprecated(const std::string& componentName) const
    { const ComponentSchema* schema = GetSchema(componentName); if (schema != nullptr) { return schema->isDeprecated; } return false; }

    std::vector<ComponentSchema> ParameterSchemaRegistry::GetDeprecatedSchemas() const
    { std::vector<ComponentSchema> result; for (size_t i = 0; i < m_schemas.size(); ++i) { if (m_schemas[i].isDeprecated) { result.push_back(m_schemas[i]); } } return result; }

    void ParameterSchemaRegistry::UpdateIndices()
    { m_schemaIndex.clear(); m_categoryIndex.clear(); for (size_t i = 0; i < m_schemas.size(); ++i) { m_schemaIndex[m_schemas[i].componentName] = i; m_categoryIndex[m_schemas[i].category].push_back(i); } }

} // namespace Olympe
