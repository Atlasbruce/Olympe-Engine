/*
 * Olympe Blueprint Editor - Template Manager Implementation
 */

#include "TemplateManager.h"
#include <fstream>
#include <sstream>
#include <iomanip>
#include <iostream>
#include <random>
#include <algorithm>

#ifdef _WIN32
#include <direct.h>
#include <windows.h>
#include <io.h>
#else
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <unistd.h>
#endif

namespace Olympe
{
    // ========================================================================
    // BlueprintTemplate Implementation
    // ========================================================================

    BlueprintTemplate::BlueprintTemplate()
        : createdDate(0)
        , modifiedDate(0)
    {
    }

    json BlueprintTemplate::ToJson() const
    {
        json j;
        j["id"] = id;
        j["name"] = name;
        j["description"] = description;
        j["category"] = category;
        j["author"] = author;
        j["version"] = version;
        j["thumbnailPath"] = thumbnailPath;
        j["createdDate"] = static_cast<double>(createdDate);
        j["modifiedDate"] = static_cast<double>(modifiedDate);
        j["blueprintData"] = blueprintData;
        
        return j;
    }

    BlueprintTemplate BlueprintTemplate::FromJson(const json& j)
    {
        BlueprintTemplate tpl;
        
        if (j.contains("id")) tpl.id = j["id"].get<std::string>();
        if (j.contains("name")) tpl.name = j["name"].get<std::string>();
        if (j.contains("description")) tpl.description = j["description"].get<std::string>();
        if (j.contains("category")) tpl.category = j["category"].get<std::string>();
        if (j.contains("author")) tpl.author = j["author"].get<std::string>();
        if (j.contains("version")) tpl.version = j["version"].get<std::string>();
        if (j.contains("thumbnailPath")) tpl.thumbnailPath = j["thumbnailPath"].get<std::string>();
        if (j.contains("createdDate")) tpl.createdDate = static_cast<time_t>(j["createdDate"].get<double>());
        if (j.contains("modifiedDate")) tpl.modifiedDate = static_cast<time_t>(j["modifiedDate"].get<double>());
        if (j.contains("blueprintData")) tpl.blueprintData = j["blueprintData"];
        
        return tpl;
    }

    bool BlueprintTemplate::SaveToFile(const std::string& filepath) const
    {
        try
        {
            std::ofstream file(filepath);
            if (!file.is_open())
            {
                std::cerr << "Failed to open file for writing: " << filepath << std::endl;
                return false;
            }

            json j = ToJson();
            file << j.dump(4); // Pretty print with 4-space indent
            file.close();
            
            return true;
        }
        catch (const std::exception& e)
        {
            std::cerr << "Error saving template to file: " << e.what() << std::endl;
            return false;
        }
    }

    BlueprintTemplate BlueprintTemplate::LoadFromFile(const std::string& filepath)
    {
        try
        {
            std::ifstream file(filepath);
            if (!file.is_open())
            {
                std::cerr << "Failed to open template file: " << filepath << std::endl;
                return BlueprintTemplate();
            }

            json j;
            std::stringstream buffer;
            buffer << file.rdbuf();
            j = json::parse(buffer.str());

            return FromJson(j);
        }
        catch (const std::exception& e)
        {
            std::cerr << "Error loading template from file: " << e.what() << std::endl;
            return BlueprintTemplate();
        }
    }

    // ========================================================================
    // TemplateManager Implementation
    // ========================================================================

    TemplateManager& TemplateManager::Instance()
    {
        static TemplateManager instance;
        return instance;
    }

    TemplateManager::TemplateManager()
        : m_Initialized(false)
    {
    }

    TemplateManager::~TemplateManager()
    {
    }

    void TemplateManager::Initialize(const std::string& templatesPath)
    {
        m_TemplatesPath = templatesPath;
        m_Templates.clear();
        m_LastError.clear();

        // Ensure templates directory exists
        if (!EnsureDirectoryExists(m_TemplatesPath))
        {
            m_LastError = "Failed to create templates directory: " + m_TemplatesPath;
            std::cerr << m_LastError << std::endl;
        }

        // Load all templates from directory
        LoadTemplates(m_TemplatesPath);

        m_Initialized = true;
    }

    void TemplateManager::Shutdown()
    {
        m_Templates.clear();
        m_Initialized = false;
    }

    bool TemplateManager::LoadTemplates(const std::string& templatesPath)
    {
        m_TemplatesPath = templatesPath;
        m_Templates.clear();
        m_LastError.clear();

        bool exists = false;
#ifdef _WIN32
        DWORD attribs = GetFileAttributesA(templatesPath.c_str());
        exists = (attribs != INVALID_FILE_ATTRIBUTES);
#else
        struct stat st;
        exists = (stat(templatesPath.c_str(), &st) == 0);
#endif

        if (!exists)
        {
            m_LastError = "Templates directory does not exist: " + templatesPath;
            std::cerr << m_LastError << std::endl;
            return false;
        }

        ScanTemplateDirectory();
        return true;
    }

