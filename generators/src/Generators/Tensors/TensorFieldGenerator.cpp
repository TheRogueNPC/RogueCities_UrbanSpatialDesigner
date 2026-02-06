#define _USE_MATH_DEFINES
#include "RogueCity/Generators/Tensors/TensorFieldGenerator.hpp"
#include "RogueCity/Core/Util/RogueWorker.hpp"
#include <algorithm>
#include <cmath>
#include <thread>

namespace RogueCity::Generators {

    TensorFieldGenerator::TensorFieldGenerator()
        : TensorFieldGenerator(Config{}) {}

    TensorFieldGenerator::TensorFieldGenerator(const Config& config)
        : config_(config) {
        // Allocate grid storage
        grid_.resize(config_.width * config_.height, Tensor2D::zero());
    }

    void TensorFieldGenerator::addOrganicField(const Vec2& center, double radius, double theta, float curviness, double decay) {
        basis_fields_.push_back(
            std::make_unique<OrganicField>(center, radius, theta, curviness, decay)
        );
        field_generated_ = false;
    }

    void TensorFieldGenerator::addRadialField(const Vec2& center, double radius, double decay) {
        basis_fields_.push_back(
            std::make_unique<RadialField>(center, radius, decay)
        );
        field_generated_ = false;  // Invalidate cached field
    }

    void TensorFieldGenerator::addRadialField(const Vec2& center, double radius, int spokes, double decay) {
        basis_fields_.push_back(
            std::make_unique<RadialHubSpokeField>(center, radius, spokes, decay)
        );
        field_generated_ = false;
    }

    void TensorFieldGenerator::addGridField(const Vec2& center, double radius, double theta, double decay) {
        basis_fields_.push_back(
            std::make_unique<GridField>(center, radius, theta, decay)
        );
        field_generated_ = false;
    }

    void TensorFieldGenerator::addHexagonalField(const Vec2& center, double radius, double theta, double decay) {
        basis_fields_.push_back(
            std::make_unique<HexagonalField>(center, radius, theta, 120.0, decay)
        );
        field_generated_ = false;
    }

    void TensorFieldGenerator::addStemField(const Vec2& center, double radius, double theta, float branch_angle, double decay) {
        basis_fields_.push_back(
            std::make_unique<StemField>(center, radius, theta, branch_angle, decay)
        );
        field_generated_ = false;
    }

    void TensorFieldGenerator::addLooseGridField(const Vec2& center, double radius, double theta, float jitter, double decay) {
        basis_fields_.push_back(
            std::make_unique<LooseGridField>(center, radius, theta, jitter, decay)
        );
        field_generated_ = false;
    }

    void TensorFieldGenerator::addSuburbanField(const Vec2& center, double radius, float loop_strength, double decay) {
        basis_fields_.push_back(
            std::make_unique<SuburbanField>(center, radius, loop_strength, decay)
        );
        field_generated_ = false;
    }

    void TensorFieldGenerator::addSuperblockField(const Vec2& center, double radius, double theta, float block_size, double decay) {
        basis_fields_.push_back(
            std::make_unique<SuperblockField>(center, radius, theta, block_size, decay)
        );
        field_generated_ = false;
    }

    void TensorFieldGenerator::addLinearField(const Vec2& center, double radius, double theta, double decay) {
        basis_fields_.push_back(
            std::make_unique<LinearField>(center, radius, theta, decay)
        );
        field_generated_ = false;
    }

    void TensorFieldGenerator::addGridCorrective(const Vec2& center, double radius, double theta, double decay) {
        basis_fields_.push_back(
            std::make_unique<GridCorrectiveField>(center, radius, theta, decay)
        );
        field_generated_ = false;
    }

