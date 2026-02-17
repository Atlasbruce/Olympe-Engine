/**
 * @file AnimationEditorWindow.cpp
 * @brief Implementation of Animation Editor window
 * @author Olympe Engine - Animation System
 * @date 2025
 */

#include "AnimationEditorWindow.h"
#include "../DataManager.h"
#include "../system/system_utils.h"
#include "../json_helper.h"
#include "../GameEngine.h"
#include "../third_party/imgui/imgui.h"
#include "../third_party/imgui/backends/imgui_impl_sdl3.h"
#include "../third_party/imgui/backends/imgui_impl_sdlrenderer3.h"
#include <SDL3/SDL.h>
#include <algorithm>
#include <ctime>

#ifdef _WIN32
    #include <windows.h>
#else
    #include <dirent.h>
    #include <sys/stat.h>
#endif

using json = nlohmann::json;

namespace Olympe
{

// ========================================================================
// Constructor / Destructor
// ========================================================================

AnimationEditorWindow::AnimationEditorWindow()
{
    SYSTEM_LOG << "AnimationEditorWindow: Initialized\n";
}

AnimationEditorWindow::~AnimationEditorWindow()
{
    DestroySeparateWindow();
    SYSTEM_LOG << "AnimationEditorWindow: Destroyed\n";
}

// ========================================================================
// Public API
// ========================================================================

void AnimationEditorWindow::Toggle()
{
    m_isOpen = !m_isOpen;
    
    if (m_isOpen)
    {
        // Create separate window if it doesn't exist
        if (!m_separateWindow)
        {
            CreateSeparateWindow();
        }
        
        // Show the window
        if (m_separateWindow)
        {
            SDL_ShowWindow(m_separateWindow);
        }
        
        SYSTEM_LOG << "AnimationEditorWindow: Opened\n";
        
        // Load initial bank if no bank is loaded
        if (!m_hasBankLoaded)
        {
            // Try to load banks from directory
            auto bankFiles = ScanBankDirectory("GameData/Animations/Banks");
            if (!bankFiles.empty())
            {
                SYSTEM_LOG << "AnimationEditorWindow: Found " << bankFiles.size() << " banks\n";
            }
        }
    }
    else
    {
        // Hide the window
        if (m_separateWindow)
        {
            SDL_HideWindow(m_separateWindow);
        }
        
        SYSTEM_LOG << "AnimationEditorWindow: Closed\n";
        
        // Prompt for unsaved changes
        if (m_isDirty)
        {
            PromptUnsavedChanges();
        }
    }
}

void AnimationEditorWindow::UpdatePreview(float deltaTime)
{
    if (!m_isPreviewPlaying || m_isPreviewPaused)
        return;
    
    if (m_selectedSequenceIndex < 0 || m_selectedSequenceIndex >= static_cast<int>(m_currentBank.animations.size()))
        return;
    
    // Get selected sequence
    auto it = m_currentBank.animations.begin();
    std::advance(it, m_selectedSequenceIndex);
    const AnimationSequence& seq = it->second;
    
    // Update frame timer
    m_previewFrameTimer += deltaTime * m_previewSpeed;
    
    if (m_previewFrameTimer >= seq.frameDuration)
    {
        m_previewFrameTimer = 0.0f;
        m_previewCurrentFrame++;
        
        int maxFrame = seq.startFrame + seq.frameCount - 1;
        
        if (m_previewCurrentFrame > maxFrame)
        {
            if (seq.loop)
            {
                m_previewCurrentFrame = seq.startFrame;
            }
            else
            {
                m_previewCurrentFrame = maxFrame;
                m_isPreviewPlaying = false;
            }
        }
    }
}

void AnimationEditorWindow::Render()
{
    if (!m_isOpen)
        return;
    
    // Set window size and position
    ImGui::SetNextWindowSize(ImVec2(1400, 800), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowPos(ImVec2(100, 100), ImGuiCond_FirstUseEver);
    
    // Window flags
    ImGuiWindowFlags windowFlags = ImGuiWindowFlags_MenuBar;
    if (m_isDirty)
        windowFlags |= ImGuiWindowFlags_UnsavedDocument;
    
    // Begin main window
    if (!ImGui::Begin("Animation Editor [F9]", &m_isOpen, windowFlags))
    {
        ImGui::End();
        return;
    }
    
    // Render menu bar
    RenderMainMenu();
    
    // 3-Column Layout
    ImGui::BeginChild("LeftPanel", ImVec2(200, 0), true);
    RenderBankListPanel();
    ImGui::EndChild();
    
    ImGui::SameLine();
    
    ImGui::BeginChild("MiddlePanel", ImVec2(600, 0), true);
    // Tabs for Spritesheets and Sequences
    if (ImGui::BeginTabBar("EditorTabs"))
    {
        if (ImGui::BeginTabItem("Spritesheets"))
        {
            m_activeTab = 0;
            RenderSpritesheetPanel();
            ImGui::EndTabItem();
        }
        
        if (ImGui::BeginTabItem("Sequences"))
        {
            m_activeTab = 1;
            RenderSequencePanel();
            ImGui::EndTabItem();
        }
        
        ImGui::EndTabBar();
    }
    ImGui::EndChild();
    
    ImGui::SameLine();
    
    ImGui::BeginChild("RightPanel", ImVec2(0, 0), true);
    RenderPreviewPanel();
    ImGui::Separator();
    RenderPropertiesPanel();
    ImGui::EndChild();
    
    ImGui::End();
}

// ========================================================================
// UI Panel Rendering
// ========================================================================

void AnimationEditorWindow::RenderMainMenu()
{
    if (ImGui::BeginMenuBar())
    {
        if (ImGui::BeginMenu("File"))
        {
            if (ImGui::MenuItem("New Bank", "Ctrl+N"))
            {
                NewBank();
            }
            
            if (ImGui::MenuItem("Open Bank", "Ctrl+O"))
            {
                m_showOpenBankDialog = true;
            }
            
            ImGui::Separator();
            
            if (ImGui::MenuItem("Save", "Ctrl+S", false, m_hasBankLoaded))
            {
                SaveBank();
            }
            
            if (ImGui::MenuItem("Save As", "Ctrl+Shift+S", false, m_hasBankLoaded))
            {
                SaveBankAs();
            }
            
            ImGui::Separator();
            
            if (ImGui::MenuItem("Close", "Ctrl+W"))
            {
                m_isOpen = false;
            }
            
            ImGui::EndMenu();
        }
        
        if (ImGui::BeginMenu("Edit"))
        {
            if (ImGui::MenuItem("Add Spritesheet", "Ctrl+Shift+A", false, m_hasBankLoaded))
            {
                AddSpritesheet();
            }
            
            if (ImGui::MenuItem("Add Sequence", "Ctrl+A", false, m_hasBankLoaded))
            {
                AddSequence();
            }
            
            ImGui::Separator();
            
            if (ImGui::MenuItem("Remove Selected", "Del", false, m_selectedSpritesheetIndex >= 0 || m_selectedSequenceIndex >= 0))
            {
                if (m_activeTab == 0 && m_selectedSpritesheetIndex >= 0)
                {
                    RemoveSpritesheet(m_selectedSpritesheetIndex);
                }
                else if (m_activeTab == 1 && m_selectedSequenceIndex >= 0)
                {
                    RemoveSequence(m_selectedSequenceIndex);
                }
            }
            
            ImGui::EndMenu();
        }
        
        if (ImGui::BeginMenu("View"))
        {
            ImGui::Checkbox("Show Grid", &m_showGrid);
            
            ImGui::EndMenu();
        }
        
        ImGui::EndMenuBar();
    }
}

void AnimationEditorWindow::RenderBankListPanel()
{
    ImGui::Text("Animation Banks");
    ImGui::Separator();
    
    if (ImGui::Button("+ New Bank", ImVec2(-1, 0)))
    {
        NewBank();
    }
    
    // List available banks
    auto bankFiles = ScanBankDirectory("GameData/Animations/Banks");
    
    ImGui::BeginChild("BankList", ImVec2(0, 0), false);
    
    for (const auto& filepath : bankFiles)
    {
        // Extract filename
        size_t lastSlash = filepath.find_last_of("/\\");
        std::string filename = (lastSlash != std::string::npos) ? filepath.substr(lastSlash + 1) : filepath;
        
        bool isSelected = (filepath == m_currentBankPath);
        
        if (ImGui::Selectable(filename.c_str(), isSelected))
        {
            if (m_isDirty)
            {
                if (PromptUnsavedChanges())
                {
                    OpenBank(filepath);
                }
            }
            else
            {
                OpenBank(filepath);
            }
        }
    }
    
    ImGui::EndChild();
}

void AnimationEditorWindow::RenderSpritesheetPanel()
{
    if (!m_hasBankLoaded)
    {
        ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "No bank loaded. Create or open a bank.");
        return;
    }
    
    ImGui::Text("Spritesheets (%d)", static_cast<int>(m_currentBank.spritesheets.size()));
    
    if (ImGui::Button("+ Add Spritesheet"))
    {
        AddSpritesheet();
    }
    
    ImGui::Separator();
    
    // List spritesheets
    ImGui::BeginChild("SpritesheetList", ImVec2(250, 0), true);
    
    for (int i = 0; i < static_cast<int>(m_currentBank.spritesheets.size()); ++i)
    {
        const auto& sheet = m_currentBank.spritesheets[i];
        bool isSelected = (i == m_selectedSpritesheetIndex);
        
        if (ImGui::Selectable(sheet.id.c_str(), isSelected))
        {
            m_selectedSpritesheetIndex = i;
        }
    }
    
    ImGui::EndChild();
    
    ImGui::SameLine();
    
    // Properties editor
    ImGui::BeginChild("SpritesheetProperties", ImVec2(0, 0), true);
    
    if (m_selectedSpritesheetIndex >= 0 && m_selectedSpritesheetIndex < static_cast<int>(m_currentBank.spritesheets.size()))
    {
        auto& sheet = m_currentBank.spritesheets[m_selectedSpritesheetIndex];
        
        ImGui::Text("Spritesheet Properties");
        ImGui::Separator();
        
        // ID
        char idBuf[256];
        strncpy_s(idBuf, sheet.id.c_str(), sizeof(idBuf) - 1);
        idBuf[sizeof(idBuf) - 1] = '\0';
        if (ImGui::InputText("ID", idBuf, sizeof(idBuf)))
        {
            sheet.id = idBuf;
            MarkDirty();
        }
        
        // Path
        char pathBuf[512];
        strncpy_s(pathBuf, sheet.path.c_str(), sizeof(pathBuf) - 1);
        pathBuf[sizeof(pathBuf) - 1] = '\0';
        if (ImGui::InputText("Path", pathBuf, sizeof(pathBuf)))
        {
            sheet.path = pathBuf;
            MarkDirty();
        }
        
        // Description
        char descBuf[512];
        strncpy_s(descBuf, sheet.description.c_str(), sizeof(descBuf) - 1);
        descBuf[sizeof(descBuf) - 1] = '\0';
        if (ImGui::InputText("Description", descBuf, sizeof(descBuf)))
        {
            sheet.description = descBuf;
            MarkDirty();
        }
        
        ImGui::Separator();
        ImGui::Text("Grid Layout");
        
        if (ImGui::InputInt("Frame Width", &sheet.frameWidth))
        {
            MarkDirty();
        }
        
        if (ImGui::InputInt("Frame Height", &sheet.frameHeight))
        {
            MarkDirty();
        }
        
        if (ImGui::InputInt("Columns", &sheet.columns))
        {
            MarkDirty();
        }
        
        if (ImGui::InputInt("Rows", &sheet.rows))
        {
            MarkDirty();
        }
        
        if (ImGui::InputInt("Total Frames", &sheet.totalFrames))
        {
            MarkDirty();
        }
        
        if (ImGui::InputInt("Spacing", &sheet.spacing))
        {
            MarkDirty();
        }
        
        if (ImGui::InputInt("Margin", &sheet.margin))
        {
            MarkDirty();
        }
        
        if (ImGui::Button("Auto-Detect Grid"))
        {
            AutoDetectGrid(sheet);
        }
        
        ImGui::Separator();
        ImGui::Text("Hotspot");
        
        if (ImGui::InputFloat("Hotspot X", &sheet.hotspot.x))
        {
            MarkDirty();
        }
        
        if (ImGui::InputFloat("Hotspot Y", &sheet.hotspot.y))
        {
            MarkDirty();
        }
        
        // Preview spritesheet image
        ImGui::Separator();
        ImGui::Text("Preview");
        
        SDL_Texture* tex = LoadSpritesheetTexture(sheet.path);
        if (tex)
        {
            // Get texture dimensions
            float texW = 0.f;
            float texH = 0.f;
            SDL_GetTextureSize(tex, &texW, &texH);
            
            // Calculate preview size
            float previewW = static_cast<float>(texW) * m_spritesheetZoom;
            float previewH = static_cast<float>(texH) * m_spritesheetZoom;
            
            // Render texture
            ImGui::Image((ImTextureID)(intptr_t)tex, ImVec2(previewW, previewH));
            
            // Grid overlay
            if (m_showGrid && sheet.frameWidth > 0 && sheet.frameHeight > 0 && sheet.columns > 0)
            {
                ImDrawList* drawList = ImGui::GetWindowDrawList();
                ImVec2 p = ImGui::GetItemRectMin();
                
                ImU32 gridColor = IM_COL32(255, 255, 0, 128);
                
                // Vertical lines
                for (int col = 0; col <= sheet.columns; ++col)
                {
                    float x = p.x + (sheet.margin + col * (sheet.frameWidth + sheet.spacing)) * m_spritesheetZoom;
                    drawList->AddLine(ImVec2(x, p.y), ImVec2(x, p.y + previewH), gridColor);
                }
                
                // Horizontal lines
                for (int row = 0; row <= sheet.rows; ++row)
                {
                    float y = p.y + (sheet.margin + row * (sheet.frameHeight + sheet.spacing)) * m_spritesheetZoom;
                    drawList->AddLine(ImVec2(p.x, y), ImVec2(p.x + previewW, y), gridColor);
                }
            }
        }
        else
        {
            ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.5f, 1.0f), "Texture not loaded");
        }
        
