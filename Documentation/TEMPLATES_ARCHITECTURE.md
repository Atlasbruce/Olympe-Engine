# Extending the Template System - Developer Guide

## Overview

This guide explains how to extend and customize the Blueprint Template system for advanced use cases, custom workflows, and integration with external tools.

## Architecture Overview

### Components

```
TemplateManager (Backend)
    ├── Template Storage (JSON files)
    ├── Template Catalog (in-memory)
    ├── CRUD Operations
    └── Category Management

TemplateBrowserPanel (Frontend)
    ├── Template List View
    ├── Search & Filtering
    ├── Context Menus
    └── "Save as Template" Modal
```

### Data Flow

```
User Action (GUI)
    ↓
TemplateBrowserPanel
    ↓
BlueprintEditor API
    ↓
TemplateManager
    ↓
File System (JSON)
```

## Core Classes

### BlueprintTemplate Structure

**Location:** `Source/BlueprintEditor/TemplateManager.h`

```cpp
struct BlueprintTemplate {
    std::string id;              // UUID unique identifier
    std::string name;            // Template display name
    std::string description;     // User description
    std::string category;        // Category (AI, Character, etc.)
    std::string author;          // Creator name
    std::string version;         // Template version
    json blueprintData;          // Complete blueprint JSON data
    std::string thumbnailPath;   // Optional preview image
    time_t createdDate;          // Creation timestamp
    time_t modifiedDate;         // Last modification
    
    // Serialization
    json ToJson() const;
    static BlueprintTemplate FromJson(const json& j);
    
    // File I/O
    bool SaveToFile(const std::string& filepath) const;
    static BlueprintTemplate LoadFromFile(const std::string& filepath);
};
```

### TemplateManager API

**Location:** `Source/BlueprintEditor/TemplateManager.h`

```cpp
class TemplateManager {
public:
    static TemplateManager& Get();  // Singleton access
    
    // Lifecycle
    void Initialize(const std::string& templatesPath = "Blueprints/Templates");
    void Shutdown();
    
    // Template management
    bool SaveTemplate(const BlueprintTemplate& tpl);
    bool DeleteTemplate(const std::string& templateId);
    bool RefreshTemplates();
    
    // Queries
    const std::vector<BlueprintTemplate>& GetAllTemplates() const;
    const BlueprintTemplate* FindTemplate(const std::string& id) const;
    std::vector<BlueprintTemplate> GetTemplatesByCategory(const std::string& category) const;
    std::vector<std::string> GetAllCategories() const;
    
    // Application
    bool ApplyTemplateToBlueprint(const std::string& templateId, json& targetBlueprint);
    
    // Creation
    BlueprintTemplate CreateTemplateFromBlueprint(
        const json& blueprint,
        const std::string& name,
        const std::string& description,
        const std::string& category,
        const std::string& author = "User"
    );
};
```

## Extending Templates

### Adding Custom Metadata

**Step 1:** Extend BlueprintTemplate structure

```cpp
struct BlueprintTemplate {
    // ... existing fields ...
    
    // Custom fields
    std::string tags;              // Comma-separated tags
    int usageCount;                // Track usage
    std::string projectVersion;    // Project compatibility
    std::vector<std::string> dependencies;  // Required assets
};
```

**Step 2:** Update serialization

```cpp
json BlueprintTemplate::ToJson() const {
    json j;
    // ... existing fields ...
    j["tags"] = tags;
    j["usageCount"] = usageCount;
    j["projectVersion"] = projectVersion;
    j["dependencies"] = dependencies;
    return j;
}
```

**Step 3:** Update deserialization

```cpp
BlueprintTemplate BlueprintTemplate::FromJson(const json& j) {
    BlueprintTemplate tpl;
    // ... existing fields ...
    if (j.contains("tags")) tpl.tags = j["tags"];
    if (j.contains("usageCount")) tpl.usageCount = j["usageCount"];
    // ... etc
    return tpl;
}
```

### Custom Template Validation

Add validation before saving templates:

```cpp
class TemplateValidator {
public:
    struct ValidationResult {
        bool isValid;
        std::vector<std::string> errors;
        std::vector<std::string> warnings;
    };
    
    static ValidationResult Validate(const BlueprintTemplate& tpl) {
        ValidationResult result;
        result.isValid = true;
        
        // Name validation
        if (tpl.name.empty()) {
            result.errors.push_back("Template name cannot be empty");
            result.isValid = false;
        }
        
        // Blueprint data validation
        if (!tpl.blueprintData.contains("components")) {
            result.errors.push_back("Blueprint must have components");
            result.isValid = false;
        }
        
        // Warnings
        if (tpl.description.empty()) {
            result.warnings.push_back("Consider adding a description");
        }
        
        return result;
    }
};

// Usage in TemplateManager
bool TemplateManager::SaveTemplate(const BlueprintTemplate& tpl) {
    auto validation = TemplateValidator::Validate(tpl);
    if (!validation.isValid) {
        m_LastError = "Validation failed: " + validation.errors[0];
        return false;
    }
    // ... rest of save logic
}
```

