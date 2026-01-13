/*
 * Olympe Tilemap Editor - Editor State Implementation
 */

#include "../include/EditorState.h"
#include <iostream>
#include <algorithm>

namespace Olympe {
namespace Editor {

EditorState::EditorState()
    : current_tool_(EditorTool::BRUSH)
    , selected_tileset_id_(-1)
    , selected_tile_id_(-1)
    , active_layer_index_(0)
    , grid_visible_(true)
    , grid_snap_(true)
    , grid_width_(32)
    , grid_height_(32)
    , camera_x_(0.0f)
    , camera_y_(0.0f)
    , zoom_level_(1.0f)
    , has_selection_(false)
    , selection_x_(0)
    , selection_y_(0)
    , selection_width_(0)
    , selection_height_(0)
    , is_dirty_(false)
{
    std::cout << "[EditorState] Initialized" << std::endl;
    
    // Create default layer
    EditorLayer default_layer;
    default_layer.name = "Layer 0";
    default_layer.visible = true;
    default_layer.locked = false;
    default_layer.opacity = 1.0f;
    default_layer.z_index = 0;
    layers_.push_back(default_layer);
}

EditorState::~EditorState()
{
    std::cout << "[EditorState] Shutdown" << std::endl;
}

void EditorState::set_current_tool(EditorTool tool)
{
    current_tool_ = tool;
    std::cout << "[EditorState] Tool changed to: " << get_tool_name() << std::endl;
}

EditorTool EditorState::get_current_tool() const
{
    return current_tool_;
}

const std::string& EditorState::get_tool_name() const
{
    static const std::string tool_names[] = {
        "None", "Brush", "Eraser", "Fill", "Rectangle", "Eyedropper", "Select"
    };
    return tool_names[static_cast<int>(current_tool_)];
}

void EditorState::set_selected_tile(int tileset_id, int tile_id)
{
    selected_tileset_id_ = tileset_id;
    selected_tile_id_ = tile_id;
    selected_tiles_.clear();
    selected_tiles_.push_back(tile_id);
}

void EditorState::get_selected_tile(int& out_tileset_id, int& out_tile_id) const
{
    out_tileset_id = selected_tileset_id_;
    out_tile_id = selected_tile_id_;
}

bool EditorState::has_tile_selected() const
{
    return selected_tile_id_ >= 0;
}

void EditorState::clear_tile_selection()
{
    selected_tileset_id_ = -1;
    selected_tile_id_ = -1;
    selected_tiles_.clear();
}

void EditorState::set_selected_tiles(const std::vector<int>& tile_ids)
{
    selected_tiles_ = tile_ids;
}

const std::vector<int>& EditorState::get_selected_tiles() const
{
    return selected_tiles_;
}

void EditorState::add_layer(const std::string& name)
{
    EditorLayer layer;
    layer.name = name;
    layer.visible = true;
    layer.locked = false;
    layer.opacity = 1.0f;
    layer.z_index = static_cast<int>(layers_.size());
    layers_.push_back(layer);
    mark_dirty();
}

void EditorState::remove_layer(int index)
{
    if (index >= 0 && index < static_cast<int>(layers_.size())) {
        layers_.erase(layers_.begin() + index);
        if (active_layer_index_ >= static_cast<int>(layers_.size())) {
            active_layer_index_ = static_cast<int>(layers_.size()) - 1;
        }
        mark_dirty();
    }
}

void EditorState::set_active_layer(int index)
{
    if (index >= 0 && index < static_cast<int>(layers_.size())) {
        active_layer_index_ = index;
    }
}

int EditorState::get_active_layer() const
{
    return active_layer_index_;
}

const std::vector<EditorLayer>& EditorState::get_layers() const
{
    return layers_;
}

EditorLayer& EditorState::get_layer(int index)
{
    return layers_[index];
}

void EditorState::reorder_layers(int from_index, int to_index)
{
    if (from_index >= 0 && from_index < static_cast<int>(layers_.size()) &&
        to_index >= 0 && to_index < static_cast<int>(layers_.size()) &&
        from_index != to_index) {
        EditorLayer temp = layers_[from_index];
        layers_.erase(layers_.begin() + from_index);
        layers_.insert(layers_.begin() + to_index, temp);
        mark_dirty();
    }
}

void EditorState::push_command(const EditorCommand& command)
{
    undo_stack_.push(command);
    limit_undo_stack_size();
    
    // Clear redo stack when new command is pushed
    while (!redo_stack_.empty()) {
        redo_stack_.pop();
    }
    
    mark_dirty();
}

bool EditorState::can_undo() const
{
    return !undo_stack_.empty();
}

bool EditorState::can_redo() const
{
    return !redo_stack_.empty();
}

EditorCommand EditorState::undo()
{
    if (!can_undo()) {
        return EditorCommand();
    }
    
    EditorCommand command = undo_stack_.top();
    undo_stack_.pop();
    redo_stack_.push(command);
    
    return command;
}

EditorCommand EditorState::redo()
{
    if (!can_redo()) {
        return EditorCommand();
    }
    
    EditorCommand command = redo_stack_.top();
    redo_stack_.pop();
    undo_stack_.push(command);
    
    return command;
}

void EditorState::clear_history()
{
    while (!undo_stack_.empty()) {
        undo_stack_.pop();
    }
    while (!redo_stack_.empty()) {
        redo_stack_.pop();
    }
}

size_t EditorState::get_undo_stack_size() const
{
    return undo_stack_.size();
}

size_t EditorState::get_redo_stack_size() const
{
    return redo_stack_.size();
}

void EditorState::set_grid_visible(bool visible)
{
    grid_visible_ = visible;
}

bool EditorState::is_grid_visible() const
{
    return grid_visible_;
}

void EditorState::set_grid_snap(bool snap)
{
    grid_snap_ = snap;
}

bool EditorState::is_grid_snap_enabled() const
{
    return grid_snap_;
}

void EditorState::set_grid_size(int width, int height)
{
    grid_width_ = width;
    grid_height_ = height;
}

void EditorState::get_grid_size(int& out_width, int& out_height) const
{
    out_width = grid_width_;
    out_height = grid_height_;
}

void EditorState::set_camera_position(float x, float y)
{
    camera_x_ = x;
    camera_y_ = y;
}

void EditorState::get_camera_position(float& out_x, float& out_y) const
{
    out_x = camera_x_;
    out_y = camera_y_;
}

void EditorState::set_zoom_level(float zoom)
{
    zoom_level_ = std::max(0.1f, std::min(10.0f, zoom));
}

float EditorState::get_zoom_level() const
{
    return zoom_level_;
}

void EditorState::set_selection_rect(int x, int y, int width, int height)
{
    has_selection_ = true;
    selection_x_ = x;
    selection_y_ = y;
    selection_width_ = width;
    selection_height_ = height;
}

void EditorState::get_selection_rect(int& out_x, int& out_y, int& out_width, int& out_height) const
{
    out_x = selection_x_;
    out_y = selection_y_;
    out_width = selection_width_;
    out_height = selection_height_;
}

bool EditorState::has_selection() const
{
    return has_selection_;
}

void EditorState::clear_selection()
{
    has_selection_ = false;
    selection_x_ = 0;
    selection_y_ = 0;
    selection_width_ = 0;
    selection_height_ = 0;
}

bool EditorState::is_dirty() const
{
    return is_dirty_;
}

void EditorState::mark_dirty()
{
    is_dirty_ = true;
}

void EditorState::mark_clean()
{
    is_dirty_ = false;
}

void EditorState::copy_to_clipboard(const nlohmann::json& data)
{
    clipboard_data_ = data;
}

const nlohmann::json& EditorState::get_clipboard_data() const
{
    return clipboard_data_;
}

bool EditorState::has_clipboard_data() const
{
    return !clipboard_data_.empty();
}

std::string EditorState::tool_to_string(EditorTool tool) const
{
    return get_tool_name();
}

void EditorState::limit_undo_stack_size()
{
    // TODO: Implement undo stack size limiting
    // This would require converting stack to a custom container
}

} // namespace Editor
} // namespace Olympe