        // Zoom controls
        ImGui::Separator();
        ImGui::SliderFloat("Zoom", &m_spritesheetZoom, 0.1f, 4.0f);
    }
    else
    {
        ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Select a spritesheet to edit");
    }
    
    ImGui::EndChild();
}

void AnimationEditorWindow::RenderSequencePanel()
{
    if (!m_hasBankLoaded)
    {
        ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "No bank loaded. Create or open a bank.");
        return;
    }
    
    ImGui::Text("Sequences (%d)", static_cast<int>(m_currentBank.animations.size()));
    
    if (ImGui::Button("+ Add Sequence"))
    {
        AddSequence();
    }
    
    ImGui::Separator();
    
    // List sequences
    ImGui::BeginChild("SequenceList", ImVec2(250, 0), true);
    
    int index = 0;
    for (auto it = m_currentBank.animations.begin(); it != m_currentBank.animations.end(); ++it, ++index)
    {
        const auto& seq = it->second;
        bool isSelected = (index == m_selectedSequenceIndex);
        
        if (ImGui::Selectable(seq.name.c_str(), isSelected))
        {
            m_selectedSequenceIndex = index;
            ResetPreview();
        }
    }
    
    ImGui::EndChild();
    
    ImGui::SameLine();
    
    // Properties editor
    ImGui::BeginChild("SequenceProperties", ImVec2(0, 0), true);
    
    if (m_selectedSequenceIndex >= 0 && m_selectedSequenceIndex < static_cast<int>(m_currentBank.animations.size()))
    {
        auto it = m_currentBank.animations.begin();
        std::advance(it, m_selectedSequenceIndex);
        auto& seq = it->second;
        
        ImGui::Text("Sequence Properties");
        ImGui::Separator();
        
        // Name
        char nameBuf[256];
        strncpy_s(nameBuf, sizeof(nameBuf), seq.name.c_str(), _TRUNCATE);
        nameBuf[sizeof(nameBuf) - 1] = '\0';
        if (ImGui::InputText("Name", nameBuf, sizeof(nameBuf)))
        {
            std::string oldName = seq.name;
            seq.name = nameBuf;
            
            // Update key in map
            if (oldName != seq.name)
            {
                auto findIt = m_currentBank.animations.find(oldName);
                if (findIt != m_currentBank.animations.end())
                {
                    AnimationSequence moved = std::move(findIt->second);
                    m_currentBank.animations.erase(findIt);
                    m_currentBank.animations[seq.name] = std::move(moved);
                }
            }
            
            MarkDirty();
        }
        
        // Spritesheet selector
        ImGui::Text("Spritesheet");
        
        if (ImGui::BeginCombo("##SpritesheetSelector", seq.spritesheetId.c_str()))
        {
            for (const auto& sheet : m_currentBank.spritesheets)
            {
                bool isSelected = (seq.spritesheetId == sheet.id);
                if (ImGui::Selectable(sheet.id.c_str(), isSelected))
                {
                    seq.spritesheetId = sheet.id;
                    MarkDirty();
                }
                
                if (isSelected)
                {
                    ImGui::SetItemDefaultFocus();
                }
            }
            ImGui::EndCombo();
        }
        
        ImGui::Separator();
        ImGui::Text("Frame Range");
        
        if (ImGui::InputInt("Start Frame", &seq.startFrame))
        {
            if (seq.startFrame < 0) seq.startFrame = 0;
            MarkDirty();
        }
        
        if (ImGui::InputInt("Frame Count", &seq.frameCount))
        {
            if (seq.frameCount < 1) seq.frameCount = 1;
            MarkDirty();
        }
        
        ImGui::Separator();
        ImGui::Text("Playback Settings");
        
        if (ImGui::InputFloat("Frame Duration (s)", &seq.frameDuration))
        {
            if (seq.frameDuration < 0.001f) seq.frameDuration = 0.001f;
            MarkDirty();
        }
        
        if (ImGui::Checkbox("Loop", &seq.loop))
        {
            MarkDirty();
        }
        
        if (ImGui::SliderFloat("Speed", &seq.speed, 0.1f, 5.0f))
        {
            MarkDirty();
        }
        
        // Next animation
        char nextAnimBuf[256];
        strncpy_s(nextAnimBuf, sizeof(nextAnimBuf), seq.nextAnimation.c_str(), _TRUNCATE);
        nextAnimBuf[sizeof(nextAnimBuf) - 1] = '\0';
        if (ImGui::InputText("Next Animation", nextAnimBuf, sizeof(nextAnimBuf)))
        {
            seq.nextAnimation = nextAnimBuf;
            MarkDirty();
        }
        
        ImGui::Separator();
        ImGui::Text("Stats");
        ImGui::Text("Total Duration: %.2f s", seq.GetTotalDuration());
        ImGui::Text("Effective FPS: %.2f", seq.GetEffectiveFPS());
    }
    else
    {
        ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Select a sequence to edit");
    }
    
    ImGui::EndChild();
}

