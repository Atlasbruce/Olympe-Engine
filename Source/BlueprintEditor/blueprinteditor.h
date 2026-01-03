/*
Olympe Engine V2 2025
Nicolas Chereau
nchereau@gmail.com

Purpose:
- Blueprint Editor main header file.

*/

#pragma once
#include <SDL3/SDL.h>

class BlueprintEditor
{
public:
	BlueprintEditor() {}
	~BlueprintEditor() {}

	void Initialize();
	void Uninitialize();

    static BlueprintEditor& GetInstance()
    {
        static BlueprintEditor instance;
        return instance;
    }
    static BlueprintEditor& Get() { return GetInstance(); }

    void Render();
	void HandleEvent(SDL_Event* event); // Placeholder for future event handling

	static bool bshowEditor;
};