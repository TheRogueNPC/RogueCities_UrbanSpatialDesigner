/**
 * @file DeterminismHash.cpp
 * @brief Implements deterministic hashing for urban spatial entities in RogueCities.
 *
 * Provides functions to compute, save, and validate determinism hashes for roads, districts, lots, buildings,
 * and tensor layers within the editor's global state. Uses FNV-1a hashing and normalization of floating-point values
 * to ensure consistent results across platforms and runs.
 *
 * Key features:
 * - Hashing of scalar, boolean, float, double, and vector types.
 * - Deterministic ordering of containers by entity ID before hashing.
 * - Normalization of floating-point values to handle zero and NaN consistently.
 * - Parsing and trimming of baseline hash files for validation.
 * - Serialization of determinism hash values to string and file.
 *
 * Functions:
 * - ComputeDeterminismHash: Computes determinism hash for the current editor state.
 * - SaveBaselineHash: Saves determinism hash to a file in a standardized format.
 * - ValidateAgainstBaseline: Validates current hash against a saved baseline.
 * - DeterminismHash::to_string: Serializes hash values to a formatted string.
 * - DeterminismHash::operator==: Compares two determinism hashes for equality.
 *
 * Internal helpers:
 * - HashBytes, HashScalar, HashBool, HashFloat, HashDouble, HashVec2, HashRoad, HashDistrict, HashLot, HashBuilding
 * - HashSortedByID: Hashes containers in deterministic order.
 * - HashTensorLayer: Hashes tensor field layer if present.
 * - Trim, ParseU64, ParseBaseline: String and file parsing utilities.
 *
 * @namespace RogueCity::Core::Validation
 */
 
#include "RogueCity/Core/Validation/DeterminismHash.hpp"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <limits>
#include <optional>
#include <sstream>
#include <string_view>
#include <type_traits>
#include <vector>

namespace RogueCity::Core::Validation {

    namespace {

        constexpr uint64_t kFnvOffsetBasis = 14695981039346656037ull;
        constexpr uint64_t kFnvPrime = 1099511628211ull;

        void HashBytes(const void* data, size_t size, uint64_t& hash) {
            const auto* bytes = static_cast<const uint8_t*>(data);
            for (size_t i = 0; i < size; ++i) {
                hash ^= static_cast<uint64_t>(bytes[i]);
                hash *= kFnvPrime;
            }
        }

        template <typename T>
        void HashScalar(const T& value, uint64_t& hash) {
            HashBytes(&value, sizeof(T), hash);
        }

        [[nodiscard]] uint32_t NormalizeFloatBits(float value) {
            float normalized = value;
            if (normalized == 0.0f) {
                normalized = 0.0f;
            } else if (std::isnan(normalized)) {
                normalized = std::numeric_limits<float>::quiet_NaN();
            }
            uint32_t bits = 0u;
            std::memcpy(&bits, &normalized, sizeof(bits));
            return bits;
        }

        [[nodiscard]] uint64_t NormalizeDoubleBits(double value) {
            double normalized = value;
            if (normalized == 0.0) {
                normalized = 0.0;
            } else if (std::isnan(normalized)) {
                normalized = std::numeric_limits<double>::quiet_NaN();
            }
            uint64_t bits = 0u;
            std::memcpy(&bits, &normalized, sizeof(bits));
            return bits;
        }

        void HashBool(bool value, uint64_t& hash) {
            const uint8_t encoded = value ? 1u : 0u;
            HashScalar(encoded, hash);
        }

        void HashDouble(double value, uint64_t& hash) {
            const uint64_t bits = NormalizeDoubleBits(value);
            HashScalar(bits, hash);
        }

        void HashFloat(float value, uint64_t& hash) {
            const uint32_t bits = NormalizeFloatBits(value);
            HashScalar(bits, hash);
        }

        void HashVec2(const Vec2& value, uint64_t& hash) {
            HashDouble(value.x, hash);
            HashDouble(value.y, hash);
        }

        void HashRoad(const Road& road, uint64_t& hash) {
            HashScalar(road.id, hash);
            HashScalar(static_cast<uint8_t>(road.type), hash);
            HashScalar(road.source_axiom_id, hash);
            HashBool(road.is_user_created, hash);
            HashScalar(static_cast<uint8_t>(road.generation_tag), hash);
            HashBool(road.generation_locked, hash);

            const uint64_t point_count = static_cast<uint64_t>(road.points.size());
            HashScalar(point_count, hash);
            for (const auto& point : road.points) {
                HashVec2(point, hash);
            }
        }

