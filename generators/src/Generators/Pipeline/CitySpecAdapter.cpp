#include "RogueCity/Generators/Pipeline/CitySpecAdapter.hpp"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdint>
#include <functional>
#include <string>

namespace RogueCity::Generators {

namespace {

std::string ToLower(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return value;
}

bool ContainsToken(const std::string& haystackLower, const char* tokenLower) {
    return haystackLower.find(tokenLower) != std::string::npos;
}

uint32_t FallbackSeedFromSpec(const Core::CitySpec& spec) {
    std::string key = spec.intent.description + "|" + spec.intent.scale + "|" + spec.intent.climate;
    for (const auto& tag : spec.intent.styleTags) {
        key += "|" + tag;
    }
    for (const auto& district : spec.districts) {
        key += "|" + district.type + ":" + std::to_string(district.density);
    }

    const size_t h = std::hash<std::string>{}(key);
    const uint32_t folded = static_cast<uint32_t>((h ^ (h >> 32)) & 0xFFFFFFFFu);
    return folded == 0u ? 1u : folded;
}

void ApplyScaleDefaults(
    const std::string& scaleLower,
    CityGenerator::Config& config,
    float& footprintScale,
    int& baseSeedCount) {
    if (scaleLower == "hamlet") {
        config.width = 900;
        config.height = 900;
        footprintScale = 0.55f;
        baseSeedCount = 8;
        return;
    }
    if (scaleLower == "town") {
        config.width = 1400;
        config.height = 1400;
        footprintScale = 0.78f;
        baseSeedCount = 12;
        return;
    }
    if (scaleLower == "metro") {
        config.width = 3200;
        config.height = 3200;
        footprintScale = 1.45f;
        baseSeedCount = 34;
        return;
    }

    // Default to "city".
    config.width = 2200;
    config.height = 2200;
    footprintScale = 1.0f;
    baseSeedCount = 22;
}

CityGenerator::AxiomInput BuildDistrictAxiom(
    const Core::DistrictHint& district,
    const Core::Vec2& center,
    const float footprintScale,
    const size_t index,
    const size_t count) {
    const std::string kind = ToLower(district.type);
    const float density = std::clamp(district.density, 0.0f, 1.0f);

    constexpr double kPi = 3.14159265358979323846;
    const double angle = (count == 0) ? 0.0 : (2.0 * kPi * static_cast<double>(index) / static_cast<double>(count));
    const float ring = (160.0f + static_cast<float>(index % 3) * 120.0f) * footprintScale;

    CityGenerator::AxiomInput axiom;
    axiom.position = Core::Vec2(
        center.x + std::cos(angle) * ring,
        center.y + std::sin(angle) * ring);
    axiom.radius = std::clamp(static_cast<double>((220.0f + 420.0f * density) * footprintScale), 90.0, 1200.0);
    axiom.decay = std::clamp(3.0 - static_cast<double>(density) * 1.6, 0.9, 3.8);
    axiom.theta = angle;

    if (kind == "downtown" || kind == "commercial" || kind == "civic") {
        axiom.type = (density > 0.75f) ? CityGenerator::AxiomInput::Type::Radial : CityGenerator::AxiomInput::Type::Grid;
        axiom.radial_spokes = std::clamp(6 + static_cast<int>(density * 14.0f), 6, 24);
        axiom.loose_grid_jitter = std::clamp(0.05f + (1.0f - density) * 0.20f, 0.03f, 0.32f);
    } else if (kind == "industrial" || kind == "logistics") {
        axiom.type = (density > 0.65f) ? CityGenerator::AxiomInput::Type::Linear : CityGenerator::AxiomInput::Type::Superblock;
        axiom.superblock_block_size = std::clamp((220.0f + (1.0f - density) * 240.0f) * footprintScale, 150.0f, 520.0f);
    } else if (kind == "residential" || kind == "suburban") {
        axiom.type = CityGenerator::AxiomInput::Type::Suburban;
        axiom.suburban_loop_strength = std::clamp(0.45f + (1.0f - density) * 0.45f, 0.30f, 0.95f);
    } else {
        axiom.type = CityGenerator::AxiomInput::Type::Organic;
        axiom.organic_curviness = std::clamp(0.35f + (1.0f - density) * 0.45f, 0.15f, 0.95f);
    }

    return axiom;
}

} // namespace

bool CitySpecAdapter::TryBuildRequest(
    const Core::CitySpec& spec,
    CitySpecGenerationRequest& outRequest,
    std::string* outError) {
    outRequest = CitySpecGenerationRequest{};

    const std::string scaleLower = ToLower(spec.intent.scale.empty() ? "city" : spec.intent.scale);
    float footprintScale = 1.0f;
    int baseSeedCount = 20;

    ApplyScaleDefaults(scaleLower, outRequest.config, footprintScale, baseSeedCount);
    outRequest.config.cell_size = 10.0;

    const float roadDensity = std::clamp(spec.roadDensity, 0.05f, 1.0f);
    outRequest.config.num_seeds = std::max(8, static_cast<int>(std::round(baseSeedCount * (0.5f + roadDensity))));
    outRequest.config.seed = spec.seed == 0 ? FallbackSeedFromSpec(spec) : spec.seed;

    outRequest.tags.reserve(spec.intent.styleTags.size() + spec.districts.size() + 2);
    outRequest.tags.push_back("scale:" + scaleLower);
    outRequest.tags.push_back("climate:" + ToLower(spec.intent.climate));
    for (const auto& tag : spec.intent.styleTags) {
        outRequest.tags.push_back("style:" + ToLower(tag));
    }

    const Core::Vec2 center(
        static_cast<double>(outRequest.config.width) * 0.5,
        static_cast<double>(outRequest.config.height) * 0.5);

    const std::string descLower = ToLower(spec.intent.description);
    if (ContainsToken(descLower, "coastal") || ContainsToken(descLower, "harbor") || ContainsToken(descLower, "waterfront")) {
        CityGenerator::AxiomInput shoreline;
        shoreline.type = CityGenerator::AxiomInput::Type::Linear;
        shoreline.position = Core::Vec2(center.x, center.y - static_cast<double>(outRequest.config.height) * 0.28);
        shoreline.radius = std::max(200.0, static_cast<double>(outRequest.config.width) * 0.52);
        shoreline.theta = 0.0;
        shoreline.decay = 1.6;
        outRequest.axioms.push_back(shoreline);
        outRequest.tags.push_back("hint:coastal");
    }

    const size_t districtCount = spec.districts.size();
    for (size_t i = 0; i < districtCount; ++i) {
        const auto& district = spec.districts[i];
        outRequest.axioms.push_back(BuildDistrictAxiom(district, center, footprintScale, i, districtCount));
        outRequest.tags.push_back("district:" + ToLower(district.type));
    }

    if (outRequest.axioms.empty()) {
        CityGenerator::AxiomInput fallback;
        fallback.type = CityGenerator::AxiomInput::Type::Grid;
        fallback.position = center;
        fallback.radius = std::clamp(static_cast<double>(outRequest.config.width) * 0.28, 180.0, 950.0);
        fallback.theta = 0.0;
        fallback.decay = 2.0;
        outRequest.axioms.push_back(fallback);
        outRequest.tags.push_back("fallback:grid");
    }

    for (const auto& axiom : outRequest.axioms) {
        const bool inBounds = axiom.position.x >= 0.0 &&
                              axiom.position.y >= 0.0 &&
                              axiom.position.x <= static_cast<double>(outRequest.config.width) &&
                              axiom.position.y <= static_cast<double>(outRequest.config.height);
        if (!inBounds) {
            if (outError) {
                *outError = "CitySpec produced an out-of-bounds axiom position";
            }
            return false;
        }
    }

    return true;
}

CitySpecGenerationRequest CitySpecAdapter::BuildRequest(const Core::CitySpec& spec) {
    CitySpecGenerationRequest request;
    std::string error;
    if (!TryBuildRequest(spec, request, &error)) {
        request = CitySpecGenerationRequest{};
    }
    return request;
}

} // namespace RogueCity::Generators
