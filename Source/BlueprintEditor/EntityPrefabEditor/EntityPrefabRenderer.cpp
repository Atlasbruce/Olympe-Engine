#include "EntityPrefabRenderer.h"
#include "EntityPrefabGraphDocument.h"
#include "PrefabLoader.h"
#include "../../system/system_utils.h"
#include <memory>

namespace Olympe {

EntityPrefabRenderer::EntityPrefabRenderer(PrefabCanvas& canvas)
    : m_canvas(canvas), m_filePath(""), m_isDirty(false)
{
}

EntityPrefabRenderer::~EntityPrefabRenderer()
{
}

void EntityPrefabRenderer::Render()
{
    m_canvas.Render();
}

bool EntityPrefabRenderer::Load(const std::string& path)
{
    m_filePath = path;

    try
    {
        // Load JSON file
        nlohmann::json jsonData = PrefabLoader::LoadJsonFromFile(path);
        SYSTEM_LOG << "[EntityPrefabRenderer] Loaded JSON from: " << path << "\n";

        // Verify it's an EntityPrefab
        if (jsonData.contains("blueprintType"))
        {
            std::string blueprintType = jsonData["blueprintType"].get<std::string>();
            SYSTEM_LOG << "[EntityPrefabRenderer] blueprintType: " << blueprintType << "\n";
            if (blueprintType != "EntityPrefab")
            {
                SYSTEM_LOG << "[EntityPrefabRenderer] ERROR: Not an EntityPrefab type\n";
                return false;
            }
        }

        // Get or create document
        EntityPrefabGraphDocument* document = m_canvas.GetDocument();
        if (document == nullptr)
        {
            SYSTEM_LOG << "[EntityPrefabRenderer] ERROR: GetDocument() returned nullptr\n";
            return false;
        }

        SYSTEM_LOG << "[EntityPrefabRenderer] Document obtained, loading from file...\n";

        // Load from JSON into document
        if (!document->LoadFromFile(path))
        {
            SYSTEM_LOG << "[EntityPrefabRenderer] ERROR: document->LoadFromFile() failed\n";
            return false;
        }

        SYSTEM_LOG << "[EntityPrefabRenderer] Successfully loaded prefab\n";
        m_isDirty = false;
        return true;
    }
    catch (const std::exception& e)
    {
        SYSTEM_LOG << "[EntityPrefabRenderer] EXCEPTION: " << e.what() << "\n";
        return false;
    }
}

bool EntityPrefabRenderer::Save(const std::string& path)
{
    std::string savePath = path.empty() ? m_filePath : path;

    if (savePath.empty())
    {
        return false;
    }

    try
    {
        EntityPrefabGraphDocument* document = m_canvas.GetDocument();
        if (document == nullptr)
        {
            return false;
        }

        // Save document to file
        if (!document->SaveToFile(savePath))
        {
            return false;
        }

        m_isDirty = false;
        m_filePath = savePath;
        return true;
    }
    catch (const std::exception&)
    {
        return false;
    }
}

bool EntityPrefabRenderer::IsDirty() const
{
    // Check both the document's dirty flag and track modifications
    EntityPrefabGraphDocument* document = m_canvas.GetDocument();
    if (document != nullptr)
    {
        return document->IsDirty();
    }
    return m_isDirty;
}

std::string EntityPrefabRenderer::GetGraphType() const
{
    return "EntityPrefab";
}

std::string EntityPrefabRenderer::GetCurrentPath() const
{
    return m_filePath;
}

} // namespace Olympe