void AnimationEditorWindow::RenderPreviewPanel()
{
    ImGui::Text("Preview");
    ImGui::Separator();
    
    if (!m_hasBankLoaded)
    {
        ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "No bank loaded");
        return;
    }
    
    if (m_selectedSequenceIndex < 0 || m_selectedSequenceIndex >= static_cast<int>(m_currentBank.animations.size()))
    {
        ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "No sequence selected");
        return;
    }
    
    // Get selected sequence
    auto it = m_currentBank.animations.begin();
    std::advance(it, m_selectedSequenceIndex);
    const AnimationSequence& seq = it->second;
    
    // Playback controls
    if (ImGui::Button(m_isPreviewPlaying && !m_isPreviewPaused ? "Pause" : "Play"))
    {
        if (m_isPreviewPlaying && !m_isPreviewPaused)
        {
            PausePreview();
        }
        else
        {
            StartPreview();
        }
    }
    
    ImGui::SameLine();
    
    if (ImGui::Button("Stop"))
    {
        StopPreview();
    }
    
    ImGui::SameLine();
    
    ImGui::SliderFloat("Speed", &m_previewSpeed, 0.1f, 5.0f);
    
    // Frame scrubber
    int maxFrame = seq.startFrame + seq.frameCount - 1;
    if (ImGui::SliderInt("Frame", &m_previewCurrentFrame, seq.startFrame, maxFrame))
    {
        // Manual frame seek
        m_previewFrameTimer = 0.0f;
    }
    
    ImGui::Text("Frame: %d / %d", m_previewCurrentFrame - seq.startFrame + 1, seq.frameCount);
    
    ImGui::Separator();
    
    // Render current frame
    RenderPreviewFrame();
}

