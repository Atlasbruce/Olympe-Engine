/*
Olympe Engine V2 2025
Nicolas Chereau
nchereau@gmail.com

Purpose:
- Blueprint Editor implementation file.

*/
#include "BlueprintEditor.h"

void BlueprintEditorPlugin::Render() {
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
}