### Template Inheritance

Implement template hierarchies:

```cpp
struct BlueprintTemplate {
    std::string parentTemplateId;  // Optional parent template
    
    // Apply template with inheritance
    json GetFinalBlueprint() const {
        json result = blueprintData;
        
        if (!parentTemplateId.empty()) {
            auto* parent = TemplateManager::Get().FindTemplate(parentTemplateId);
            if (parent) {
                // Merge parent data with child
                result = MergeTemplates(parent->blueprintData, blueprintData);
            }
        }
        
        return result;
    }
    
private:
    static json MergeTemplates(const json& parent, const json& child) {
        json merged = parent;
        // Merge logic: child overrides parent
        for (auto& el : child.items()) {
            merged[el.key()] = el.value();
        }
        return merged;
    }
};
```

## Custom Template Categories

### Hierarchical Categories

```cpp
class CategoryManager {
public:
    struct Category {
        std::string name;
        std::string displayName;
        std::string icon;
        std::vector<Category> subcategories;
    };
    
    static std::vector<Category> GetCategoryTree() {
        return {
            {"AI", "AI & Behavior", "🤖", {
                {"AI/Enemy", "Enemy AI", "👹", {}},
                {"AI/NPC", "NPC AI", "🧑", {}},
                {"AI/Player", "Player AI", "🎮", {}}
            }},
            {"Character", "Characters", "👤", {
                {"Character/Player", "Player Characters", "⭐", {}},
                {"Character/NPC", "NPCs", "👥", {}}
            }},
            // ... more categories
        };
    }
};
```

### Category-Based Search

```cpp
std::vector<BlueprintTemplate> SearchTemplates(
    const std::string& query,
    const std::string& categoryPath) 
{
    auto& allTemplates = TemplateManager::Get().GetAllTemplates();
    std::vector<BlueprintTemplate> results;
    
    for (const auto& tpl : allTemplates) {
        // Match category (including subcategories)
        if (!categoryPath.empty() && 
            !tpl.category.starts_with(categoryPath)) {
            continue;
        }
        
        // Match search query
        if (!query.empty()) {
            bool matches = 
                tpl.name.find(query) != std::string::npos ||
                tpl.description.find(query) != std::string::npos ||
                tpl.tags.find(query) != std::string::npos;
            
            if (!matches) continue;
        }
        
        results.push_back(tpl);
    }
    
    return results;
}
```

## Template Thumbnails

### Generating Thumbnails

```cpp
class TemplateThumbnailGenerator {
public:
    static std::string GenerateThumbnail(const BlueprintTemplate& tpl) {
        // 1. Render blueprint to image
        auto image = RenderBlueprintToImage(tpl.blueprintData);
        
        // 2. Save as PNG
        std::string thumbnailPath = "Templates/Thumbnails/" + tpl.id + ".png";
        SaveImageAsPNG(image, thumbnailPath);
        
        return thumbnailPath;
    }
    
private:
    static Image RenderBlueprintToImage(const json& blueprint) {
        // Create image canvas
        Image img(256, 256);
        
        // Draw component icons
        if (blueprint.contains("components")) {
            int y = 10;
            for (const auto& comp : blueprint["components"]) {
                DrawComponentIcon(img, comp["type"], 10, y);
                y += 30;
            }
        }
        
        return img;
    }
};
```

### Loading Thumbnails in UI

```cpp
void TemplateBrowserPanel::RenderTemplateBrowser() {
    for (const auto& tpl : templates) {
        ImGui::PushID(tpl.id.c_str());
        
        // Load and display thumbnail
        if (!tpl.thumbnailPath.empty()) {
            ImTextureID texture = LoadTexture(tpl.thumbnailPath);
            ImGui::Image(texture, ImVec2(64, 64));
            ImGui::SameLine();
        }
        
        ImGui::Selectable(tpl.name.c_str());
        ImGui::PopID();
    }
}
```

## Integration with External Tools

### Export Templates

