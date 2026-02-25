/**
 * @file TextureReplayHash.cpp
 * @brief Implements hashing and digest computation for texture layers in a TextureSpace.
 *
 * This file provides functions to compute FNV-1a hashes for various texture layers (float, uint8_t, Vec2)
 * and combines them into a digest for replay and export purposes. The digest includes hashes for height,
 * material, zone, tensor, and distance layers, as well as a combined hash incorporating texture space metadata.
 *
 * Key Functions:
 * - hashBytes: Applies FNV-1a hashing to arbitrary byte data.
 * - normalizedFloatBits: Normalizes float values and returns their bit representation.
 * - hashFloatTexture: Hashes a Texture2D<float> layer.
 * - hashU8Texture: Hashes a Texture2D<uint8_t> layer.
 * - hashVec2Texture: Hashes a Texture2D<Vec2> layer.
 * - combineDigests: Combines layer hashes and texture space metadata into a single digest.
 * - TextureReplayHash::compute: Computes the digest for a given TextureSpace.
 * - TextureReplayHash::toHex: Converts a hash value to a hexadecimal string.
 * - TextureReplayHash::writeManifest: Writes the digest and metadata to a JSON manifest file.
 *
 * @namespace RogueCity::Core::Export
 */
 
#include "RogueCity/Core/Export/TextureReplayHash.hpp"

#include <cmath>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <limits>
#include <sstream>

namespace RogueCity::Core::Export {

    namespace {
        constexpr uint64_t kFnvOffset = 1469598103934665603ull;
        constexpr uint64_t kFnvPrime = 1099511628211ull;

        void hashBytes(const void* data, size_t size, uint64_t& hash) {
            const uint8_t* bytes = static_cast<const uint8_t*>(data);
            for (size_t i = 0; i < size; ++i) {
                hash ^= static_cast<uint64_t>(bytes[i]);
                hash *= kFnvPrime;
            }
        }

        [[nodiscard]] uint32_t normalizedFloatBits(float value) {
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

        [[nodiscard]] uint64_t hashFloatTexture(const Data::Texture2D<float>& texture) {
            uint64_t hash = kFnvOffset;
            const int w = texture.width();
            const int h = texture.height();
            hashBytes(&w, sizeof(w), hash);
            hashBytes(&h, sizeof(h), hash);
            for (float v : texture.data()) {
                const uint32_t bits = normalizedFloatBits(v);
                hashBytes(&bits, sizeof(bits), hash);
            }
            return hash;
        }

        [[nodiscard]] uint64_t hashU8Texture(const Data::Texture2D<uint8_t>& texture) {
            uint64_t hash = kFnvOffset;
            const int w = texture.width();
            const int h = texture.height();
            hashBytes(&w, sizeof(w), hash);
            hashBytes(&h, sizeof(h), hash);
            if (!texture.data().empty()) {
                hashBytes(texture.data().data(), texture.data().size(), hash);
            }
            return hash;
        }

        [[nodiscard]] uint64_t hashVec2Texture(const Data::Texture2D<Vec2>& texture) {
            uint64_t hash = kFnvOffset;
            const int w = texture.width();
            const int h = texture.height();
            hashBytes(&w, sizeof(w), hash);
            hashBytes(&h, sizeof(h), hash);
            for (const Vec2& v : texture.data()) {
                const float x = static_cast<float>(v.x);
                const float y = static_cast<float>(v.y);
                const uint32_t x_bits = normalizedFloatBits(x);
                const uint32_t y_bits = normalizedFloatBits(y);
                hashBytes(&x_bits, sizeof(x_bits), hash);
                hashBytes(&y_bits, sizeof(y_bits), hash);
            }
            return hash;
        }

        [[nodiscard]] uint64_t combineDigests(
            const Data::TextureSpace& texture_space,
            const TextureReplayDigest& digest) {
            uint64_t hash = kFnvOffset;
            const int resolution = texture_space.resolution();
            const Bounds bounds = texture_space.bounds();
            const float min_x = static_cast<float>(bounds.min.x);
            const float min_y = static_cast<float>(bounds.min.y);
            const float max_x = static_cast<float>(bounds.max.x);
            const float max_y = static_cast<float>(bounds.max.y);

            hashBytes(&resolution, sizeof(resolution), hash);
            const uint32_t min_x_bits = normalizedFloatBits(min_x);
            const uint32_t min_y_bits = normalizedFloatBits(min_y);
            const uint32_t max_x_bits = normalizedFloatBits(max_x);
            const uint32_t max_y_bits = normalizedFloatBits(max_y);
            hashBytes(&min_x_bits, sizeof(min_x_bits), hash);
            hashBytes(&min_y_bits, sizeof(min_y_bits), hash);
            hashBytes(&max_x_bits, sizeof(max_x_bits), hash);
            hashBytes(&max_y_bits, sizeof(max_y_bits), hash);

            hashBytes(&digest.height, sizeof(digest.height), hash);
            hashBytes(&digest.material, sizeof(digest.material), hash);
            hashBytes(&digest.zone, sizeof(digest.zone), hash);
            hashBytes(&digest.tensor, sizeof(digest.tensor), hash);
            hashBytes(&digest.distance, sizeof(digest.distance), hash);
            return hash;
        }
    } // namespace

    TextureReplayDigest TextureReplayHash::compute(const Data::TextureSpace& texture_space) {
        TextureReplayDigest digest{};
        digest.height = hashFloatTexture(texture_space.heightLayer());
        digest.material = hashU8Texture(texture_space.materialLayer());
        digest.zone = hashU8Texture(texture_space.zoneLayer());
        digest.tensor = hashVec2Texture(texture_space.tensorLayer());
        digest.distance = hashFloatTexture(texture_space.distanceLayer());
        digest.combined = combineDigests(texture_space, digest);
        return digest;
    }

    std::string TextureReplayHash::toHex(uint64_t hash) {
        std::ostringstream oss;
        oss << "0x" << std::hex << std::setw(16) << std::setfill('0') << hash;
        return oss.str();
    }

    bool TextureReplayHash::writeManifest(
        const Data::TextureSpace& texture_space,
        const std::string& output_path) {
        const TextureReplayDigest digest = compute(texture_space);
        std::ofstream out(output_path, std::ios::trunc);
        if (!out) {
            return false;
        }

        const Bounds bounds = texture_space.bounds();
        out << "{\n";
        out << "  \"texture_space\": {\n";
        out << "    \"resolution\": " << texture_space.resolution() << ",\n";
        out << "    \"bounds\": {\n";
        out << "      \"min\": [" << bounds.min.x << ", " << bounds.min.y << "],\n";
        out << "      \"max\": [" << bounds.max.x << ", " << bounds.max.y << "]\n";
        out << "    }\n";
        out << "  },\n";
        out << "  \"hashes\": {\n";
        out << "    \"height\": \"" << toHex(digest.height) << "\",\n";
        out << "    \"material\": \"" << toHex(digest.material) << "\",\n";
        out << "    \"zone\": \"" << toHex(digest.zone) << "\",\n";
        out << "    \"tensor\": \"" << toHex(digest.tensor) << "\",\n";
        out << "    \"distance\": \"" << toHex(digest.distance) << "\",\n";
        out << "    \"combined\": \"" << toHex(digest.combined) << "\"\n";
        out << "  }\n";
        out << "}\n";
        return true;
    }

} // namespace RogueCity::Core::Export
