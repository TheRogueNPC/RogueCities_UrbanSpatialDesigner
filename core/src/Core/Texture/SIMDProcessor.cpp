/**
 * @file SIMDProcessor.cpp
 * @brief Implements SIMD-accelerated and scalar image processing routines for RogueCity textures.
 *
 * This file provides functions for CPU feature detection and optimized image processing operations,
 * including box blur, downsampling, and float-to-uint8 conversion. SIMD (AVX2) acceleration is used
 * where available, with scalar fallbacks for compatibility.
 *
 * Key Functions:
 * - DetectCPUFeatures(): Detects available SIMD features (SSE2, AVX, AVX2) at runtime.
 * - BoxBlur3x3Scalar(): Performs a 3x3 box blur using scalar operations.
 * - BoxBlur3x3AVX2(): Performs a 3x3 box blur using AVX2 SIMD instructions (if supported).
 * - SIMDProcessor::boxBlur3x3(): Applies a 3x3 box blur, choosing SIMD or scalar based on CPU features.
 * - SIMDProcessor::downsample2x(): Downsamples a texture by a factor of 2 using averaging.
 * - SIMDProcessor::floatToUint8(): Converts an array of floats to uint8_t, normalizing to [0, 255] and using SIMD if available.
 *
 * SIMDProcessor::CPUFeatures:
 *   - sse2: Indicates SSE2 support.
 *   - avx: Indicates AVX support.
 *   - avx2: Indicates AVX2 support.
 *
 * Platform Support:
 * - Windows (MSVC): Uses __cpuid and _xgetbv for feature detection.
 * - Linux/macOS (GCC/Clang): Uses __get_cpuid and inline assembly for feature detection.
 *
 * Namespace: RogueCity::Core::Texture
 */
 
#include "RogueCity/Core/Texture/SIMDProcessor.hpp"

#include <algorithm>
#include <cmath>
#include <limits>

#if defined(_MSC_VER) && (defined(_M_IX86) || defined(_M_X64))
#include <intrin.h>
#elif (defined(__i386__) || defined(__x86_64__)) && (defined(__GNUC__) || defined(__clang__))
#include <cpuid.h>
#endif

#if defined(ROGUECITY_ENABLE_SIMD_AVX2) && (defined(__AVX2__) || defined(_M_AVX2))
#include <immintrin.h>
#define ROGUECITY_SIMD_AVX2_COMPILED 1
#endif

namespace RogueCity::Core::Texture {

    namespace {
        [[nodiscard]] SIMDProcessor::CPUFeatures DetectCPUFeatures() {
            SIMDProcessor::CPUFeatures features{};

#if defined(_MSC_VER) && (defined(_M_IX86) || defined(_M_X64))
            int regs[4]{ 0, 0, 0, 0 };
            __cpuid(regs, 1);
            features.sse2 = (regs[3] & (1 << 26)) != 0;

            const bool osxsave = (regs[2] & (1 << 27)) != 0;
            const bool avx_hw = (regs[2] & (1 << 28)) != 0;
            if (osxsave && avx_hw) {
                const unsigned long long xcr0 = _xgetbv(0);
                features.avx = (xcr0 & 0x6ull) == 0x6ull;
            }

            __cpuidex(regs, 7, 0);
            features.avx2 = features.avx && ((regs[1] & (1 << 5)) != 0);
#elif (defined(__i386__) || defined(__x86_64__)) && (defined(__GNUC__) || defined(__clang__))
            unsigned int eax = 0;
            unsigned int ebx = 0;
            unsigned int ecx = 0;
            unsigned int edx = 0;

            if (__get_cpuid(1, &eax, &ebx, &ecx, &edx) != 0) {
                features.sse2 = (edx & bit_SSE2) != 0u;

                const bool osxsave = (ecx & bit_OSXSAVE) != 0u;
                const bool avx_hw = (ecx & bit_AVX) != 0u;
                if (osxsave && avx_hw) {
                    unsigned int xcr0_lo = 0u;
                    unsigned int xcr0_hi = 0u;
                    __asm__ __volatile__(
                        "xgetbv"
                        : "=a"(xcr0_lo), "=d"(xcr0_hi)
                        : "c"(0u));
                    const unsigned long long xcr0 =
                        (static_cast<unsigned long long>(xcr0_hi) << 32u) |
                        static_cast<unsigned long long>(xcr0_lo);
                    features.avx = (xcr0 & 0x6ull) == 0x6ull;
                }

                if (features.avx && (__get_cpuid_count(7, 0, &eax, &ebx, &ecx, &edx) != 0)) {
                    features.avx2 = (ebx & bit_AVX2) != 0u;
                }
            }
#endif

            return features;
        }

