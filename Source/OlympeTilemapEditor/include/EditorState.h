/*
 * Olympe Tilemap Editor - Editor State
 * 
 * Maintains the current state of the editor including selected tools,
 * tiles, layers, undo/redo history, and other editor-specific state.
 */

#ifndef OLYMPE_EDITOR_EDITORSTATE_H
#define OLYMPE_EDITOR_EDITORSTATE_H

#include <string>
#include <vector>
#include <memory>
#include <stack>
#include "../../../Source/third_party/nlohmann/json.hpp"

namespace Olympe {
namespace Editor {

/**
 * Editor tool types
 */
enum class EditorTool {
    NONE,
    BRUSH,
    ERASER,
    FILL,
    RECTANGLE,
    EYEDROPPER,
    SELECT
};

/**
 * Editor layer structure
 */
struct EditorLayer {
    std::string name;
    bool visible;
    bool locked;
    float opacity;
    int z_index;
};

/**
 * Command pattern for undo/redo
 */
struct EditorCommand {
    std::string command_type;
    nlohmann::json data;
    std::string timestamp;
};

/**
 * EditorState class
 * Manages the current state of the tilemap editor
 */
class EditorState {
public:
    EditorState();
    ~EditorState();

    // Tool management
    void set_current_tool(EditorTool tool);
    EditorTool get_current_tool() const;
    const std::string& get_tool_name() const;
    
    // Tile selection
    void set_selected_tile(int tileset_id, int tile_id);
    void get_selected_tile(int& out_tileset_id, int& out_tile_id) const;
    bool has_tile_selected() const;
    void clear_tile_selection();
    
    // Multi-tile selection
    void set_selected_tiles(const std::vector<int>& tile_ids);
    const std::vector<int>& get_selected_tiles() const;
    
    // Layer management
    void add_layer(const std::string& name);
    void remove_layer(int index);
    void set_active_layer(int index);
    int get_active_layer() const;
    const std::vector<EditorLayer>& get_layers() const;
    EditorLayer& get_layer(int index);
    void reorder_layers(int from_index, int to_index);
    
    // Undo/Redo
    void push_command(const EditorCommand& command);
    bool can_undo() const;
    bool can_redo() const;
    EditorCommand undo();
    EditorCommand redo();
    void clear_history();
    size_t get_undo_stack_size() const;
    size_t get_redo_stack_size() const;
    
    // Grid settings
    void set_grid_visible(bool visible);
    bool is_grid_visible() const;
    void set_grid_snap(bool snap);
    bool is_grid_snap_enabled() const;
    void set_grid_size(int width, int height);
    void get_grid_size(int& out_width, int& out_height) const;
    
    // Camera/View state
    void set_camera_position(float x, float y);
    void get_camera_position(float& out_x, float& out_y) const;
    void set_zoom_level(float zoom);
    float get_zoom_level() const;
    
    // Selection state
    void set_selection_rect(int x, int y, int width, int height);
    void get_selection_rect(int& out_x, int& out_y, int& out_width, int& out_height) const;
    bool has_selection() const;
    void clear_selection();
    
    // State queries
    bool is_dirty() const;
    void mark_dirty();
    void mark_clean();
    
    // Clipboard
    void copy_to_clipboard(const nlohmann::json& data);
    const nlohmann::json& get_clipboard_data() const;
    bool has_clipboard_data() const;

private:
    // Tool state
    EditorTool current_tool_;
    int selected_tileset_id_;
    int selected_tile_id_;
    std::vector<int> selected_tiles_;
    
    // Layer state
    std::vector<EditorLayer> layers_;
    int active_layer_index_;
    
    // Undo/Redo stacks
    std::stack<EditorCommand> undo_stack_;
    std::stack<EditorCommand> redo_stack_;
    static const size_t MAX_UNDO_STACK_SIZE = 100;
    
    // Grid settings
    bool grid_visible_;
    bool grid_snap_;
    int grid_width_;
    int grid_height_;
    
    // Camera state
    float camera_x_;
    float camera_y_;
    float zoom_level_;
    
    // Selection state
    bool has_selection_;
    int selection_x_;
    int selection_y_;
    int selection_width_;
    int selection_height_;
    
    // Clipboard
    nlohmann::json clipboard_data_;
    
    // Dirty flag
    bool is_dirty_;
    
    // Helper methods
    std::string tool_to_string(EditorTool tool) const;
    void limit_undo_stack_size();
};

} // namespace Editor
} // namespace Olympe

#endif // OLYMPE_EDITOR_EDITORSTATE_H
