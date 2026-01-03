#pragma once
#include <string>
#include <vector>
// use bundled nlohmann json header from workspace
#include "../../Source/third_party/nlohmann/json.hpp"

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
