#pragma once

#include "CityParams.hpp"
#include <string>
#include <vector>

namespace RCG
{
    /**
     * @struct CityPreset
     * @brief Stores a named set of city generation parameters
     */
    struct CityPreset
    {
        std::string name;
        std::string description;
        CityParams params;

        CityPreset() = default;
        CityPreset(const std::string &preset_name, const std::string &preset_desc, const CityParams &preset_params)
            : name(preset_name), description(preset_desc), params(preset_params) {}
    };

    /**
     * @class PresetManager
     * @brief Manages saving, loading, and organizing city generation presets
     */
    class PresetManager
    {
    public:
        PresetManager();
        ~PresetManager();

        // Load/Save presets from disk
        void load_presets(const std::string &filepath = "presets.json");
        void save_presets(const std::string &filepath = "presets.json");

        // Preset management
        void add_preset(const CityPreset &preset);
        void remove_preset(const std::string &preset_name);
        bool has_preset(const std::string &preset_name) const;
        CityPreset get_preset(const std::string &preset_name) const;
        const std::vector<CityPreset> &get_all_presets() const { return presets; }

        // Built-in presets
        static CityPreset create_urban_dense_preset();
        static CityPreset create_suburban_preset();
        static CityPreset create_rural_preset();
        static CityPreset create_organic_preset();
        static CityPreset create_grid_city_preset();

        // Initialize default presets
        void initialize_defaults();

    private:
        std::vector<CityPreset> presets;

        bool load_json(const std::string &json_string);
        std::string save_json();
    };

} // namespace RCG