    bool TemplateManager::SaveTemplate(const BlueprintTemplate& tpl)
    {
        if (tpl.id.empty())
        {
            m_LastError = "Template ID is empty";
            return false;
        }

        // Construct filename from template ID
        std::string filename = tpl.id + ".json";
        std::string filepath = m_TemplatesPath + "/" + filename;

        // Save to file
        if (!tpl.SaveToFile(filepath))
        {
            m_LastError = "Failed to save template to file: " + filepath;
            return false;
        }

        // Update in-memory catalog
        // Check if template already exists
        bool found = false;
        for (auto& existingTpl : m_Templates)
        {
            if (existingTpl.id == tpl.id)
            {
                existingTpl = tpl;
                found = true;
                break;
            }
        }

        // If not found, add to catalog
        if (!found)
        {
            m_Templates.push_back(tpl);
        }

        return true;
    }

    bool TemplateManager::DeleteTemplate(const std::string& templateId)
    {
        if (templateId.empty())
        {
            m_LastError = "Template ID is empty";
            return false;
        }

        // Find template in catalog
        auto it = std::find_if(m_Templates.begin(), m_Templates.end(),
            [&templateId](const BlueprintTemplate& tpl) {
                return tpl.id == templateId;
            });

        if (it == m_Templates.end())
        {
            m_LastError = "Template not found: " + templateId;
            return false;
        }

        // Delete file
        std::string filename = templateId + ".json";
        std::string filepath = m_TemplatesPath + "/" + filename;

        try
        {
            bool fileExists = false;
#ifdef _WIN32
            DWORD attribs = GetFileAttributesA(filepath.c_str());
            fileExists = (attribs != INVALID_FILE_ATTRIBUTES && !(attribs & FILE_ATTRIBUTE_DIRECTORY));
#else
            struct stat st;
            fileExists = (stat(filepath.c_str(), &st) == 0 && S_ISREG(st.st_mode));
#endif
            if (fileExists)
            {
                std::remove(filepath.c_str());
            }
        }
        catch (const std::exception& e)
        {
            m_LastError = "Failed to delete template file: " + std::string(e.what());
            std::cerr << m_LastError << std::endl;
            return false;
        }

        // Remove from catalog
        m_Templates.erase(it);

        return true;
    }

    bool TemplateManager::RefreshTemplates()
    {
        return LoadTemplates(m_TemplatesPath);
    }

    const BlueprintTemplate* TemplateManager::FindTemplate(const std::string& id) const
    {
        for (const auto& tpl : m_Templates)
        {
            if (tpl.id == id)
            {
                return &tpl;
            }
        }
        return nullptr;
    }

    std::vector<BlueprintTemplate> TemplateManager::GetTemplatesByCategory(const std::string& category) const
    {
        std::vector<BlueprintTemplate> result;
        
        for (const auto& tpl : m_Templates)
        {
            if (tpl.category == category)
            {
                result.push_back(tpl);
            }
        }
        
        return result;
    }

    std::vector<std::string> TemplateManager::GetAllCategories() const
    {
        std::vector<std::string> categories;
        
        for (const auto& tpl : m_Templates)
        {
            // Check if category already exists in list
            if (std::find(categories.begin(), categories.end(), tpl.category) == categories.end())
            {
                categories.push_back(tpl.category);
            }
        }
        
        return categories;
    }

    bool TemplateManager::ApplyTemplateToBlueprint(const std::string& templateId, json& targetBlueprint)
    {
        const BlueprintTemplate* tpl = FindTemplate(templateId);
        if (!tpl)
        {
            m_LastError = "Template not found: " + templateId;
            return false;
        }

        // Copy blueprint data from template
        targetBlueprint = tpl->blueprintData;
        
        return true;
    }

    BlueprintTemplate TemplateManager::CreateTemplateFromBlueprint(
        const json& blueprint,
        const std::string& name,
        const std::string& description,
        const std::string& category,
        const std::string& author)
    {
        BlueprintTemplate tpl;
        
        tpl.id = GenerateUUID();
        tpl.name = name;
        tpl.description = description;
        tpl.category = category;
        tpl.author = author;
        tpl.version = "1.0";
        tpl.blueprintData = blueprint;
        tpl.thumbnailPath = "";
        tpl.createdDate = std::time(nullptr);
        tpl.modifiedDate = tpl.createdDate;
        
        return tpl;
    }

