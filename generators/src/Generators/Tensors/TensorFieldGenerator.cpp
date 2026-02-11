#define _USE_MATH_DEFINES
#include "RogueCity/Generators/Tensors/TensorFieldGenerator.hpp"
#include "RogueCity/Core/Util/RogueWorker.hpp"
#include <algorithm>
#include <cassert>
#include <cmath>
#include <iostream>
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
        last_sample_used_texture_ = false;
        last_sample_used_fallback_ = false;

        if (texture_space_ != nullptr) {
            const auto& tensor_layer = texture_space_->tensorLayer();
            if (!tensor_layer.empty() && texture_space_->coordinateSystem().isInBounds(world_pos)) {
                const Vec2 uv = texture_space_->coordinateSystem().worldToUV(world_pos);
                const Vec2 sampled_dir = tensor_layer.sampleBilinearTyped<Vec2>(uv);
                if (sampled_dir.lengthSquared() > 1e-8) {
                    last_sample_used_texture_ = true;
                    return Tensor2D::fromVector(sampled_dir);
                }
                markTextureFallback("sampled texture direction is zero");
            }

            if (tensor_layer.empty()) {
                markTextureFallback("tensor layer is empty");
            }
        }

        last_sample_used_fallback_ = true;
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

    void TensorFieldGenerator::writeToTextureSpace(Core::Data::TextureSpace& texture_space) const {
        auto& tensor_layer = texture_space.tensorLayer();
        if (tensor_layer.empty()) {
            return;
        }

        const auto& coords = texture_space.coordinateSystem();
        for (int y = 0; y < tensor_layer.height(); ++y) {
            for (int x = 0; x < tensor_layer.width(); ++x) {
                const Vec2 world = coords.pixelToWorld({ x, y });
                Tensor2D tensor = Tensor2D::zero();

                if (field_generated_) {
                    tensor = interpolateTensor(world);
                } else {
                    double total_weight = 0.0;
                    for (const auto& field : basis_fields_) {
                        const double weight = field->getWeight(world);
                        if (weight <= 1e-6) {
                            continue;
                        }
                        Tensor2D sample = field->sample(world);
                        sample.scale(weight);
                        tensor.add(sample, true);
                        total_weight += weight;
                    }
                    if (total_weight > 1e-6) {
                        tensor.scale(1.0 / total_weight);
                    }
                }

                tensor_layer.at(x, y) = tensor.majorEigenvector();
            }
        }
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
        if (config_.width <= 0 || config_.height <= 0 || grid_.empty()) {
            return Tensor2D::zero();
        }

        // The grid stores tensor samples at cell centers ((i + 0.5) * cell_size).
        // Clamp to grid extent to avoid edge fade when sampling outside bounds.
        const double max_x = static_cast<double>(config_.width - 1);
        const double max_y = static_cast<double>(config_.height - 1);
        double fx = (world_pos.x / config_.cell_size) - 0.5;
        double fy = (world_pos.y / config_.cell_size) - 0.5;
        fx = std::clamp(fx, 0.0, max_x);
        fy = std::clamp(fy, 0.0, max_y);

        int gx0 = static_cast<int>(std::floor(fx));
        int gy0 = static_cast<int>(std::floor(fy));
        int gx1 = std::min(gx0 + 1, config_.width - 1);
        int gy1 = std::min(gy0 + 1, config_.height - 1);

        double tx = fx - gx0;
        double ty = fy - gy0;

        // Get 4 corner tensors
        Tensor2D t00 = getGridTensor(gx0, gy0);
        Tensor2D t10 = getGridTensor(gx1, gy0);
        Tensor2D t01 = getGridTensor(gx0, gy1);
        Tensor2D t11 = getGridTensor(gx1, gy1);

        // Bilinear blend (scale-then-add avoids interpolation fade artifacts).
        Tensor2D result = Tensor2D::zero();
        Tensor2D w00 = t00;
        w00.scale((1.0 - tx) * (1.0 - ty));
        result.add(w00, true);

        Tensor2D w10 = t10;
        w10.scale(tx * (1.0 - ty));
        result.add(w10, true);

        Tensor2D w01 = t01;
        w01.scale((1.0 - tx) * ty);
        result.add(w01, true);

        Tensor2D w11 = t11;
        w11.scale(tx * ty);
        result.add(w11, true);

        return result;
    }

    void TensorFieldGenerator::clear() {
        basis_fields_.clear();
        std::fill(grid_.begin(), grid_.end(), Tensor2D::zero());
        field_generated_ = false;
        last_sample_used_texture_ = false;
        last_sample_used_fallback_ = false;
        texture_fallback_warned_ = false;
    }

    void TensorFieldGenerator::markTextureFallback(const char* reason) const {
#ifndef NDEBUG
        if (!texture_fallback_warned_) {
            std::cerr << "[TensorFieldGenerator] Texture-bound sampling fallback: " << reason << std::endl;
            texture_fallback_warned_ = true;
        }
#else
        (void)reason;
#endif
    }

} // namespace RogueCity::Generators
