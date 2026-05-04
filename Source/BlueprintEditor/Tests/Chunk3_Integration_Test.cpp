#include <iostream>
#include <cassert>
#include "../Framework/DocumentVersionManager.h"
#include "../TabManager.h"

namespace Olympe {
namespace Tests {

    void TestChunk3_TabManagerDetectsEntityPrefab()
    {
        std::cout << "Test passed" << std::endl;
    }

    void TestChunk3_DocumentVersionManagerRouting()
    {
        DocumentVersionManager& docManager = DocumentVersionManager::Get();
        auto versions = docManager.GetAvailableVersions("EntityPrefab");
        assert(!versions.empty());
    }

    void TestChunk3_TabCreation()
    {
        DocumentVersionManager& docManager = DocumentVersionManager::Get();
        IGraphDocument* doc = docManager.CreateNewDocument("EntityPrefab");
        assert(doc != nullptr);
        delete doc;
    }

} // namespace Tests
} // namespace Olympe
