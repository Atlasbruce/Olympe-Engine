/*
 * Olympe Blueprint Editor - Main Implementation
 * 
 * A functional editor for loading, editing, and saving entity blueprints.
 * This is Phase 1: Component property editing
 */

#include "../include/BlueprintEditor.h"
#include "../include/EntityBlueprint.h"
#include <iostream>
#include <string>
#include <limits>
#include <iomanip>

using namespace Olympe::Blueprint;

namespace Olympe
{
    // Editor state
    struct EditorState
    {
        EntityBlueprint currentBlueprint;
        std::string currentFilepath;
        bool hasUnsavedChanges = false;
        bool isRunning = true;
    };

    static EditorState s_EditorState;

    // Helper functions for console I/O
    void ClearScreen()
    {
        #ifdef _WIN32
            system("cls");
        #else
            system("clear");
        #endif
    }

    void Pause()
    {
        std::cout << "\nPress Enter to continue...";
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        std::cin.get();
    }

    std::string GetStringInput(const std::string& prompt)
    {
        std::cout << prompt;
        std::string input;
        std::getline(std::cin, input);
        return input;
    }

    float GetFloatInput(const std::string& prompt)
    {
        std::cout << prompt;
        float value;
        while (!(std::cin >> value))
        {
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            std::cout << "Invalid input. Please enter a number: ";
        }
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        return value;
    }

    int GetIntInput(const std::string& prompt)
    {
        std::cout << prompt;
        int value;
        while (!(std::cin >> value))
        {
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            std::cout << "Invalid input. Please enter an integer: ";
        }
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        return value;
    }

    // Display functions
    void DisplayHeader()
    {
        std::cout << "╔════════════════════════════════════════════════════════════╗\n";
        std::cout << "║          OLYMPE BLUEPRINT EDITOR - Phase 1                 ║\n";
        std::cout << "║          Component Property Editor                         ║\n";
        std::cout << "╚════════════════════════════════════════════════════════════╝\n";
        
        if (!s_EditorState.currentBlueprint.name.empty())
        {
            std::cout << "Current Blueprint: " << s_EditorState.currentBlueprint.name;
            if (s_EditorState.hasUnsavedChanges)
                std::cout << " *";
            std::cout << "\n";
            if (!s_EditorState.currentFilepath.empty())
                std::cout << "File: " << s_EditorState.currentFilepath << "\n";
        }
        std::cout << "\n";
    }

    void DisplayMainMenu()
    {
        std::cout << "═══════════════ MAIN MENU ═══════════════\n";
        std::cout << "1. New Blueprint\n";
        std::cout << "2. Load Blueprint\n";
        std::cout << "3. Save Blueprint\n";
        std::cout << "4. Save Blueprint As...\n";
        std::cout << "5. View Blueprint Details\n";
        std::cout << "6. Edit Components\n";
        std::cout << "7. Exit\n";
        std::cout << "═════════════════════════════════════════\n";
    }

    void DisplayComponentList()
    {
        auto& blueprint = s_EditorState.currentBlueprint;
        
        std::cout << "\n═══════════════ COMPONENTS ═══════════════\n";
        if (blueprint.components.empty())
        {
            std::cout << "  (No components)\n";
        }
        else
        {
            for (size_t i = 0; i < blueprint.components.size(); ++i)
            {
                std::cout << "  " << (i + 1) << ". " << blueprint.components[i].type << "\n";
            }
        }
        std::cout << "══════════════════════════════════════════\n";
    }

    void DisplayComponentDetails(const ComponentData& comp)
    {
        std::cout << "\n─── Component: " << comp.type << " ───\n";
        std::cout << "Properties:\n";
        std::cout << comp.properties.dump(2) << "\n";
        std::cout << "────────────────────────────────────\n";
    }

    // Editor operations
    void NewBlueprint()
    {
        ClearScreen();
        DisplayHeader();
        
        std::cout << "═══════════════ NEW BLUEPRINT ═══════════════\n\n";
        
        std::string name = GetStringInput("Enter blueprint name: ");
        std::string desc = GetStringInput("Enter description (optional): ");
        
        s_EditorState.currentBlueprint = EntityBlueprint(name);
        s_EditorState.currentBlueprint.description = desc;
        s_EditorState.currentFilepath = "";
        s_EditorState.hasUnsavedChanges = true;
        
        std::cout << "\n✓ New blueprint created: " << name << "\n";
        Pause();
    }