    std::string TemplateManager::GenerateUUID() const
    {
        // Simple UUID generation using random hex values
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(0, 15);

        const char* hexChars = "0123456789abcdef";
        std::string uuid;
        
        for (int i = 0; i < 8; i++) uuid += hexChars[dis(gen)];
        uuid += "-";
        for (int i = 0; i < 4; i++) uuid += hexChars[dis(gen)];
        uuid += "-";
        for (int i = 0; i < 4; i++) uuid += hexChars[dis(gen)];
        uuid += "-";
        for (int i = 0; i < 4; i++) uuid += hexChars[dis(gen)];
        uuid += "-";
        for (int i = 0; i < 12; i++) uuid += hexChars[dis(gen)];
        
        return uuid;
    }

    void TemplateManager::ScanTemplateDirectory()
    {
        bool dirExists = false;
        bool isDir = false;
#ifdef _WIN32
        DWORD attribs = GetFileAttributesA(m_TemplatesPath.c_str());
        if (attribs != INVALID_FILE_ATTRIBUTES)
        {
            dirExists = true;
            isDir = (attribs & FILE_ATTRIBUTE_DIRECTORY);
        }
#else
        struct stat st;
        if (stat(m_TemplatesPath.c_str(), &st) == 0)
        {
            dirExists = true;
            isDir = S_ISDIR(st.st_mode);
        }
#endif

        if (!dirExists || !isDir)
        {
            return;
        }

        try
        {
#ifdef _WIN32
            WIN32_FIND_DATAA findData;
            std::string searchPath = m_TemplatesPath + "\\*.json";
            HANDLE hFind = FindFirstFileA(searchPath.c_str(), &findData);
            if (hFind != INVALID_HANDLE_VALUE)
            {
                do
                {
                    std::string filename = findData.cFileName;
                    if (filename != "." && filename != "..")
                    {
                        std::string fullPath = m_TemplatesPath + "/" + filename;
                        BlueprintTemplate tpl = BlueprintTemplate::LoadFromFile(fullPath);
                        if (!tpl.id.empty())
                        {
                            m_Templates.push_back(tpl);
                        }
                    }
                } while (FindNextFileA(hFind, &findData) != 0);
                FindClose(hFind);
            }
#else
            DIR* dir = opendir(m_TemplatesPath.c_str());
            if (dir)
            {
                struct dirent* entry;
                while ((entry = readdir(dir)) != nullptr)
                {
                    std::string filename = entry->d_name;
                    if (filename != "." && filename != "..")
                    {
                        if (filename.length() > 5 && filename.substr(filename.length() - 5) == ".json")
                        {
                            std::string fullPath = m_TemplatesPath + "/" + filename;
                            BlueprintTemplate tpl = BlueprintTemplate::LoadFromFile(fullPath);
                            if (!tpl.id.empty())
                            {
                                m_Templates.push_back(tpl);
                            }
                        }
                    }
                }
                closedir(dir);
            }
#endif
        }
        catch (const std::exception& e)
        {
            m_LastError = "Error scanning template directory: " + std::string(e.what());
            std::cerr << m_LastError << std::endl;
        }
    }

    bool TemplateManager::EnsureDirectoryExists(const std::string& path)
    {
        if (path.empty()) return false;

        std::string normalizedPath = path;
        std::replace(normalizedPath.begin(), normalizedPath.end(), '\\', '/');
        if (!normalizedPath.empty() && normalizedPath.back() == '/') normalizedPath.pop_back();
        if (normalizedPath.empty()) return true;

        std::string accum;
        size_t pos = 0;
        if (!normalizedPath.empty() && normalizedPath[0] == '/') { accum = "/"; pos = 1; }

        while (true)
        {
            size_t next = normalizedPath.find('/', pos);
            std::string part = (next == std::string::npos) ? normalizedPath.substr(pos) : normalizedPath.substr(pos, next - pos);
            if (!accum.empty() && accum.back() != '/') accum += '/';
            accum += part;

#ifdef _WIN32
            int r = _mkdir(accum.c_str());
#else
            int r = mkdir(accum.c_str(), 0755);
#endif
            if (r != 0)
            {
                if (errno != EEXIST)
                {
                    bool exists = false;
#ifdef _WIN32
                    DWORD attribs = GetFileAttributesA(accum.c_str());
                    exists = (attribs != INVALID_FILE_ATTRIBUTES && (attribs & FILE_ATTRIBUTE_DIRECTORY));
#else
                    struct stat st;
                    exists = (stat(accum.c_str(), &st) == 0 && S_ISDIR(st.st_mode));
#endif
                    if (!exists)
                    {
                        std::cerr << "Failed to create directory part: " << accum << std::endl;
                        return false;
                    }
                }
            }

            if (next == std::string::npos) break;
            pos = next + 1;
        }

        return true;
    }
}