void AnimationEditorWindow::RenderPreviewFrame()
{
    if (m_selectedSequenceIndex < 0 || m_selectedSequenceIndex >= static_cast<int>(m_currentBank.animations.size()))
        return;
    
    // Get selected sequence
    auto it = m_currentBank.animations.begin();
    std::advance(it, m_selectedSequenceIndex);
    const AnimationSequence& seq = it->second;
    
    // Get spritesheet
    const SpritesheetInfo* sheet = m_currentBank.GetSpritesheet(seq.spritesheetId);
    if (!sheet)
    {
        ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.5f, 1.0f), "Spritesheet not found: %s", seq.spritesheetId.c_str());
        return;
    }
    
    // Load texture
    SDL_Texture* tex = LoadSpritesheetTexture(sheet->path);
    if (!tex)
    {
        ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.5f, 1.0f), "Failed to load texture: %s", sheet->path.c_str());
        return;
    }
    
    // Calculate srcRect for current frame
    int frameIndex = m_previewCurrentFrame;
    if (frameIndex < 0) frameIndex = 0;
    if (frameIndex >= sheet->totalFrames) frameIndex = sheet->totalFrames - 1;
    
    int row = frameIndex / sheet->columns;
    int col = frameIndex % sheet->columns;
    
    float srcX = static_cast<float>(sheet->margin + col * (sheet->frameWidth + sheet->spacing));
    float srcY = static_cast<float>(sheet->margin + row * (sheet->frameHeight + sheet->spacing));
    float srcW = static_cast<float>(sheet->frameWidth);
    float srcH = static_cast<float>(sheet->frameHeight);
    
    // Get texture dimensions for UV calculation
    float texW = 0.f;
    float texH = 0.f;
    SDL_GetTextureSize(tex, &texW, &texH);
    
    // Calculate UV coordinates
    ImVec2 uv0(srcX / texW, srcY / texH);
    ImVec2 uv1((srcX + srcW) / texW, (srcY + srcH) / texH);
    
    // Render with 2x scale
    ImVec2 previewSize(srcW * 2.0f, srcH * 2.0f);
    
    // Center the preview
    ImVec2 availSize = ImGui::GetContentRegionAvail();
    ImVec2 cursorPos = ImGui::GetCursorPos();
    ImVec2 centeredPos(cursorPos.x + (availSize.x - previewSize.x) * 0.5f, cursorPos.y);
    
    if (centeredPos.x < cursorPos.x) centeredPos.x = cursorPos.x;
    
    ImGui::SetCursorPos(centeredPos);
    ImGui::Image((ImTextureID)(intptr_t)tex, previewSize, uv0, uv1);
}

