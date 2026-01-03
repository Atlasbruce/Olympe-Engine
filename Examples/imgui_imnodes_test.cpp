/*
 * Olympe Engine - ImGui & ImNodes Integration Test
 * 
 * This is a minimal Hello World demo showing ImGui and ImNodes are properly integrated.
 * Compile this to verify the technical prerequisites are met.
 * 
 * Build instructions:
 *   g++ -std=c++17 -I../Source/third_party \
 *       imgui_imnodes_test.cpp \
 *       ../Source/third_party/imgui/imgui.cpp \
 *       ../Source/third_party/imgui/imgui_demo.cpp \
 *       ../Source/third_party/imgui/imgui_draw.cpp \
 *       ../Source/third_party/imgui/imgui_tables.cpp \
 *       ../Source/third_party/imgui/imgui_widgets.cpp \
 *       ../Source/third_party/imnodes/imnodes.cpp \
 *       -o imgui_imnodes_test
 */

#include <iostream>
#include "imgui/imgui.h"
#include "imnodes/imnodes.h"

int main(int argc, char** argv)
{
    std::cout << "=== Olympe Engine - ImGui & ImNodes Integration Test ===" << std::endl;
    std::cout << std::endl;
    
    // Test ImGui
    std::cout << "[ImGui Test]" << std::endl;
    std::cout << "  Creating ImGui context..." << std::endl;
    
    IMGUI_CHECKVERSION();
    ImGuiContext* imgui_ctx = ImGui::CreateContext();
    
    if (imgui_ctx)
    {
        std::cout << "  ✓ ImGui context created successfully!" << std::endl;
        std::cout << "  ✓ ImGui version: " << IMGUI_VERSION << std::endl;
        
        // Test basic ImGui functionality
        ImGuiIO& io = ImGui::GetIO();
        std::cout << "  ✓ ImGuiIO initialized" << std::endl;
        
        // Build font atlas (required for rendering)
        unsigned char* pixels;
        int width, height;
        io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);
        std::cout << "  ✓ Font atlas built (" << width << "x" << height << ")" << std::endl;
        
        // Simulate a frame
        io.DisplaySize = ImVec2(1280, 720);
        io.DeltaTime = 1.0f / 60.0f;
        
        ImGui::NewFrame();
        
        // Create a simple window
        ImGui::Begin("Hello ImGui!");
        ImGui::Text("This is a test window from Olympe Engine");
        ImGui::Text("ImGui integration successful!");
        ImGui::End();
        
        ImGui::Render();
        
        std::cout << "  ✓ ImGui frame rendered (simulated)" << std::endl;
    }
    else
    {
        std::cout << "  ✗ Failed to create ImGui context!" << std::endl;
        return 1;
    }
    
    std::cout << std::endl;
    
    // Test ImNodes
    std::cout << "[ImNodes Test]" << std::endl;
    std::cout << "  Creating ImNodes context..." << std::endl;
    
    ImNodesContext* imnodes_ctx = ImNodes::CreateContext();
    
    if (imnodes_ctx)
    {
        std::cout << "  ✓ ImNodes context created successfully!" << std::endl;
        
        // ImNodes needs to be used within an ImGui frame
        ImGuiIO& io = ImGui::GetIO();
        io.DisplaySize = ImVec2(1280, 720);
        io.DeltaTime = 1.0f / 60.0f;
        
        ImGui::NewFrame();
        
        ImGui::Begin("Node Editor");
        
        // Test ImNodes API
        ImNodes::BeginNodeEditor();
        
        ImNodes::BeginNode(1);
        ImNodes::BeginNodeTitleBar();
        ImGui::TextUnformatted("Test Node");
        ImNodes::EndNodeTitleBar();
        
        ImNodes::BeginInputAttribute(2);
        ImGui::Text("Input");
        ImNodes::EndInputAttribute();
        
        ImNodes::BeginOutputAttribute(3);
        ImGui::Text("Output");
        ImNodes::EndOutputAttribute();
        
        ImNodes::EndNode();
        
        ImNodes::EndNodeEditor();
        
        ImGui::End();
        
        ImGui::Render();
        
        std::cout << "  ✓ ImNodes node created (simulated)" << std::endl;
        
        ImNodes::DestroyContext(imnodes_ctx);
        std::cout << "  ✓ ImNodes context destroyed" << std::endl;
    }
    else
    {
        std::cout << "  ✗ Failed to create ImNodes context!" << std::endl;
        ImGui::DestroyContext(imgui_ctx);
        return 1;
    }
    
    std::cout << std::endl;
    
    // Cleanup
    ImGui::DestroyContext(imgui_ctx);
    std::cout << "[Cleanup]" << std::endl;
    std::cout << "  ✓ ImGui context destroyed" << std::endl;
    
    std::cout << std::endl;
    std::cout << "=== All Tests Passed! ===" << std::endl;
    std::cout << "ImGui and ImNodes are properly integrated and ready to use." << std::endl;
    
    return 0;
}