    void LoadBlueprint()
    {
        ClearScreen();
        DisplayHeader();
        
        std::cout << "═══════════════ LOAD BLUEPRINT ═══════════════\n\n";
        std::cout << "Example files:\n";
        std::cout << "  Blueprints/example_entity_simple.json\n";
        std::cout << "  Blueprints/example_entity_complete.json\n\n";
        
        std::string filepath = GetStringInput("Enter filepath: ");
        
        EntityBlueprint loaded = EntityBlueprint::LoadFromFile(filepath);
        
        if (loaded.name.empty())
        {
            std::cout << "\n✗ Failed to load blueprint from: " << filepath << "\n";
            std::cout << "  Make sure the file exists and is valid JSON.\n";
        }
        else
        {
            s_EditorState.currentBlueprint = loaded;
            s_EditorState.currentFilepath = filepath;
            s_EditorState.hasUnsavedChanges = false;
            
            std::cout << "\n✓ Blueprint loaded successfully!\n";
            std::cout << "  Name: " << loaded.name << "\n";
            std::cout << "  Components: " << loaded.components.size() << "\n";
        }
        
        Pause();
    }

    void SaveBlueprint()
    {
        if (s_EditorState.currentBlueprint.name.empty())
        {
            std::cout << "\n✗ No blueprint to save!\n";
            Pause();
            return;
        }
        
        if (s_EditorState.currentFilepath.empty())
        {
            // No filepath set, prompt for one
            ClearScreen();
            DisplayHeader();
            std::cout << "═══════════════ SAVE BLUEPRINT ═══════════════\n\n";
            std::string filepath = GetStringInput("Enter filepath to save: ");
            s_EditorState.currentFilepath = filepath;
        }
        
        bool success = s_EditorState.currentBlueprint.SaveToFile(s_EditorState.currentFilepath);
        
        if (success)
        {
            s_EditorState.hasUnsavedChanges = false;
            std::cout << "\n✓ Blueprint saved to: " << s_EditorState.currentFilepath << "\n";
        }
        else
        {
            std::cout << "\n✗ Failed to save blueprint!\n";
        }
        
        Pause();
    }

    void SaveBlueprintAs()
    {
        if (s_EditorState.currentBlueprint.name.empty())
        {
            std::cout << "\n✗ No blueprint to save!\n";
            Pause();
            return;
        }
        
        ClearScreen();
        DisplayHeader();
        std::cout << "═══════════════ SAVE BLUEPRINT AS ═══════════════\n\n";
        
        std::string filepath = GetStringInput("Enter new filepath: ");
        
        bool success = s_EditorState.currentBlueprint.SaveToFile(filepath);
        
        if (success)
        {
            s_EditorState.currentFilepath = filepath;
            s_EditorState.hasUnsavedChanges = false;
            std::cout << "\n✓ Blueprint saved to: " << filepath << "\n";
        }
        else
        {
            std::cout << "\n✗ Failed to save blueprint!\n";
        }
        
        Pause();
    }

    void ViewBlueprintDetails()
    {
        ClearScreen();
        DisplayHeader();
        
        if (s_EditorState.currentBlueprint.name.empty())
        {
            std::cout << "✗ No blueprint loaded!\n";
            Pause();
            return;
        }
        
        auto& blueprint = s_EditorState.currentBlueprint;
        
        std::cout << "═══════════════ BLUEPRINT DETAILS ═══════════════\n\n";
        std::cout << "Name: " << blueprint.name << "\n";
        std::cout << "Description: " << blueprint.description << "\n";
        std::cout << "Components: " << blueprint.components.size() << "\n\n";
        
        DisplayComponentList();
        
        std::cout << "\n─── Full JSON ───\n";
        std::cout << blueprint.ToJson().dump(2) << "\n";
        
        Pause();
    }