void AnimationEditorWindow::RenderPropertiesPanel()
{
    ImGui::Text("Bank Properties");
    ImGui::Separator();
    
    if (!m_hasBankLoaded)
    {
        ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "No bank loaded");
        return;
    }
    
    // Bank ID
    char bankIdBuf[256];
    strncpy_s(bankIdBuf, m_currentBank.bankId.c_str(), sizeof(bankIdBuf) - 1);
    bankIdBuf[sizeof(bankIdBuf) - 1] = '\0';
    if (ImGui::InputText("Bank ID", bankIdBuf, sizeof(bankIdBuf)))
    {
        m_currentBank.bankId = bankIdBuf;
        MarkDirty();
    }
    
    // Description
    char descBuf[1024];
    strncpy_s(descBuf, m_currentBank.description.c_str(), sizeof(descBuf) - 1);
    descBuf[sizeof(descBuf) - 1] = '\0';
    if (ImGui::InputTextMultiline("Description", descBuf, sizeof(descBuf), ImVec2(-1, 80)))
    {
        m_currentBank.description = descBuf;
        MarkDirty();
    }
    
    // Author
    char authorBuf[256];
    strncpy_s(authorBuf, m_currentBank.author.c_str(), sizeof(authorBuf) - 1);
    authorBuf[sizeof(authorBuf) - 1] = '\0';
    if (ImGui::InputText("Author", authorBuf, sizeof(authorBuf)))
    {
        m_currentBank.author = authorBuf;
        MarkDirty();
    }
    
    // Dates (read-only)
    ImGui::Text("Created: %s", m_currentBank.createdDate.c_str());
    ImGui::Text("Modified: %s", m_currentBank.lastModifiedDate.c_str());
}

// ========================================================================
// File Operations
// ========================================================================

void AnimationEditorWindow::NewBank()
{
    if (m_isDirty)
    {
        if (!PromptUnsavedChanges())
            return;
    }
    
    m_currentBank = AnimationBank();
    m_currentBank.bankId = "new_bank";
    m_currentBank.author = "Olympe Engine";
    
    // Set created date
    time_t now = time(nullptr);
    char dateBuf[64];
    struct tm timeInfo;
    gmtime_s(&timeInfo, &now);
    strftime(dateBuf, sizeof(dateBuf), "%Y-%m-%dT%H:%M:%SZ", &timeInfo);
    m_currentBank.createdDate = dateBuf;
    m_currentBank.lastModifiedDate = dateBuf;
    
    m_currentBankPath = "";
    m_hasBankLoaded = true;
    m_selectedSpritesheetIndex = -1;
    m_selectedSequenceIndex = -1;
    
    MarkDirty();
    
    SYSTEM_LOG << "AnimationEditorWindow: Created new bank\n";
}

void AnimationEditorWindow::OpenBank(const std::string& filepath)
{
    ImportBankJSON(filepath);
}

void AnimationEditorWindow::SaveBank()
{
    if (m_currentBankPath.empty())
    {
        SaveBankAs();
        return;
    }
    
    ExportBankJSON(m_currentBankPath);
    ClearDirty();
}

void AnimationEditorWindow::SaveBankAs()
{
    // For now, use a simple path
    std::string filepath = "GameData/Animations/Banks/" + m_currentBank.bankId + ".json";
    
    ExportBankJSON(filepath);
    m_currentBankPath = filepath;
    ClearDirty();
}

