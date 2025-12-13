#pragma once

namespace Olympe
{
    class BlueprintEditor
    {
    public:
        BlueprintEditor();
        ~BlueprintEditor();

        void Initialize();
        void Shutdown();
        void Run();  // Main editor loop
    };
}