    void EditComponentProperty(ComponentData* comp)
    {
        bool editing = true;
        
        while (editing)
        {
            ClearScreen();
            DisplayHeader();
            DisplayComponentDetails(*comp);
            
            std::cout << "\n1. Edit property value\n";
            std::cout << "2. View JSON\n";
            std::cout << "3. Back\n";
            
            int choice = GetIntInput("\nChoice: ");
            
            switch (choice)
            {
            case 1:
            {
                std::string key = GetStringInput("Enter property key (e.g., 'position', 'x'): ");
                
                // Check if property exists and what type it is
                try
                {
                    if (comp->properties.contains(key))
                    {
                        auto& prop = comp->properties[key];
                        
                        if (prop.is_number())
                        {
                            float newValue = GetFloatInput("Enter new number value: ");
                            comp->properties[key] = newValue;
                            s_EditorState.hasUnsavedChanges = true;
                            std::cout << "✓ Updated!\n";
                        }
                        else if (prop.is_string())
                        {
                            std::string newValue = GetStringInput("Enter new string value: ");
                            comp->properties[key] = newValue;
                            s_EditorState.hasUnsavedChanges = true;
                            std::cout << "✓ Updated!\n";
                        }
                        else if (prop.is_boolean())
                        {
                            std::string input = GetStringInput("Enter new boolean value (true/false): ");
                            bool newValue = (input == "true" || input == "1");
                            comp->properties[key] = newValue;
                            s_EditorState.hasUnsavedChanges = true;
                            std::cout << "✓ Updated!\n";
                        }
                        else if (prop.is_object())
                        {
                            std::cout << "\nNested object detected. Enter sub-key (e.g., 'x'): ";
                            std::string subkey = GetStringInput("");
                            
                            if (prop.contains(subkey))
                            {
                                auto& subprop = prop[subkey];
                                if (subprop.is_number())
                                {
                                    float newValue = GetFloatInput("Enter new value: ");
                                    comp->properties[key][subkey] = newValue;
                                    s_EditorState.hasUnsavedChanges = true;
                                    std::cout << "✓ Updated!\n";
                                }
                                else if (subprop.is_string())
                                {
                                    std::string newValue = GetStringInput("Enter new value: ");
                                    comp->properties[key][subkey] = newValue;
                                    s_EditorState.hasUnsavedChanges = true;
                                    std::cout << "✓ Updated!\n";
                                }
                            }
                            else
                            {
                                std::cout << "✗ Sub-key not found\n";
                            }
                        }
                        else
                        {
                            std::cout << "✗ Unsupported property type\n";
                        }
                    }
                    else
                    {
                        std::cout << "✗ Property key not found\n";
                    }
                    Pause();
                }
                catch (const std::exception& e)
                {
                    std::cout << "✗ Error: " << e.what() << "\n";
                    Pause();
                }
                break;
            }
            case 2:
                std::cout << "\n" << comp->properties.dump(2) << "\n";
                Pause();
                break;
            case 3:
                editing = false;
                break;
            }
        }
    }

    void AddComponentMenu()
    {
        ClearScreen();
        DisplayHeader();
        
        std::cout << "═══════════════ ADD COMPONENT ═══════════════\n\n";
        std::cout << "Available Components:\n";
        std::cout << "1. Position\n";
        std::cout << "2. BoundingBox\n";
        std::cout << "3. VisualSprite\n";
        std::cout << "4. Movement\n";
        std::cout << "5. PhysicsBody\n";
        std::cout << "6. Health\n";
        std::cout << "7. AIBehavior\n";
        std::cout << "8. Custom (enter type manually)\n";
        std::cout << "9. Cancel\n";
        
        int choice = GetIntInput("\nChoice: ");
        
        ComponentData newComp;
        
        switch (choice)
        {
        case 1:
        {
            float x = GetFloatInput("Enter X position: ");
            float y = GetFloatInput("Enter Y position: ");
            newComp = CreatePositionComponent(x, y);
            break;
        }
        case 2:
        {
            float x = GetFloatInput("Enter X offset: ");
            float y = GetFloatInput("Enter Y offset: ");
            float w = GetFloatInput("Enter width: ");
            float h = GetFloatInput("Enter height: ");
            newComp = CreateBoundingBoxComponent(x, y, w, h);
            break;
        }
        case 3:
        {
            std::string path = GetStringInput("Enter sprite path: ");
            float w = GetFloatInput("Enter sprite width: ");
            float h = GetFloatInput("Enter sprite height: ");
            newComp = CreateVisualSpriteComponent(path, 0, 0, w, h);
            break;
        }
        case 4:
        {
            newComp = CreateMovementComponent(1.0f, 0.0f, 0.0f, 0.0f);
            break;
        }
        case 5:
        {
            float mass = GetFloatInput("Enter mass: ");
            float speed = GetFloatInput("Enter speed: ");
            newComp = CreatePhysicsBodyComponent(mass, speed);
            break;
        }
        case 6:
        {
            int maxHealth = GetIntInput("Enter max health: ");
            newComp = CreateHealthComponent(maxHealth, maxHealth);
            break;
        }
        case 7:
        {
            std::cout << "Behavior types: idle, patrol, chase, flee, wander\n";
            std::string behaviorType = GetStringInput("Enter behavior type: ");
            newComp = CreateAIBehaviorComponent(behaviorType);
            break;
        }
        case 8:
        {
            std::string type = GetStringInput("Enter component type: ");
            newComp.type = type;
            newComp.properties = json::object();
            break;
        }
        case 9:
            return;
        default:
            std::cout << "Invalid choice\n";
            Pause();
            return;
        }
        
        s_EditorState.currentBlueprint.AddComponent(newComp.type, newComp.properties);
        s_EditorState.hasUnsavedChanges = true;
        
        std::cout << "\n✓ Component '" << newComp.type << "' added!\n";
        Pause();
    }