void AnimationEditorWindow::ImportBankJSON(const std::string& filepath)
{
    json j;
    if (!JsonHelper::LoadJsonFromFile(filepath, j))
    {
        SYSTEM_LOG << "AnimationEditorWindow: Failed to load JSON from " << filepath << "\n";
        return;
    }
    
    try
    {
        AnimationBank bank;

        // Parse basic info
        bank.bankId = JsonHelper::GetString(j, "bankId", "unknown");
        bank.description = JsonHelper::GetString(j, "description", "");

        // Parse metadata
        if (j.contains("metadata"))
        {
            const json& meta = j["metadata"];
            bank.author = JsonHelper::GetString(meta, "author", "");
            bank.createdDate = JsonHelper::GetString(meta, "created", "");
            bank.lastModifiedDate = JsonHelper::GetString(meta, "lastModified", "");
            
            if (meta.contains("tags") && meta["tags"].is_array())
            {
                for (const auto& tag : meta["tags"])
                {
                    if (tag.is_string())
                    {
                        bank.tags.push_back(tag.get<std::string>());
                    }
                }
            }
        }
        
        // Parse spritesheets
        if (j.contains("spritesheets") && j["spritesheets"].is_array())
        {
            for (const auto& sheetJson : j["spritesheets"])
            {
                SpritesheetInfo sheet;
                sheet.id = JsonHelper::GetString(sheetJson, "id", "");
                sheet.path = JsonHelper::GetString(sheetJson, "path", "");
                sheet.description = JsonHelper::GetString(sheetJson, "description", "");
                sheet.frameWidth = JsonHelper::GetInt(sheetJson, "frameWidth", 32);
                sheet.frameHeight = JsonHelper::GetInt(sheetJson, "frameHeight", 32);
                sheet.columns = JsonHelper::GetInt(sheetJson, "columns", 1);
                sheet.rows = JsonHelper::GetInt(sheetJson, "rows", 1);
                sheet.totalFrames = JsonHelper::GetInt(sheetJson, "totalFrames", 1);
                sheet.spacing = JsonHelper::GetInt(sheetJson, "spacing", 0);
                sheet.margin = JsonHelper::GetInt(sheetJson, "margin", 0);

                if (sheetJson.contains("hotspot"))
                {
                    sheet.hotspot.x = JsonHelper::GetFloat(sheetJson["hotspot"], "x", 0.0f);
                    sheet.hotspot.y = JsonHelper::GetFloat(sheetJson["hotspot"], "y", 0.0f);
                }
                
                bank.spritesheets.push_back(sheet);
            }
        }
        
        // Parse sequences
        if (j.contains("sequences") && j["sequences"].is_array())
        {
            for (const auto& seqJson : j["sequences"])
            {
                AnimationSequence seq;
                seq.name = JsonHelper::GetString(seqJson, "name", "");
                seq.spritesheetId = JsonHelper::GetString(seqJson, "spritesheetId", "");

                if (seqJson.contains("frames"))
                {
                    seq.startFrame = JsonHelper::GetInt(seqJson["frames"], "start", 0);
                    seq.frameCount = JsonHelper::GetInt(seqJson["frames"], "count", 1);
                }

                seq.frameDuration = JsonHelper::GetFloat(seqJson, "frameDuration", 0.1f);
                seq.loop = JsonHelper::GetBool(seqJson, "loop", true);
                seq.speed = JsonHelper::GetFloat(seqJson, "speed", 1.0f);
                seq.nextAnimation = JsonHelper::GetString(seqJson, "nextAnimation", "");
                
                bank.animations[seq.name] = seq;
            }
        }
        
        m_currentBank = bank;
        m_currentBankPath = filepath;
        m_hasBankLoaded = true;
        m_selectedSpritesheetIndex = -1;
        m_selectedSequenceIndex = -1;
        ClearDirty();
        
        SYSTEM_LOG << "AnimationEditorWindow: Loaded bank from " << filepath << "\n";
    }
    catch (const std::exception& e)
    {
        SYSTEM_LOG << "AnimationEditorWindow: Error parsing JSON: " << e.what() << "\n";
    }
}

void AnimationEditorWindow::ExportBankJSON(const std::string& filepath)
{
    try
    {
        json j = json::object();
        
        j["schema_version"] = 2;
        j["type"] = "AnimationBank";
        j["bankId"] = m_currentBank.bankId;
        j["description"] = m_currentBank.description;
        
        // Metadata
        json meta = json::object();
        meta["author"] = m_currentBank.author;
        meta["created"] = m_currentBank.createdDate;
        
        // Update last modified date
        time_t now = time(nullptr);
        char dateBuf[64];
        struct tm timeInfo;
        gmtime_s(&timeInfo, &now);
        strftime(dateBuf, sizeof(dateBuf), "%Y-%m-%dT%H:%M:%SZ", &timeInfo);
        meta["lastModified"] = dateBuf;
        m_currentBank.lastModifiedDate = dateBuf;
        
        json tagsArray = json::array();
        for (const auto& tag : m_currentBank.tags)
        {
            tagsArray.push_back(tag);
        }
        meta["tags"] = tagsArray;
        
        j["metadata"] = meta;
        
        // Spritesheets
        json sheetsArray = json::array();
        for (const auto& sheet : m_currentBank.spritesheets)
        {
            json sheetJson = json::object();
            sheetJson["id"] = sheet.id;
            sheetJson["path"] = sheet.path;
            sheetJson["description"] = sheet.description;
            sheetJson["frameWidth"] = sheet.frameWidth;
            sheetJson["frameHeight"] = sheet.frameHeight;
            sheetJson["columns"] = sheet.columns;
            sheetJson["rows"] = sheet.rows;
            sheetJson["totalFrames"] = sheet.totalFrames;
            sheetJson["spacing"] = sheet.spacing;
            sheetJson["margin"] = sheet.margin;
            
            json hotspot = json::object();
            hotspot["x"] = sheet.hotspot.x;
            hotspot["y"] = sheet.hotspot.y;
            sheetJson["hotspot"] = hotspot;
            
            sheetsArray.push_back(sheetJson);
        }
        j["spritesheets"] = sheetsArray;
        
        // Sequences
        json seqsArray = json::array();
        for (const auto& pair : m_currentBank.animations)
        {
            const auto& seq = pair.second;
            json seqJson = json::object();
            seqJson["name"] = seq.name;
            seqJson["spritesheetId"] = seq.spritesheetId;
            
            json frames = json::object();
            frames["start"] = seq.startFrame;
            frames["count"] = seq.frameCount;
            seqJson["frames"] = frames;
            
            seqJson["frameDuration"] = seq.frameDuration;
            seqJson["loop"] = seq.loop;
            seqJson["speed"] = seq.speed;
            seqJson["nextAnimation"] = seq.nextAnimation;
            
            seqsArray.push_back(seqJson);
        }
        j["sequences"] = seqsArray;
        
        // Write to file
        JsonHelper::SaveJsonToFile(filepath, j);
        
        SYSTEM_LOG << "AnimationEditorWindow: Saved bank to " << filepath << "\n";
    }
    catch (const std::exception& e)
    {
        SYSTEM_LOG << "AnimationEditorWindow: Error exporting JSON: " << e.what() << "\n";
    }
}

