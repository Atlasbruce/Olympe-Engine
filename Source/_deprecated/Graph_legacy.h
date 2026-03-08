// [DEPRECATED - Phase 1 ATS VS - 2026-03-08]
// Archivé depuis Source/BlueprintEditor/Graph.h
// Ce fichier est conservé uniquement pour référence historique.
// Il ne doit plus être inclus ni compilé.
// Remplacé par : TaskGraphTemplate (Source/TaskSystem/TaskGraphTemplate.h)
// Voir : Documentation/ATS_VS_Phase1_Migration_Notes.md

#pragma once
#include <string>
#include <vector>
// use bundled nlohmann json header from workspace
#include "../third_party/nlohmann/json.hpp"

struct Node
{
    int id;
    std::string type;
    float x, y;
};

struct Graph
{
    std::vector<Node> nodes;

    nlohmann::json ToJson() const;
    static Graph FromJson(const nlohmann::json& j);
};
