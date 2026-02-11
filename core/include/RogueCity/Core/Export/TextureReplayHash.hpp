#pragma once

#include "RogueCity/Core/Data/TextureSpace.hpp"

#include <cstdint>
#include <string>

namespace RogueCity::Core::Export {

    struct TextureReplayDigest {
        uint64_t height{ 0 };
        uint64_t material{ 0 };
        uint64_t zone{ 0 };
        uint64_t tensor{ 0 };
        uint64_t distance{ 0 };
        uint64_t combined{ 0 };
    };

    class TextureReplayHash {
    public:
        [[nodiscard]] static TextureReplayDigest compute(const Data::TextureSpace& texture_space);
        [[nodiscard]] static std::string toHex(uint64_t hash);
        static bool writeManifest(
            const Data::TextureSpace& texture_space,
            const std::string& output_path);
    };

} // namespace RogueCity::Core::Export