std::vector<std::string> AnimationEditorWindow::ScanBankDirectory(const std::string& dirPath)
{
    std::vector<std::string> files;
    
#ifdef _WIN32
    WIN32_FIND_DATAA findData;
    std::string searchPath = dirPath + "/*.json";
    HANDLE hFind = FindFirstFileA(searchPath.c_str(), &findData);
    
    if (hFind != INVALID_HANDLE_VALUE)
    {
        do
        {
            if (!(findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
            {
                files.push_back(dirPath + "/" + findData.cFileName);
            }
        } while (FindNextFileA(hFind, &findData));
        
        FindClose(hFind);
    }
#else
    DIR* dir = opendir(dirPath.c_str());
    if (dir)
    {
        struct dirent* entry;
        while ((entry = readdir(dir)) != nullptr)
        {
            std::string filename = entry->d_name;
            if (filename.size() > 5 && filename.substr(filename.size() - 5) == ".json")
            {
                files.push_back(dirPath + "/" + filename);
            }
        }
        closedir(dir);
    }
#endif
    
    return files;
}

// ========================================================================
// Spritesheet Operations
// ========================================================================

void AnimationEditorWindow::AddSpritesheet()
{
    SpritesheetInfo sheet;
    sheet.id = "new_spritesheet_" + std::to_string(m_currentBank.spritesheets.size());
    sheet.path = "";
    sheet.frameWidth = 32;
    sheet.frameHeight = 32;
    sheet.columns = 1;
    sheet.rows = 1;
    sheet.totalFrames = 1;
    sheet.spacing = 0;
    sheet.margin = 0;
    sheet.hotspot.x = 16.0f;
    sheet.hotspot.y = 16.0f;
    
    m_currentBank.spritesheets.push_back(sheet);
    m_selectedSpritesheetIndex = static_cast<int>(m_currentBank.spritesheets.size()) - 1;
    
    MarkDirty();
    
    SYSTEM_LOG << "AnimationEditorWindow: Added spritesheet\n";
}

void AnimationEditorWindow::RemoveSpritesheet(int index)
{
    if (index < 0 || index >= static_cast<int>(m_currentBank.spritesheets.size()))
        return;
    
    m_currentBank.spritesheets.erase(m_currentBank.spritesheets.begin() + index);
    m_selectedSpritesheetIndex = -1;
    
    MarkDirty();
    
    SYSTEM_LOG << "AnimationEditorWindow: Removed spritesheet\n";
}

void AnimationEditorWindow::AutoDetectGrid(SpritesheetInfo& sheet)
{
    // Load texture to get dimensions
    SDL_Texture* tex = LoadSpritesheetTexture(sheet.path);
    if (!tex)
    {
        SYSTEM_LOG << "AnimationEditorWindow: Cannot auto-detect grid - texture not loaded\n";
        return;
    }
    
    float texWf = 0.f;
    float texHf = 0.f;
    SDL_GetTextureSize(tex, &texWf, &texHf);
    int texW = static_cast<int>(texWf);
    int texH = static_cast<int>(texHf);

    // Try to calculate columns/rows based on frame size
    if (sheet.frameWidth > 0 && sheet.frameHeight > 0)
    {
        sheet.columns = (texW - 2 * sheet.margin + sheet.spacing) / (sheet.frameWidth + sheet.spacing);
        sheet.rows = (texH - 2 * sheet.margin + sheet.spacing) / (sheet.frameHeight + sheet.spacing);
        sheet.totalFrames = sheet.columns * sheet.rows;
        
        MarkDirty();
        
        SYSTEM_LOG << "AnimationEditorWindow: Auto-detected grid: " << sheet.columns << "x" << sheet.rows << " = " << sheet.totalFrames << " frames\n";
    }
}

SDL_Texture* AnimationEditorWindow::LoadSpritesheetTexture(const std::string& path)
{
    if (path.empty())
        return nullptr;
    
    // Use DataManager to load texture
    auto* sprite = DataManager::Get().GetSprite(path, path);
    return sprite ? sprite : nullptr;
}

// ========================================================================
// Sequence Operations
// ========================================================================

void AnimationEditorWindow::AddSequence()
{
    AnimationSequence seq;
    seq.name = "new_sequence_" + std::to_string(m_currentBank.animations.size());
    seq.spritesheetId = m_currentBank.spritesheets.empty() ? "" : m_currentBank.spritesheets[0].id;
    seq.startFrame = 0;
    seq.frameCount = 1;
    seq.frameDuration = 0.1f;
    seq.loop = true;
    seq.speed = 1.0f;
    seq.nextAnimation = "";
    
    m_currentBank.animations[seq.name] = seq;
    m_selectedSequenceIndex = static_cast<int>(m_currentBank.animations.size()) - 1;
    
    MarkDirty();
    
    SYSTEM_LOG << "AnimationEditorWindow: Added sequence\n";
}

void AnimationEditorWindow::RemoveSequence(int index)
{
    if (index < 0 || index >= static_cast<int>(m_currentBank.animations.size()))
        return;
    
    auto it = m_currentBank.animations.begin();
    std::advance(it, index);
    m_currentBank.animations.erase(it);
    m_selectedSequenceIndex = -1;
    
    MarkDirty();
    
    SYSTEM_LOG << "AnimationEditorWindow: Removed sequence\n";
}

// ========================================================================
// Preview Operations
// ========================================================================

void AnimationEditorWindow::StartPreview()
{
    if (m_selectedSequenceIndex < 0 || m_selectedSequenceIndex >= static_cast<int>(m_currentBank.animations.size()))
        return;
    
    auto it = m_currentBank.animations.begin();
    std::advance(it, m_selectedSequenceIndex);
    const AnimationSequence& seq = it->second;
    
    if (m_previewCurrentFrame < seq.startFrame || m_previewCurrentFrame > seq.startFrame + seq.frameCount - 1)
    {
        m_previewCurrentFrame = seq.startFrame;
    }
    
    m_isPreviewPlaying = true;
    m_isPreviewPaused = false;
    m_previewFrameTimer = 0.0f;
}

void AnimationEditorWindow::StopPreview()
{
    m_isPreviewPlaying = false;
    m_isPreviewPaused = false;
    ResetPreview();
}

void AnimationEditorWindow::PausePreview()
{
    m_isPreviewPaused = !m_isPreviewPaused;
}

void AnimationEditorWindow::ResetPreview()
{
    if (m_selectedSequenceIndex < 0 || m_selectedSequenceIndex >= static_cast<int>(m_currentBank.animations.size()))
    {
        m_previewCurrentFrame = 0;
        return;
    }
    
    auto it = m_currentBank.animations.begin();
    std::advance(it, m_selectedSequenceIndex);
    const AnimationSequence& seq = it->second;
    
    m_previewCurrentFrame = seq.startFrame;
    m_previewFrameTimer = 0.0f;
}

// ========================================================================
// Helper Methods
// ========================================================================

void AnimationEditorWindow::MarkDirty()
{
    m_isDirty = true;
}

void AnimationEditorWindow::ClearDirty()
{
    m_isDirty = false;
}

bool AnimationEditorWindow::PromptUnsavedChanges()
{
    // For now, just return true (user accepts closing)
    // In a full implementation, this would show a dialog
    SYSTEM_LOG << "AnimationEditorWindow: Unsaved changes detected\n";
    return true;
}

void AnimationEditorWindow::UpdateWindowTitle()
{
    // Title is set in Render() with m_isDirty flag
}

// ========================================================================
// Standalone Window Management
// ========================================================================

void AnimationEditorWindow::CreateSeparateWindow()
{
    if (m_separateWindow) 
    {
        SYSTEM_LOG << "[AnimationEditor] Separate window already exists\n";
        return; // Already created
    }
    
    // Save current context
    ImGuiContext* mainContext = ImGui::GetCurrentContext();
    
    // Create SDL window (1280x720, resizable)
    if (!SDL_CreateWindowAndRenderer(
        "Animation Editor - Olympe Engine",
        1280,
        720,
        SDL_WINDOW_RESIZABLE,
        &m_separateWindow,
        &m_separateRenderer))
    {
        SYSTEM_LOG << "[AnimationEditor] Failed to create window: " << SDL_GetError() << "\n";
        return;
    }
    
    // Create separate ImGui context
    m_separateImGuiContext = ImGui::CreateContext();
    ImGui::SetCurrentContext(m_separateImGuiContext);
    
    // Setup ImGui style
    ImGui::StyleColorsDark();
    
    // Initialize ImGui backends
    ImGui_ImplSDL3_InitForSDLRenderer(m_separateWindow, m_separateRenderer);
    ImGui_ImplSDLRenderer3_Init(m_separateRenderer);
    
    // Restore main context
    ImGui::SetCurrentContext(mainContext);
    
    SYSTEM_LOG << "[AnimationEditor] Standalone window created\n";
}

void AnimationEditorWindow::DestroySeparateWindow()
{
    if (!m_separateWindow)
        return;
    
    // Save current context
    ImGuiContext* mainContext = ImGui::GetCurrentContext();
    
    if (m_separateImGuiContext)
    {
        ImGui::SetCurrentContext(m_separateImGuiContext);
        ImGui_ImplSDLRenderer3_Shutdown();
        ImGui_ImplSDL3_Shutdown();
        ImGui::DestroyContext(m_separateImGuiContext);
        m_separateImGuiContext = nullptr;
    }
    
    // Restore main context if needed
    if (mainContext != m_separateImGuiContext && mainContext != nullptr)
    {
        ImGui::SetCurrentContext(mainContext);
    }
    
    if (m_separateRenderer)
    {
        SDL_DestroyRenderer(m_separateRenderer);
        m_separateRenderer = nullptr;
    }
    
    if (m_separateWindow)
    {
        SDL_DestroyWindow(m_separateWindow);
        m_separateWindow = nullptr;
    }
    
    SYSTEM_LOG << "[AnimationEditor] Separate window destroyed\n";
}

void AnimationEditorWindow::RenderSeparateWindow()
{
    if (!m_separateWindow || !m_separateRenderer) 
        return;
    
    // Switch to separate ImGui context
    ImGuiContext* mainContext = ImGui::GetCurrentContext();
    ImGui::SetCurrentContext(m_separateImGuiContext);
    
    // Clear window
    SDL_SetRenderDrawColor(m_separateRenderer, 45, 45, 48, 255);
    SDL_RenderClear(m_separateRenderer);
    
    // ImGui frame
    ImGui_ImplSDLRenderer3_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();
    
    // Render Animation Editor UI
    Render();
    
    // Present
    ImGui::Render();
    ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), m_separateRenderer);
    SDL_RenderPresent(m_separateRenderer);
    
    // Restore main context
    ImGui::SetCurrentContext(mainContext);
}

void AnimationEditorWindow::ProcessEvent(SDL_Event* event)
{
    if (!m_separateWindow || !m_isOpen) 
        return;
    
    // Check if event is for our window
    if (event->type == SDL_EVENT_WINDOW_CLOSE_REQUESTED)
    {
        Uint32 windowID = SDL_GetWindowID(m_separateWindow);
        if (event->window.windowID == windowID)
        {
            Toggle(); // Close window
            return;
        }
    }
    
    // Forward event to ImGui (separate context)
    ImGuiContext* mainContext = ImGui::GetCurrentContext();
    ImGui::SetCurrentContext(m_separateImGuiContext);
    ImGui_ImplSDL3_ProcessEvent(event);
    ImGui::SetCurrentContext(mainContext);
}

void AnimationEditorWindow::Update(float deltaTime)
{
    if (!m_isOpen) 
        return;
    
    // Update preview animation
    UpdatePreview(deltaTime);
    
    // Render separate window
    RenderSeparateWindow();
}

} // namespace Olympe
