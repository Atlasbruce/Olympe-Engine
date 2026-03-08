// [DEPRECATED - Phase 1 ATS VS - 2026-03-08]
// Archivé depuis Source/BlueprintEditor/Graph.cpp
// Ce fichier est conservé uniquement pour référence historique.
// Il ne doit plus être inclus ni compilé.
// Remplacé par : TaskGraphTemplate (Source/TaskSystem/TaskGraphTemplate.h)
// Voir : Documentation/ATS_VS_Phase1_Migration_Notes.md

#include "Graph_legacy.h"
#include "../json_helper.h"

using json = nlohmann::json;

nlohmann::json Graph::ToJson() const
{
    json j;
    j["nodes"] = json::array();
    for (size_t i = 0; i < nodes.size(); ++i)
    {
        const Node& n = nodes[i];
        json nj;
        nj["id"] = n.id;
        nj["type"] = n.type;
        nj["x"] = n.x;
        nj["y"] = n.y;
        j["nodes"].push_back(nj);
    }
    return j;
}

Graph Graph::FromJson(const nlohmann::json& j)
{
    Graph g;
    
    if (JsonHelper::IsArray(j, "nodes"))
    {
        JsonHelper::ForEachInArray(j, "nodes", [&g](const json& nj, size_t idx)
        {
            Node n;
            n.id = JsonHelper::GetInt(nj, "id", 0);
            n.type = JsonHelper::GetString(nj, "type", "");
            n.x = JsonHelper::GetFloat(nj, "x", 0.0f);
            n.y = JsonHelper::GetFloat(nj, "y", 0.0f);
            g.nodes.push_back(n);
        });
    }
    
    return g;
}
