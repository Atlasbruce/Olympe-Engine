#include <fstream>
#include "Graph.h"
#include "../../Source/third_party/nlohmann/json.hpp"

void SaveGraphToFile(const Graph& g, const std::string& path)
{
    auto j = g.ToJson();
    std::ofstream ofs(path);
    if (ofs) ofs << j.dump(4);
}

Graph LoadGraphFromFile(const std::string& path)
{
    std::ifstream ifs(path);
    if (!ifs) return Graph();
    std::string contents((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
    nlohmann::json j = nlohmann::json::parse(contents);
    return Graph::FromJson(j);
}