        void HashDistrict(const District& district, uint64_t& hash) {
            HashScalar(district.id, hash);
            HashScalar(district.primary_axiom_id, hash);
            HashScalar(district.secondary_axiom_id, hash);
            HashScalar(static_cast<uint8_t>(district.type), hash);
            HashVec2(district.orientation, hash);
            HashFloat(district.budget_allocated, hash);
            HashScalar(district.projected_population, hash);
            HashFloat(district.desirability, hash);
            HashBool(district.is_user_placed, hash);
            HashScalar(static_cast<uint8_t>(district.generation_tag), hash);
            HashBool(district.generation_locked, hash);

            const uint64_t border_count = static_cast<uint64_t>(district.border.size());
            HashScalar(border_count, hash);
            for (const auto& point : district.border) {
                HashVec2(point, hash);
            }
        }

        void HashLot(const LotToken& lot, uint64_t& hash) {
            HashScalar(lot.id, hash);
            HashScalar(lot.district_id, hash);
            HashVec2(lot.centroid, hash);
            HashScalar(static_cast<uint8_t>(lot.primary_road), hash);
            HashScalar(static_cast<uint8_t>(lot.secondary_road), hash);
            HashFloat(lot.access, hash);
            HashFloat(lot.exposure, hash);
            HashFloat(lot.serviceability, hash);
            HashFloat(lot.privacy, hash);
            HashFloat(lot.area, hash);
            HashFloat(lot.budget_allocation, hash);
            HashScalar(static_cast<uint8_t>(lot.lot_type), hash);
            HashBool(lot.is_user_placed, hash);
            HashBool(lot.locked_type, hash);
            HashScalar(static_cast<uint8_t>(lot.generation_tag), hash);
            HashBool(lot.generation_locked, hash);

            const uint64_t boundary_count = static_cast<uint64_t>(lot.boundary.size());
            HashScalar(boundary_count, hash);
            for (const auto& point : lot.boundary) {
                HashVec2(point, hash);
            }
        }

        void HashBuilding(const BuildingSite& building, uint64_t& hash) {
            HashScalar(building.id, hash);
            HashScalar(building.lot_id, hash);
            HashScalar(building.district_id, hash);
            HashVec2(building.position, hash);
            HashFloat(building.rotation_radians, hash);
            HashFloat(building.uniform_scale, hash);
            HashScalar(static_cast<uint8_t>(building.type), hash);
            HashBool(building.is_user_placed, hash);
            HashBool(building.locked_type, hash);
            HashScalar(static_cast<uint8_t>(building.generation_tag), hash);
            HashBool(building.generation_locked, hash);
            HashFloat(building.estimated_cost, hash);
        }

        template <typename TContainer, typename THashFn>
        [[nodiscard]] uint64_t HashSortedByID(const TContainer& container, THashFn hash_fn) {
            using Element = std::remove_cv_t<std::remove_reference_t<decltype(*container.begin())>>;
            std::vector<const Element*> ordered;
            ordered.reserve(container.size());
            for (const auto& value : container) {
                ordered.push_back(&value);
            }
            std::sort(ordered.begin(), ordered.end(), [](const auto* a, const auto* b) {
                return a->id < b->id;
            });

            uint64_t hash = kFnvOffsetBasis;
            const uint64_t count = static_cast<uint64_t>(ordered.size());
            HashScalar(count, hash);
            for (const auto* value : ordered) {
                hash_fn(*value, hash);
            }
            return hash;
        }

        [[nodiscard]] uint64_t HashTensorLayer(const Editor::GlobalState& gs) {
            if (!gs.HasTextureSpace()) {
                return 0ull;
            }

            const auto& texture_space = gs.TextureSpaceRef();
            const auto& tensor = texture_space.tensorLayer();

            uint64_t hash = kFnvOffsetBasis;
            const int resolution = texture_space.resolution();
            const auto bounds = texture_space.bounds();
            HashScalar(resolution, hash);
            HashVec2(bounds.min, hash);
            HashVec2(bounds.max, hash);
            HashScalar(tensor.width(), hash);
            HashScalar(tensor.height(), hash);
            for (const auto& sample : tensor.data()) {
                HashVec2(sample, hash);
            }
            return hash;
        }