```cpp
class TemplateExporter {
public:
    // Export single template
    static bool ExportTemplate(const std::string& templateId,
                              const std::string& exportPath) {
        auto* tpl = TemplateManager::Get().FindTemplate(templateId);
        if (!tpl) return false;
        
        return tpl->SaveToFile(exportPath);
    }
    
    // Export all templates in category
    static bool ExportCategory(const std::string& category,
                              const std::string& exportDir) {
        auto templates = TemplateManager::Get().GetTemplatesByCategory(category);
        
        for (const auto& tpl : templates) {
            std::string path = exportDir + "/" + tpl.id + ".json";
            if (!tpl.SaveToFile(path)) {
                return false;
            }
        }
        
        return true;
    }
    
    // Export as template pack (ZIP)
    static bool ExportTemplatePack(const std::vector<std::string>& templateIds,
                                   const std::string& packPath) {
        // Create ZIP archive
        ZipArchive zip(packPath);
        
        // Add templates
        for (const auto& id : templateIds) {
            auto* tpl = TemplateManager::Get().FindTemplate(id);
            if (tpl) {
                json j = tpl->ToJson();
                zip.AddFile(id + ".json", j.dump(4));
            }
        }
        
        // Add metadata
        json packMeta;
        packMeta["name"] = "Template Pack";
        packMeta["templates"] = templateIds;
        zip.AddFile("pack.json", packMeta.dump(4));
        
        return zip.Close();
    }
};
```

### Import Templates

```cpp
class TemplateImporter {
public:
    // Import single template
    static bool ImportTemplate(const std::string& filepath) {
        auto tpl = BlueprintTemplate::LoadFromFile(filepath);
        if (tpl.id.empty()) return false;
        
        // Check for conflicts
        if (TemplateManager::Get().FindTemplate(tpl.id)) {
            // Handle conflict (prompt user, auto-rename, etc.)
            tpl.id = GenerateNewUUID();
        }
        
        return TemplateManager::Get().SaveTemplate(tpl);
    }
    
    // Import template pack
    static bool ImportTemplatePack(const std::string& packPath) {
        ZipArchive zip(packPath);
        if (!zip.Open()) return false;
        
        // Read pack metadata
        auto packMeta = json::parse(zip.ReadFile("pack.json"));
        
        // Import each template
        for (const auto& filename : zip.GetFileList()) {
            if (filename.ends_with(".json") && filename != "pack.json") {
                auto tplJson = json::parse(zip.ReadFile(filename));
                auto tpl = BlueprintTemplate::FromJson(tplJson);
                TemplateManager::Get().SaveTemplate(tpl);
            }
        }
        
        return true;
    }
};
```

## Template Versioning

### Version Compatibility

```cpp
struct TemplateVersion {
    int major;
    int minor;
    int patch;
    
    static TemplateVersion Parse(const std::string& versionStr) {
        // Parse "1.2.3" format
        TemplateVersion v;
        sscanf(versionStr.c_str(), "%d.%d.%d", &v.major, &v.minor, &v.patch);
        return v;
    }
    
    bool IsCompatibleWith(const TemplateVersion& other) const {
        // Same major version = compatible
        return major == other.major;
    }
};

class TemplateVersionManager {
public:
    static bool CanApplyTemplate(const BlueprintTemplate& tpl) {
        auto tplVersion = TemplateVersion::Parse(tpl.version);
        auto engineVersion = GetCurrentEngineVersion();
        
        if (!tplVersion.IsCompatibleWith(engineVersion)) {
            ShowWarning("Template version mismatch. May not work correctly.");
            return false;
        }
        
        return true;
    }
    
    static void MigrateTemplate(BlueprintTemplate& tpl, 
                               const TemplateVersion& targetVersion) {
        auto currentVersion = TemplateVersion::Parse(tpl.version);
        
        // Apply migrations
        if (currentVersion.major < targetVersion.major) {
            ApplyMajorVersionMigration(tpl);
        }
        
        tpl.version = targetVersion.ToString();
    }
};
```

## Advanced Filtering

### Tag-Based Filtering

