/*
Olympe Engine V2 2025
Nicolas Chereau
nchereau@gmail.com

Purpose:
- Blueprint Editor main header file.

*/

#pragma once

#include "BlueprintEditor.h"
#include "../third_party/imnodes/imnodes.h"
#include "../third_party/imgui/imgui.h"
#include "../third_party/imgui/backends/imgui_impl_sdl3.h"
#include "../third_party/imgui/backends/imgui_impl_opengl3.h"

//extern SDL_Window* window;
//void* sdl_gl_context;

bool BlueprintEditor::bshowEditor = true;

//-------------------------------------------------------------
void BlueprintEditor::Initialize()
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImNodes::CreateContext();
    //ImGui_ImplSDL3_InitForOpenGL(window, sdl_gl_context);
    //ImGui_ImplOpenGL3_Init("#version 130"); 
}

//-------------------------------------------------------------
void BlueprintEditor::Uninitialize()
{
    //ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();
    ImNodes::DestroyContext();
}
//-------------------------------------------------------------
void BlueprintEditor::Render()
{
    if (bshowEditor == false)
		return;

	// ImGui frame start
    //ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();
    ImNodes::BeginNodeEditor();

	//-- Blueprint Editor window
    ImGui::Begin("Blueprint Editor (Minimal Demo)");
    ImGui::Text("Bienvenue dans l'éditeur Blueprint Olympe !");
    ImGui::Text("Voici un exemple de node graph avec ImNodes.");

    // Minimal Node graph
    ImNodes::BeginNode(1);
    ImNodes::BeginNodeTitleBar();
    ImGui::TextUnformatted("Node A");
    ImNodes::EndNodeTitleBar();
    ImNodes::BeginInputAttribute(2); ImGui::Text("Entrée"); ImNodes::EndInputAttribute();
    ImNodes::BeginOutputAttribute(3); ImGui::Text("Sortie"); ImNodes::EndOutputAttribute();
    ImNodes::EndNode();

    ImNodes::BeginNode(4);
    ImNodes::BeginNodeTitleBar();
    ImGui::TextUnformatted("Node B");
    ImNodes::EndNodeTitleBar();
    ImNodes::BeginInputAttribute(5); ImGui::Text("Entrée"); ImNodes::EndInputAttribute();
    ImNodes::BeginOutputAttribute(6); ImGui::Text("Sortie"); ImNodes::EndOutputAttribute();
    ImNodes::EndNode();

    ImNodes::Link(100, 3, 5); // Relie la sortie du noeud A à l’entrée de B

    ImGui::End();

	//-- ImGui finishing
    ImNodes::EndNodeEditor();
    ImGui::Render();

    // 5. Rendu)
    //SDL_GL_MakeCurrent(window, sdl_gl_context);   // si besoin
    //glClear(GL_COLOR_BUFFER_BIT);
    //ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    //SDL_GL_SwapWindow(window);

}
//-------------------------------------------------------------
void BlueprintEditor::HandleEvent(SDL_Event* event)
{
}
