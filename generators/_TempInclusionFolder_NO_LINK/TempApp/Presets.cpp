#include "Presets.hpp"
#include <nlohmann/json.hpp>
#include <fstream>
#include <iostream>

namespace RCG
{
    using json = nlohmann::json;

    PresetManager::PresetManager()
    {
    }

    PresetManager::~PresetManager()
    {
    }

    void PresetManager::load_presets(const std::string &filepath)
    {
        std::ifstream file(filepath);
        if (!file.is_open())
        {
            // If file doesn't exist, initialize with defaults
            initialize_defaults();
            return;
        }

        std::string json_string((std::istreambuf_iterator<char>(file)),
                                std::istreambuf_iterator<char>());
        file.close();

        if (!load_json(json_string))
        {
            std::cerr << "Failed to load presets from " << filepath << std::endl;
            initialize_defaults();
        }
    }

    void PresetManager::save_presets(const std::string &filepath)
    {
        std::string json_string = save_json();
        std::ofstream file(filepath);
        if (file.is_open())
        {
            file << json_string;
            file.close();
        }
        else
        {
            std::cerr << "Failed to save presets to " << filepath << std::endl;
        }
    }

    void PresetManager::add_preset(const CityPreset &preset)
    {
        // Check if preset with same name exists
        for (auto &p : presets)
        {
            if (p.name == preset.name)
            {
                // Update existing preset
                p = preset;
                return;
            }
        }
        // Add new preset
        presets.push_back(preset);
    }

    void PresetManager::remove_preset(const std::string &preset_name)
    {
        presets.erase(
            std::remove_if(presets.begin(), presets.end(),
                           [&preset_name](const CityPreset &p)
                           { return p.name == preset_name; }),
            presets.end());
    }

    bool PresetManager::has_preset(const std::string &preset_name) const
    {
        for (const auto &p : presets)
        {
            if (p.name == preset_name)
                return true;
        }
        return false;
    }

    CityPreset PresetManager::get_preset(const std::string &preset_name) const
    {
        for (const auto &p : presets)
        {
            if (p.name == preset_name)
                return p;
        }
        // Return empty preset if not found
        return CityPreset();
    }

    CityPreset PresetManager::create_urban_dense_preset()
    {
        CityParams params;
        params.noise_scale = 0.8f;
        params.city_size = 2048;
        params.road_density = 0.8f;
        params.major_road_iterations = 75;
        params.minor_road_iterations = 40;
        params.vertex_snap_distance = 10.0f;
        params.maxMajorRoads = 2000;
        params.maxTotalRoads = 6000;
        params.majorToMinorRatio = 0.35f;
        params.building_density = 0.9f;
        params.max_building_height = 80.0f;
        params.grid_rule_weight = 2;
        params.radial_rule_weight = 1;
        params.organic_rule_weight = 1;
        params.seed = 12345;

        return CityPreset("Urban Dense", "High-density urban environment with tight grid", params);
    }

    CityPreset PresetManager::create_suburban_preset()
    {
        CityParams params;
        params.noise_scale = 1.0f;
        params.city_size = 1024;
        params.road_density = 0.5f;
        params.major_road_iterations = 50;
        params.minor_road_iterations = 25;
        params.vertex_snap_distance = 15.0f;
        params.maxMajorRoads = 1000;
        params.maxTotalRoads = 3000;
        params.majorToMinorRatio = 0.3f;
        params.building_density = 0.6f;
        params.max_building_height = 30.0f;
        params.grid_rule_weight = 1;
        params.radial_rule_weight = 1;
        params.organic_rule_weight = 1;
        params.seed = 23456;

        return CityPreset("Suburban", "Moderate density suburban layout", params);
    }

    CityPreset PresetManager::create_rural_preset()
    {
        CityParams params;
        params.noise_scale = 1.5f;
        params.city_size = 1024;
        params.road_density = 0.3f;
        params.major_road_iterations = 30;
        params.minor_road_iterations = 15;
        params.vertex_snap_distance = 20.0f;
        params.maxMajorRoads = 500;
        params.maxTotalRoads = 1500;
        params.majorToMinorRatio = 0.25f;
        params.building_density = 0.3f;
        params.max_building_height = 15.0f;
        params.grid_rule_weight = 1;
        params.radial_rule_weight = 1;
        params.organic_rule_weight = 2;
        params.seed = 34567;

        return CityPreset("Rural", "Low-density rural roads with organic growth", params);
    }

    CityPreset PresetManager::create_organic_preset()
    {
        CityParams params;
        params.noise_scale = 1.8f;
        params.city_size = 1024;
        params.road_density = 0.6f;
        params.major_road_iterations = 60;
        params.minor_road_iterations = 30;
        params.vertex_snap_distance = 12.0f;
        params.maxMajorRoads = 1200;
        params.maxTotalRoads = 3500;
        params.majorToMinorRatio = 0.3f;
        params.building_density = 0.7f;
        params.max_building_height = 40.0f;
        params.grid_rule_weight = 0;
        params.radial_rule_weight = 2;
        params.organic_rule_weight = 3;
        params.seed = 45678;

        return CityPreset("Organic", "Natural, flowing road patterns", params);
    }

    CityPreset PresetManager::create_grid_city_preset()
    {
        CityParams params;
        params.noise_scale = 0.5f;
        params.city_size = 2048;
        params.road_density = 0.7f;
        params.major_road_iterations = 80;
        params.minor_road_iterations = 35;
        params.vertex_snap_distance = 8.0f;
        params.maxMajorRoads = 1800;
        params.maxTotalRoads = 5000;
        params.majorToMinorRatio = 0.35f;
        params.building_density = 0.85f;
        params.max_building_height = 60.0f;
        params.grid_rule_weight = 4;
        params.radial_rule_weight = 0;
        params.organic_rule_weight = 0;
        params.seed = 56789;

        return CityPreset("Grid City", "Pure grid-based Manhattan-style layout", params);
    }

    void PresetManager::initialize_defaults()
    {
        presets.clear();
        presets.push_back(create_urban_dense_preset());
        presets.push_back(create_suburban_preset());
        presets.push_back(create_rural_preset());
        presets.push_back(create_organic_preset());
        presets.push_back(create_grid_city_preset());
    }

    bool PresetManager::load_json(const std::string &json_string)
    {
        try
        {
            json j = json::parse(json_string);
            presets.clear();

            if (j.contains("presets") && j["presets"].is_array())
            {
                for (const auto &preset_json : j["presets"])
                {
                    CityPreset preset;
                    preset.name = preset_json.value("name", "Unnamed");
                    preset.description = preset_json.value("description", "");
                    if (preset_json.contains("params"))
                    {
                        from_json(preset_json["params"], preset.params);
                    }
                    presets.push_back(preset);
                }
            }
            return true;
        }
        catch (const std::exception &e)
        {
            std::cerr << "Error parsing presets JSON: " << e.what() << std::endl;
            return false;
        }
    }

    std::string PresetManager::save_json()
    {
        json j;
        j["presets"] = json::array();

        for (const auto &preset : presets)
        {
            json preset_json;
            preset_json["name"] = preset.name;
            preset_json["description"] = preset.description;
            to_json(preset_json["params"], preset.params);
            j["presets"].push_back(preset_json);
        }

        return j.dump(4); // Pretty print with 4-space indent
    }

} // namespace RCG