        [[nodiscard]] std::string Trim(std::string value) {
            const auto is_space = [](unsigned char c) {
                return std::isspace(c) != 0;
            };

            while (!value.empty() && is_space(static_cast<unsigned char>(value.front()))) {
                value.erase(value.begin());
            }
            while (!value.empty() && is_space(static_cast<unsigned char>(value.back()))) {
                value.pop_back();
            }
            return value;
        }

        [[nodiscard]] std::optional<uint64_t> ParseU64(std::string text) {
            text = Trim(std::move(text));
            if (text.empty()) {
                return std::nullopt;
            }
            try {
                size_t consumed = 0;
                const uint64_t value = std::stoull(text, &consumed, 0);
                if (consumed != text.size()) {
                    return std::nullopt;
                }
                return value;
            } catch (...) {
                return std::nullopt;
            }
        }

        [[nodiscard]] std::optional<DeterminismHash> ParseBaseline(std::istream& input) {
            DeterminismHash parsed{};
            bool saw_roads = false;
            bool saw_districts = false;
            bool saw_lots = false;
            bool saw_buildings = false;
            bool saw_tensor = false;

            std::string line;
            while (std::getline(input, line)) {
                line = Trim(std::move(line));
                if (line.empty() || line[0] == '#') {
                    continue;
                }
                const auto eq = line.find('=');
                if (eq == std::string::npos) {
                    continue;
                }

                const std::string key = Trim(line.substr(0, eq));
                const std::string value_text = line.substr(eq + 1);
                const auto value = ParseU64(value_text);
                if (!value.has_value()) {
                    return std::nullopt;
                }

                if (key == "roads") {
                    parsed.roads_hash = *value;
                    saw_roads = true;
                } else if (key == "districts") {
                    parsed.districts_hash = *value;
                    saw_districts = true;
                } else if (key == "lots") {
                    parsed.lots_hash = *value;
                    saw_lots = true;
                } else if (key == "buildings") {
                    parsed.buildings_hash = *value;
                    saw_buildings = true;
                } else if (key == "tensor") {
                    parsed.tensor_field_hash = *value;
                    saw_tensor = true;
                }
            }

            if (!(saw_roads && saw_districts && saw_lots && saw_buildings && saw_tensor)) {
                return std::nullopt;
            }
            return parsed;
        }

    } // namespace

    bool DeterminismHash::operator==(const DeterminismHash& other) const noexcept {
        return roads_hash == other.roads_hash &&
            districts_hash == other.districts_hash &&
            lots_hash == other.lots_hash &&
            buildings_hash == other.buildings_hash &&
            tensor_field_hash == other.tensor_field_hash;
    }

    std::string DeterminismHash::to_string() const {
        std::ostringstream oss;
        oss << std::hex << std::setfill('0');
        oss << "roads=0x" << std::setw(16) << roads_hash << '\n';
        oss << "districts=0x" << std::setw(16) << districts_hash << '\n';
        oss << "lots=0x" << std::setw(16) << lots_hash << '\n';
        oss << "buildings=0x" << std::setw(16) << buildings_hash << '\n';
        oss << "tensor=0x" << std::setw(16) << tensor_field_hash << '\n';
        return oss.str();
    }

    DeterminismHash ComputeDeterminismHash(const Editor::GlobalState& gs) {
        DeterminismHash result{};
        result.roads_hash = HashSortedByID(gs.roads, HashRoad);
        result.districts_hash = HashSortedByID(gs.districts, HashDistrict);
        result.lots_hash = HashSortedByID(gs.lots, HashLot);
        result.buildings_hash = HashSortedByID(gs.buildings.getData(), HashBuilding);
        result.tensor_field_hash = HashTensorLayer(gs);
        return result;
    }

    bool SaveBaselineHash(const DeterminismHash& hash, const std::string& filepath) {
        std::ofstream out(filepath, std::ios::trunc);
        if (!out) {
            return false;
        }

        out << "# RogueCities Determinism Baseline\n";
        out << "# Format: key=0x<16-hex-digits>\n";
        out << hash.to_string();
        return true;
    }

    bool ValidateAgainstBaseline(const DeterminismHash& hash, const std::string& filepath) {
        std::ifstream in(filepath);
        if (!in) {
            return false;
        }

        if (const auto parsed = ParseBaseline(in); parsed.has_value()) {
            return hash == *parsed;
        }

        in.clear();
        in.seekg(0, std::ios::beg);
        std::stringstream buffer;
        buffer << in.rdbuf();
        return Trim(buffer.str()) == Trim(hash.to_string());
    }

} // namespace RogueCity::Core::Validation