```cpp
class TemplateFilter {
public:
    struct FilterCriteria {
        std::string nameQuery;
        std::vector<std::string> tags;
        std::vector<std::string> categories;
        std::string author;
        time_t createdAfter;
        time_t createdBefore;
    };
    
    static std::vector<BlueprintTemplate> Filter(
        const std::vector<BlueprintTemplate>& templates,
        const FilterCriteria& criteria) 
    {
        std::vector<BlueprintTemplate> results;
        
        for (const auto& tpl : templates) {
            if (!MatchesCriteria(tpl, criteria)) {
                continue;
            }
            results.push_back(tpl);
        }
        
        return results;
    }
    
private:
    static bool MatchesCriteria(const BlueprintTemplate& tpl,
                                const FilterCriteria& criteria) {
        // Name match
        if (!criteria.nameQuery.empty() &&
            tpl.name.find(criteria.nameQuery) == std::string::npos) {
            return false;
        }
        
        // Tag match (any tag matches)
        if (!criteria.tags.empty()) {
            bool hasMatchingTag = false;
            for (const auto& tag : criteria.tags) {
                if (tpl.tags.find(tag) != std::string::npos) {
                    hasMatchingTag = true;
                    break;
                }
            }
            if (!hasMatchingTag) return false;
        }
        
        // Category match
        if (!criteria.categories.empty()) {
            bool inCategory = false;
            for (const auto& cat : criteria.categories) {
                if (tpl.category == cat) {
                    inCategory = true;
                    break;
                }
            }
            if (!inCategory) return false;
        }
        
        // Date range
        if (criteria.createdAfter > 0 && tpl.createdDate < criteria.createdAfter) {
            return false;
        }
        if (criteria.createdBefore > 0 && tpl.createdDate > criteria.createdBefore) {
            return false;
        }
        
        return true;
    }
};
```

## Template Analytics

### Usage Tracking

```cpp
class TemplateAnalytics {
public:
    struct TemplateStats {
        std::string templateId;
        int timesApplied;
        int timesViewed;
        time_t lastUsed;
        std::vector<std::string> projectsUsedIn;
    };
    
    static void RecordTemplateApplied(const std::string& templateId) {
        auto& stats = GetStats(templateId);
        stats.timesApplied++;
        stats.lastUsed = std::time(nullptr);
        SaveStats();
    }
    
    static std::vector<BlueprintTemplate> GetMostUsedTemplates(int count) {
        // Sort templates by usage
        std::vector<std::pair<std::string, int>> usage;
        for (const auto& [id, stats] : m_AllStats) {
            usage.push_back({id, stats.timesApplied});
        }
        
        std::sort(usage.begin(), usage.end(),
                 [](const auto& a, const auto& b) {
                     return a.second > b.second;
                 });
        
        // Get top templates
        std::vector<BlueprintTemplate> results;
        for (int i = 0; i < count && i < usage.size(); i++) {
            auto* tpl = TemplateManager::Get().FindTemplate(usage[i].first);
            if (tpl) results.push_back(*tpl);
        }
        
        return results;
    }

private:
    static std::map<std::string, TemplateStats> m_AllStats;
};
```

## Testing

### Unit Tests

```cpp
TEST(TemplateManager, SaveAndLoadTemplate) {
    TemplateManager mgr;
    mgr.Initialize("test_templates");
    
    // Create template
    BlueprintTemplate tpl;
    tpl.id = "test-123";
    tpl.name = "Test Template";
    tpl.description = "Test";
    tpl.category = "Test";
    
    // Save
    ASSERT_TRUE(mgr.SaveTemplate(tpl));
    
    // Load
    auto* loaded = mgr.FindTemplate("test-123");
    ASSERT_NE(loaded, nullptr);
    ASSERT_EQ(loaded->name, "Test Template");
}

TEST(TemplateManager, ApplyTemplate) {
    // Create template with specific blueprint data
    BlueprintTemplate tpl;
    tpl.id = "test-apply";
    tpl.blueprintData = CreateTestBlueprint();
    
    TemplateManager mgr;
    mgr.SaveTemplate(tpl);
    
    // Apply to new blueprint
    json newBlueprint;
    ASSERT_TRUE(mgr.ApplyTemplateToBlueprint("test-apply", newBlueprint));
    
    // Verify blueprint data matches
    ASSERT_EQ(newBlueprint, tpl.blueprintData);
}
```

## Best Practices

### Template Design

✅ **DO:**
- Keep templates focused and specific
- Include comprehensive descriptions
- Use clear category hierarchies
- Version templates when making changes
- Validate templates before saving
- Document custom metadata

❌ **DON'T:**
- Create overly generic templates
- Hardcode project-specific paths
- Skip validation
- Ignore versioning
- Create duplicate templates

### Performance

- **Lazy Loading:** Only load template data when needed
- **Caching:** Cache frequently used templates
- **Async Loading:** Load templates in background
- **Indexing:** Build search index for fast queries

### Security

- **Validation:** Always validate loaded templates
- **Sanitization:** Sanitize user input in templates
- **Access Control:** Implement permissions if needed
- **Backup:** Regular backups of template directory

---

**See Also:**
- [Templates User Guide](TEMPLATES_GUIDE.md)
- [Command System Architecture](COMMAND_SYSTEM_ARCHITECTURE.md)
- [Blueprint Editor API Reference](../QUICKSTART.md)