    void TensorFieldGenerator::generateField() {
        const uint32_t thread_count = std::max(1u, std::thread::hardware_concurrency());
        const int max_group_size = std::max(1, std::min(16, config_.height)); // Cap chunking to 16 groups per generator spec.
        const uint32_t group_size = std::min(thread_count, static_cast<uint32_t>(max_group_size));
        const int chunk_size = (config_.height + static_cast<int>(group_size) - 1) / static_cast<int>(group_size);

        // RogueWorker resides in the Rowk namespace defined by RogueWorker.hpp.
        Rowk::RogueWorker worker(thread_count);
        Rowk::WorkGroup work = worker.execute(
            [&](uint32_t id, uint32_t) {
                const int start_row = static_cast<int>(id) * chunk_size;
                const int end_row = std::min(config_.height, start_row + chunk_size);
                for (int gy = start_row; gy < end_row; ++gy) {
                    for (int gx = 0; gx < config_.width; ++gx) {
                        // Convert grid indices to world position (cell centers)
                        Vec2 world_pos(
                            (gx + 0.5) * config_.cell_size,
                            (gy + 0.5) * config_.cell_size
                        );

                        // Blend all basis fields
                        Tensor2D result = Tensor2D::zero();
                        double total_weight = 0.0;

                        for (const auto& field : basis_fields_) {
                            double weight = field->getWeight(world_pos);
                            if (weight > 1e-6) {
                                Tensor2D tensor = field->sample(world_pos);
                                tensor.scale(weight);
                                result.add(tensor, true);  // Smooth blending
                                total_weight += weight;
                            }
                        }

                        // Normalize by total weight
                        if (total_weight > 1e-6) {
                            result.scale(1.0 / total_weight);
                        }

                        // Store in grid
                        int idx = gy * config_.width + gx;
                        grid_[idx] = result;
                    }
                }
            },
            group_size
        );

        work.waitExecutionDone();

        field_generated_ = true;
    }

    Tensor2D TensorFieldGenerator::sampleTensor(const Vec2& world_pos) const {
        if (!field_generated_) {
            // Fallback: Real-time sampling (slower but works if generateField() not called)
            Tensor2D result = Tensor2D::zero();
            double total_weight = 0.0;

            for (const auto& field : basis_fields_) {
                double weight = field->getWeight(world_pos);
                if (weight > 1e-6) {
                    Tensor2D tensor = field->sample(world_pos);
                    tensor.scale(weight);
                    result.add(tensor, true);
                    total_weight += weight;
                }
            }

            if (total_weight > 1e-6) {
                result.scale(1.0 / total_weight);
            }

            return result;
        }

        // Fast path: Interpolate from pre-generated grid
        return interpolateTensor(world_pos);
    }

    bool TensorFieldGenerator::worldToGrid(const Vec2& world_pos, int& gx, int& gy) const {
        gx = static_cast<int>(world_pos.x / config_.cell_size);
        gy = static_cast<int>(world_pos.y / config_.cell_size);
        return (gx >= 0 && gx < config_.width && gy >= 0 && gy < config_.height);
    }

    Tensor2D TensorFieldGenerator::getGridTensor(int gx, int gy) const {
        if (gx < 0 || gx >= config_.width || gy < 0 || gy >= config_.height) {
            return Tensor2D::zero();
        }
        int idx = gy * config_.width + gx;
        return grid_[idx];
    }

    Tensor2D TensorFieldGenerator::interpolateTensor(const Vec2& world_pos) const {
        // Bilinear interpolation of tensors at grid points
        double fx = world_pos.x / config_.cell_size;
        double fy = world_pos.y / config_.cell_size;

        int gx0 = static_cast<int>(std::floor(fx));
        int gy0 = static_cast<int>(std::floor(fy));
        int gx1 = gx0 + 1;
        int gy1 = gy0 + 1;

        double tx = fx - gx0;
        double ty = fy - gy0;

        // Get 4 corner tensors
        Tensor2D t00 = getGridTensor(gx0, gy0);
        Tensor2D t10 = getGridTensor(gx1, gy0);
        Tensor2D t01 = getGridTensor(gx0, gy1);
        Tensor2D t11 = getGridTensor(gx1, gy1);

        // Bilinear blend (simple average for tensors)
        Tensor2D result = Tensor2D::zero();
        result.add(t00, true);
        result.scale((1.0 - tx) * (1.0 - ty));

        Tensor2D temp = t10;
        temp.scale(tx * (1.0 - ty));
        result.add(temp, true);

        temp = t01;
        temp.scale((1.0 - tx) * ty);
        result.add(temp, true);

        temp = t11;
        temp.scale(tx * ty);
        result.add(temp, true);

        return result;
    }

    void TensorFieldGenerator::clear() {
        basis_fields_.clear();
        std::fill(grid_.begin(), grid_.end(), Tensor2D::zero());
        field_generated_ = false;
    }

} // namespace RogueCity::Generators
