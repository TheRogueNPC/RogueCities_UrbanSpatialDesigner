#pragma once

#include <vector>
#include "CityParams.h"
#include "CityModel.h"
#include "AxiomInput.h"
#include "TensorField.h"

namespace DistrictGenerator
{
    struct Settings
    {
        int grid_resolution{128};
        double secondary_threshold{0.2};
        double weight_scale{1.0};
        bool use_reaction_diffusion{false};
        double rd_mix{0.0};
        double desire_weight_axiom{0.6};
        double desire_weight_frontage{0.4};
        bool disable_weight_normalization{false};
        double desire_score_epsilon{1e-6};
        bool enable_desire_geometry_factor{false};
        double desire_density_radius{200.0};
        bool debug_log_desire_scores{false};
        bool enable_adaptive_resolution{false};  // Adaptive grid resolution
        int min_grid_resolution{64};
        int max_grid_resolution{512};
        bool split_disconnected_regions{true};  // Connected-component pass
        bool use_local_secondary_cutoff{true};  // Use local variance instead of global avg
        double fixed_secondary_cutoff{0.15};  // Fixed value if not using global avg
    };

    struct DistrictField
    {
        int width{0};
        int height{0};
        CityModel::Vec2 origin{};
        CityModel::Vec2 cell_size{};
        std::vector<uint32_t> district_ids;

        bool valid() const { return width > 0 && height > 0 && !district_ids.empty(); }
        uint32_t sample_id(const CityModel::Vec2 &pos) const;
    };

    void generate(const CityParams &params,
                  const std::vector<AxiomInput> &axioms,
                  CityModel::City &city,
                  DistrictField &out_field,
                  const Settings &settings = Settings{},
                  const TensorField::TensorField *tensor_field = nullptr);

    void clip_roads_to_districts(CityModel::City &city, const DistrictField &field);
} // namespace DistrictGenerator
