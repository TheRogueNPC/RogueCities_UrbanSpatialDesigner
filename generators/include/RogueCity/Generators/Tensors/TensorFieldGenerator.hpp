#pragma once
#include "RogueCity/Core/Types.hpp"
#include "RogueCity/Core/Data/TextureSpace.hpp"
#include "RogueCity/Generators/Tensors/BasisFields.hpp"
#include <vector>
#include <memory>

namespace RogueCity::Generators {

    using namespace Core;

    /// Generates tensor field from axioms for road network generation
    class TensorFieldGenerator {
    public:
        /// Grid configuration
        struct Config {
            int width{ 200 };          // Grid width in cells
            int height{ 200 };         // Grid height in cells
            double cell_size{ 10.0 };  // Cell size in meters (10m = 200x200 = 2km x 2km)
        };

        /// Constructor
        TensorFieldGenerator();
        explicit TensorFieldGenerator(const Config& config);

        /// Add basis fields from axioms
        void addOrganicField(const Vec2& center, double radius, double theta, float curviness, double decay = 2.0);
        void addRadialField(const Vec2& center, double radius, double decay = 2.0);
        void addRadialField(const Vec2& center, double radius, int spokes, double decay = 2.0);
        void addGridField(const Vec2& center, double radius, double theta, double decay = 2.0);
        void addHexagonalField(const Vec2& center, double radius, double theta, double decay = 2.0);
        void addStemField(const Vec2& center, double radius, double theta, float branch_angle, double decay = 2.0);
        void addLooseGridField(const Vec2& center, double radius, double theta, float jitter, double decay = 2.0);
        void addSuburbanField(const Vec2& center, double radius, float loop_strength, double decay = 2.0);
        void addSuperblockField(const Vec2& center, double radius, double theta, float block_size, double decay = 2.0);
        void addLinearField(const Vec2& center, double radius, double theta, double decay = 2.0);
        void addGridCorrective(const Vec2& center, double radius, double theta, double decay = 3.0);

        /// Sample tensor at world position (interpolated from grid)
        [[nodiscard]] Tensor2D sampleTensor(const Vec2& world_pos) const;

        /// Generate field by evaluating all basis fields at grid points
        void generateField();

        /// Write major-direction tensor vectors into TextureSpace tensor layer.
        void writeToTextureSpace(Core::Data::TextureSpace& texture_space) const;

        /// Bind an existing TextureSpace tensor layer as runtime sampling substrate.
        void bindTextureSpace(const Core::Data::TextureSpace* texture_space) { texture_space_ = texture_space; }
        void clearTextureSpaceBinding() { texture_space_ = nullptr; }
        [[nodiscard]] bool usesTextureSpace() const { return texture_space_ != nullptr; }

        /// Clear all basis fields
        void clear();

        /// Getters
        [[nodiscard]] int getWidth() const { return config_.width; }
        [[nodiscard]] int getHeight() const { return config_.height; }
        [[nodiscard]] double getCellSize() const { return config_.cell_size; }
        [[nodiscard]] bool isGenerated() const { return field_generated_; }

    private:
        Config config_;
        std::vector<Tensor2D> grid_;  // width * height tensor grid
        std::vector<std::unique_ptr<BasisField>> basis_fields_;
        bool field_generated_{ false };
        const Core::Data::TextureSpace* texture_space_{ nullptr };

        /// Convert world position to grid indices
        [[nodiscard]] bool worldToGrid(const Vec2& world_pos, int& gx, int& gy) const;

        /// Get tensor at grid cell (no interpolation)
        [[nodiscard]] Tensor2D getGridTensor(int gx, int gy) const;

        /// Bilinear interpolation of tensors
        [[nodiscard]] Tensor2D interpolateTensor(const Vec2& world_pos) const;
    };

} // namespace RogueCity::Generators
