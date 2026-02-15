// FILE: LayoutPresets.h
// PURPOSE: Workspace layout persistence & presets (Y2K Cockpit configurations)
// PATTERN: JSON-based dock tree serialization with built-in presets

#pragma once

#include <string>
#include <vector>
#include <unordered_map>

namespace RC_UI::Layout {

// Dock layout node (tree structure)
struct DockNode {
    std::string name;                // Window/panel name
    int split_axis = 0;              // 0=none, 1=horizontal, 2=vertical
    float split_ratio = 0.5f;        // 0..1 for child split
    bool is_floating = false;        // Floating window flag
    ImVec2 float_pos = {0, 0};      // Position if floating
    ImVec2 float_size = {400, 300}; // Size if floating
    
    std::vector<DockNode> children;  // Child nodes (if split)
};

// Layout preset (named configuration)
struct LayoutPreset {
    std::string name;                // "Urban Planning", "Axiom Focused", etc.
    std::string description;         // Brief explanation
    DockNode root;                   // Root dock node tree
};

// Layout manager
class LayoutManager {
public:
    static LayoutManager& Instance() {
        static LayoutManager instance;
        return instance;
    }
    
    // === PRESET MANAGEMENT ===
    
    // Get all available presets (built-in + custom)
    std::vector<std::string> GetPresetNames() const;
    
    // Load a preset by name
    bool LoadPreset(const std::string& name);
    
    // Save current layout as custom preset
    bool SaveCurrentLayout(const std::string& name, const std::string& description = "");
    
    // Delete custom preset
    bool DeletePreset(const std::string& name);
    
    // === PERSISTENCE ===
    
    // Save current layout to JSON file
    bool SaveToFile(const std::string& filepath) const;
    
    // Load layout from JSON file
    bool LoadFromFile(const std::string& filepath);
    
    // Get current active preset name
    std::string GetActivePresetName() const { return m_active_preset; }
    
    // === BUILT-IN PRESETS ===
    
    // Create "Urban Planning" preset (60% viewport, 20/20 sides, 18% bottom)
    static LayoutPreset CreateUrbanPlanningPreset();
    
    // Create "Axiom Focused" preset (50% viewport, 30% left axiom, 20% right tools)
    static LayoutPreset CreateAxiomFocusedPreset();
    
    // Create "Data Heavy" preset (40% viewport, 25/35 sides for data panels)
    static LayoutPreset CreateDataHeavyPreset();
    
private:
    LayoutManager();
    
    // Load built-in presets
    void LoadBuiltInPresets();
    
    // Apply preset to ImGui docking
    void ApplyPreset(const LayoutPreset& preset);
    
    // Serialize/deserialize
    std::string SerializeToJson(const LayoutPreset& preset) const;
    LayoutPreset DeserializeFromJson(const std::string& json) const;
    
    std::unordered_map<std::string, LayoutPreset> m_presets;
    std::string m_active_preset = "Urban Planning";
};

} // namespace RC_UI::Layout