        void BoxBlur3x3Scalar(const Data::Texture2D<float>& source, Data::Texture2D<float>& destination) {
            const int width = source.width();
            const int height = source.height();
            destination.resize(width, height, 0.0f);

            if (source.empty()) {
                return;
            }

            for (int y = 0; y < height; ++y) {
                for (int x = 0; x < width; ++x) {
                    float sum = 0.0f;
                    for (int ky = -1; ky <= 1; ++ky) {
                        const int sy = std::clamp(y + ky, 0, height - 1);
                        for (int kx = -1; kx <= 1; ++kx) {
                            const int sx = std::clamp(x + kx, 0, width - 1);
                            sum += source.at(sx, sy);
                        }
                    }
                    destination.at(x, y) = sum / 9.0f;
                }
            }
        }

#if defined(ROGUECITY_SIMD_AVX2_COMPILED)
        void BoxBlur3x3AVX2(const Data::Texture2D<float>& source, Data::Texture2D<float>& destination) {
            BoxBlur3x3Scalar(source, destination);

            const int width = source.width();
            const int height = source.height();
            if (width < 10 || height < 3) {
                return;
            }

            const float* src = source.data().data();
            float* dst = destination.data().data();
            const __m256 inv_nine = _mm256_set1_ps(1.0f / 9.0f);

            for (int y = 1; y < height - 1; ++y) {
                int x = 1;
                for (; x <= width - 9; x += 8) {
                    const float* row0 = src + static_cast<size_t>(y - 1) * static_cast<size_t>(width) + static_cast<size_t>(x - 1);
                    const float* row1 = src + static_cast<size_t>(y) * static_cast<size_t>(width) + static_cast<size_t>(x - 1);
                    const float* row2 = src + static_cast<size_t>(y + 1) * static_cast<size_t>(width) + static_cast<size_t>(x - 1);

                    __m256 sum = _mm256_setzero_ps();
                    sum = _mm256_add_ps(sum, _mm256_loadu_ps(row0));
                    sum = _mm256_add_ps(sum, _mm256_loadu_ps(row0 + 1));
                    sum = _mm256_add_ps(sum, _mm256_loadu_ps(row0 + 2));
                    sum = _mm256_add_ps(sum, _mm256_loadu_ps(row1));
                    sum = _mm256_add_ps(sum, _mm256_loadu_ps(row1 + 1));
                    sum = _mm256_add_ps(sum, _mm256_loadu_ps(row1 + 2));
                    sum = _mm256_add_ps(sum, _mm256_loadu_ps(row2));
                    sum = _mm256_add_ps(sum, _mm256_loadu_ps(row2 + 1));
                    sum = _mm256_add_ps(sum, _mm256_loadu_ps(row2 + 2));

                    const __m256 result = _mm256_mul_ps(sum, inv_nine);
                    _mm256_storeu_ps(dst + static_cast<size_t>(y) * static_cast<size_t>(width) + static_cast<size_t>(x), result);
                }
            }
        }
#endif
    } // namespace

    const SIMDProcessor::CPUFeatures& SIMDProcessor::cpuFeatures() {
        static const CPUFeatures features = DetectCPUFeatures();
        return features;
    }

    bool SIMDProcessor::hasAVX2() {
        return cpuFeatures().avx2;
    }

    void SIMDProcessor::boxBlur3x3(const Data::Texture2D<float>& source, Data::Texture2D<float>& destination) {
        if (source.empty()) {
            destination.resize(source.width(), source.height(), 0.0f);
            return;
        }

#if defined(ROGUECITY_SIMD_AVX2_COMPILED)
        if (hasAVX2()) {
            BoxBlur3x3AVX2(source, destination);
            return;
        }
#endif

        BoxBlur3x3Scalar(source, destination);
    }

    void SIMDProcessor::downsample2x(const Data::Texture2D<float>& source, Data::Texture2D<float>& destination) {
        if (source.empty()) {
            destination.resize(0, 0, 0.0f);
            return;
        }

        const int out_w = std::max(1, source.width() / 2);
        const int out_h = std::max(1, source.height() / 2);
        destination.resize(out_w, out_h, 0.0f);

        for (int y = 0; y < out_h; ++y) {
            const int sy = y * 2;
            for (int x = 0; x < out_w; ++x) {
                const int sx = x * 2;
                const int sx1 = std::min(sx + 1, source.width() - 1);
                const int sy1 = std::min(sy + 1, source.height() - 1);
                const float sum =
                    source.at(sx, sy) +
                    source.at(sx1, sy) +
                    source.at(sx, sy1) +
                    source.at(sx1, sy1);
                destination.at(x, y) = sum * 0.25f;
            }
        }
    }

    void SIMDProcessor::floatToUint8(
        const float* source,
        uint8_t* destination,
        size_t count,
        float min_value,
        float max_value) {
        if (source == nullptr || destination == nullptr || count == 0u) {
            return;
        }

        const float range = max_value - min_value;
        if (range <= std::numeric_limits<float>::epsilon()) {
            std::fill(destination, destination + count, static_cast<uint8_t>(0u));
            return;
        }

        const float inv_range = 255.0f / range;

#if defined(ROGUECITY_SIMD_AVX2_COMPILED)
        if (hasAVX2()) {
            const __m256 min_v = _mm256_set1_ps(min_value);
            const __m256 scale_v = _mm256_set1_ps(inv_range);
            const __m256 zero_v = _mm256_setzero_ps();
            const __m256 max_v = _mm256_set1_ps(255.0f);

            size_t i = 0u;
            for (; i + 8u <= count; i += 8u) {
                __m256 v = _mm256_loadu_ps(source + i);
                v = _mm256_sub_ps(v, min_v);
                v = _mm256_mul_ps(v, scale_v);
                v = _mm256_min_ps(max_v, _mm256_max_ps(zero_v, v));

                alignas(32) float tmp[8];
                _mm256_store_ps(tmp, v);
                for (int k = 0; k < 8; ++k) {
                    const int rounded = static_cast<int>(std::lround(tmp[k]));
                    destination[i + static_cast<size_t>(k)] =
                        static_cast<uint8_t>(std::clamp(rounded, 0, 255));
                }
            }

            for (; i < count; ++i) {
                const float normalized = (source[i] - min_value) * inv_range;
                const int rounded = static_cast<int>(std::lround(std::clamp(normalized, 0.0f, 255.0f)));
                destination[i] = static_cast<uint8_t>(rounded);
            }
            return;
        }
#endif

        for (size_t i = 0u; i < count; ++i) {
            const float normalized = (source[i] - min_value) * inv_range;
            const int rounded = static_cast<int>(std::lround(std::clamp(normalized, 0.0f, 255.0f)));
            destination[i] = static_cast<uint8_t>(rounded);
        }
    }

} // namespace RogueCity::Core::Texture