    void EditComponentsMenu()
    {
        if (s_EditorState.currentBlueprint.name.empty())
        {
            std::cout << "\n✗ No blueprint loaded!\n";
            Pause();
            return;
        }
        
        bool editing = true;
        
        while (editing)
        {
            ClearScreen();
            DisplayHeader();
            DisplayComponentList();
            
            std::cout << "\n1. Add Component\n";
            std::cout << "2. Edit Component\n";
            std::cout << "3. Remove Component\n";
            std::cout << "4. Back to Main Menu\n";
            
            int choice = GetIntInput("\nChoice: ");
            
            switch (choice)
            {
            case 1:
                AddComponentMenu();
                break;
            case 2:
            {
                if (s_EditorState.currentBlueprint.components.empty())
                {
                    std::cout << "\n✗ No components to edit!\n";
                    Pause();
                    break;
                }
                
                int compIndex = GetIntInput("Enter component number: ") - 1;
                
                if (compIndex >= 0 && compIndex < (int)s_EditorState.currentBlueprint.components.size())
                {
                    EditComponentProperty(&s_EditorState.currentBlueprint.components[compIndex]);
                }
                else
                {
                    std::cout << "✗ Invalid component number!\n";
                    Pause();
                }
                break;
            }
            case 3:
            {
                if (s_EditorState.currentBlueprint.components.empty())
                {
                    std::cout << "\n✗ No components to remove!\n";
                    Pause();
                    break;
                }
                
                int compIndex = GetIntInput("Enter component number to remove: ") - 1;
                
                if (compIndex >= 0 && compIndex < (int)s_EditorState.currentBlueprint.components.size())
                {
                    std::string type = s_EditorState.currentBlueprint.components[compIndex].type;
                    s_EditorState.currentBlueprint.RemoveComponent(type);
                    s_EditorState.hasUnsavedChanges = true;
                    std::cout << "\n✓ Component '" << type << "' removed!\n";
                    Pause();
                }
                else
                {
                    std::cout << "✗ Invalid component number!\n";
                    Pause();
                }
                break;
            }
            case 4:
                editing = false;
                break;
            default:
                std::cout << "Invalid choice\n";
                Pause();
                break;
            }
        }
    }

    // Main Editor class implementation
    BlueprintEditor::BlueprintEditor()
    {
    }

    BlueprintEditor::~BlueprintEditor()
    {
    }

    void BlueprintEditor::Initialize()
    {
        s_EditorState = EditorState();
    }

    void BlueprintEditor::Shutdown()
    {
    }

    void BlueprintEditor::Run()
    {
        Initialize();
        
        while (s_EditorState.isRunning)
        {
            ClearScreen();
            DisplayHeader();
            DisplayMainMenu();
            
            int choice = GetIntInput("Enter choice: ");
            
            switch (choice)
            {
            case 1:
                NewBlueprint();
                break;
            case 2:
                LoadBlueprint();
                break;
            case 3:
                SaveBlueprint();
                break;
            case 4:
                SaveBlueprintAs();
                break;
            case 5:
                ViewBlueprintDetails();
                break;
            case 6:
                EditComponentsMenu();
                break;
            case 7:
                if (s_EditorState.hasUnsavedChanges)
                {
                    std::cout << "\nYou have unsaved changes. Are you sure you want to exit? (y/n): ";
                    std::string confirm;
                    std::getline(std::cin, confirm);
                    if (confirm == "y" || confirm == "Y")
                    {
                        s_EditorState.isRunning = false;
                    }
                }
                else
                {
                    s_EditorState.isRunning = false;
                }
                break;
            default:
                std::cout << "Invalid choice. Please try again.\n";
                Pause();
                break;
            }
        }
        
        Shutdown();
        
        std::cout << "\nThank you for using Olympe Blueprint Editor!\n";
    }

} // namespace Olympe
