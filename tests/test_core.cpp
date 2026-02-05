#include "RogueCity/Core/Types.hpp"
#include "RogueCity/Core/Util/RogueWorker.hpp"
#include "RogueCity/Core/Util/IndexVector.hpp"
#include "RogueCity/Core/Util/StableIndexVector.hpp"
#include <iostream>
#include <cassert>
#include <chrono>
#include <cmath>
#include <numbers>

using namespace RogueCity::Core;

// ============================================================================
// TEST UTILITIES
// ============================================================================

struct TestResult {
    bool passed{ true };
    std::string name;
    std::string error_message;
    double execution_time_ms{ 0.0 };
};

class TestRunner {
public:
    static void runTest(const std::string& test_name, std::function<void()> test_func) {
        TestResult result;
        result.name = test_name;

        auto start = std::chrono::high_resolution_clock::now();
        try {
            test_func();
            result.passed = true;
        }
        catch (const std::exception& e) {
            result.passed = false;
            result.error_message = e.what();
        }
        auto end = std::chrono::high_resolution_clock::now();
        result.execution_time_ms = std::chrono::duration<double, std::milli>(end - start).count();

        printResult(result);
        results_.push_back(result);
    }

    static void printSummary() {
        std::cout << "\n========================================\n";
        std::cout << "TEST SUMMARY\n";
        std::cout << "========================================\n";
        
        int passed = 0;
        int failed = 0;
        double total_time = 0.0;

        for (const auto& result : results_) {
            if (result.passed) passed++;
            else failed++;
            total_time += result.execution_time_ms;
        }

        std::cout << "Total tests: " << results_.size() << "\n";
        std::cout << "Passed: " << passed << "\n";
        std::cout << "Failed: " << failed << "\n";
        std::cout << "Total time: " << total_time << " ms\n";
        std::cout << "========================================\n";
    }

private:
    static void printResult(const TestResult& result) {
        if (result.passed) {
            std::cout << "[PASS] " << result.name 
                << " (" << result.execution_time_ms << " ms)\n";
        }
        else {
            std::cout << "[FAIL] " << result.name 
                << " - " << result.error_message 
                << " (" << result.execution_time_ms << " ms)\n";
        }
    }

    static std::vector<TestResult> results_;
};

std::vector<TestResult> TestRunner::results_;

#define TEST(name) TestRunner::runTest(#name, name)

// Assertion macros
#define ASSERT_TRUE(condition) \
    if (!(condition)) throw std::runtime_error("Assertion failed: " #condition)

#define ASSERT_FALSE(condition) \
    if (condition) throw std::runtime_error("Assertion failed: not " #condition)

#define ASSERT_EQUAL(a, b) \
    if ((a) != (b)) throw std::runtime_error("Assertion failed: " #a " != " #b)

#define ASSERT_NEAR(a, b, epsilon) \
    if (std::abs((a) - (b)) > (epsilon)) \
        throw std::runtime_error("Assertion failed: |" #a " - " #b "| > " #epsilon)

// ============================================================================
// VEC2 TESTS (including recent fixes)
// ============================================================================

void test_vec2_construction() {
    Vec2 v1;
    ASSERT_EQUAL(v1.x, 0.0);
    ASSERT_EQUAL(v1.y, 0.0);

    Vec2 v2(3.0, 4.0);
    ASSERT_EQUAL(v2.x, 3.0);
    ASSERT_EQUAL(v2.y, 4.0);
}

void test_vec2_length() {
    Vec2 v(3.0, 4.0);
    ASSERT_NEAR(v.length(), 5.0, 1e-9);
    ASSERT_NEAR(v.lengthSquared(), 25.0, 1e-9);
}

void test_vec2_normalize() {
    Vec2 v(3.0, 4.0);
    v.normalize();
    ASSERT_NEAR(v.length(), 1.0, 1e-9);
    ASSERT_NEAR(v.x, 0.6, 1e-9);
    ASSERT_NEAR(v.y, 0.8, 1e-9);
}

void test_vec2_operators() {
    Vec2 v1(1.0, 2.0);
    Vec2 v2(3.0, 4.0);

    Vec2 sum = v1 + v2;
    ASSERT_EQUAL(sum.x, 4.0);
    ASSERT_EQUAL(sum.y, 6.0);

    Vec2 diff = v2 - v1;
    ASSERT_EQUAL(diff.x, 2.0);
    ASSERT_EQUAL(diff.y, 2.0);

    Vec2 scaled = v1 * 2.0;
    ASSERT_EQUAL(scaled.x, 2.0);
    ASSERT_EQUAL(scaled.y, 4.0);
}

void test_vec2_dot_product() {
    Vec2 v1(1.0, 0.0);
    Vec2 v2(0.0, 1.0);
    ASSERT_NEAR(v1.dot(v2), 0.0, 1e-9);

    Vec2 v3(1.0, 1.0);
    ASSERT_NEAR(v1.dot(v3), 1.0, 1e-9);
}

void test_vec2_cross_product() {
    Vec2 v1(1.0, 0.0);
    Vec2 v2(0.0, 1.0);
    ASSERT_NEAR(v1.cross(v2), 1.0, 1e-9);
}

void test_vec2_angle() {
    Vec2 v1(1.0, 0.0);
    ASSERT_NEAR(v1.angle(), 0.0, 1e-9);

    Vec2 v2(0.0, 1.0);
    ASSERT_NEAR(v2.angle(), std::numbers::pi / 2.0, 1e-9);

    Vec2 v3(-1.0, 0.0);
    ASSERT_NEAR(std::abs(v3.angle()), std::numbers::pi, 1e-9);
}

void test_vec2_lerp_free_function() {
    // Test the free function lerp that was causing compilation errors
    Vec2 v1(0.0, 0.0);
    Vec2 v2(10.0, 10.0);

    Vec2 mid = lerp(v1, v2, 0.5);
    ASSERT_NEAR(mid.x, 5.0, 1e-9);
    ASSERT_NEAR(mid.y, 5.0, 1e-9);

    Vec2 quarter = lerp(v1, v2, 0.25);
    ASSERT_NEAR(quarter.x, 2.5, 1e-9);
    ASSERT_NEAR(quarter.y, 2.5, 1e-9);
}

void test_vec2_distance() {
    Vec2 v1(0.0, 0.0);
    Vec2 v2(3.0, 4.0);
    ASSERT_NEAR(distance(v1, v2), 5.0, 1e-9);
    ASSERT_NEAR(v1.distanceTo(v2), 5.0, 1e-9);
}

// ============================================================================
// TENSOR2D TESTS (including std::numbers::pi usage)
// ============================================================================

void test_tensor2d_fromAngle() {
    // Test that we correctly use std::numbers::pi instead of M_PI
    Tensor2D t1 = Tensor2D::fromAngle(0.0);
    ASSERT_TRUE(t1.r > 0.0); // Should have magnitude

    Tensor2D t2 = Tensor2D::fromAngle(std::numbers::pi / 2.0);
    ASSERT_TRUE(t2.r > 0.0); // Should have magnitude
}

void test_tensor2d_fromVector() {
    Vec2 v(1.0, 0.0);
    Tensor2D t = Tensor2D::fromVector(v);
    ASSERT_TRUE(t.r > 0.0); // Should have magnitude
}

void test_tensor2d_eigenvector() {
    Tensor2D t = Tensor2D::fromAngle(std::numbers::pi / 4.0);
    Vec2 major = t.majorEigenvector();
    ASSERT_NEAR(major.length(), 1.0, 1e-6); // Should be normalized
}

// ============================================================================
// CONSTANT INDEX VECTOR (CIV) TESTS
// ============================================================================

void test_civ_basic_operations() {
    civ::IndexVector<int> vec;

    // Push back
    auto id1 = vec.push_back(42);
    auto id2 = vec.push_back(99);
    auto id3 = vec.push_back(123);

    ASSERT_EQUAL(vec.size(), 3);
    ASSERT_EQUAL(vec[id1], 42);
    ASSERT_EQUAL(vec[id2], 99);
    ASSERT_EQUAL(vec[id3], 123);
}

void test_civ_erase() {
    civ::IndexVector<int> vec;
    auto id1 = vec.push_back(10);
    auto id2 = vec.push_back(20);
    auto id3 = vec.push_back(30);

    vec.erase(id2);
    ASSERT_EQUAL(vec.size(), 2);
    
    // After erase, id1 and id3 should still be valid
    ASSERT_EQUAL(vec[id1], 10);
    ASSERT_EQUAL(vec[id3], 30);
}

void test_civ_reference() {
    civ::IndexVector<int> vec;
    auto id = vec.push_back(777);
    auto ref = vec.createRef(id);

    ASSERT_TRUE(ref);
    ASSERT_EQUAL(*ref, 777);

    *ref = 888;
    ASSERT_EQUAL(vec[id], 888);
}

void test_civ_iteration() {
    civ::IndexVector<int> vec;
    vec.push_back(1);
    vec.push_back(2);
    vec.push_back(3);

    int sum = 0;
    for (int val : vec) {
        sum += val;
    }
    ASSERT_EQUAL(sum, 6);
}

// ============================================================================
// STABLE INDEX VECTOR (SIV) TESTS
// ============================================================================

void test_siv_basic_operations() {
    siv::Vector<int> vec;

    auto id1 = vec.push_back(100);
    auto id2 = vec.push_back(200);
    auto id3 = vec.push_back(300);

    ASSERT_EQUAL(vec.size(), 3);
    ASSERT_EQUAL(vec[id1], 100);
    ASSERT_EQUAL(vec[id2], 200);
    ASSERT_EQUAL(vec[id3], 300);
}

void test_siv_erase() {
    siv::Vector<int> vec;
    auto id1 = vec.push_back(10);
    auto id2 = vec.push_back(20);
    auto id3 = vec.push_back(30);

    vec.erase(id2);
    ASSERT_EQUAL(vec.size(), 2);
}

void test_siv_handle() {
    siv::Vector<int> vec;
    auto id = vec.push_back(555);
    auto handle = vec.createHandle(id);

    ASSERT_TRUE(handle.isValid());
    ASSERT_EQUAL(*handle, 555);

    *handle = 666;
    ASSERT_EQUAL(vec[id], 666);
}

void test_siv_handle_validity() {
    siv::Vector<int> vec;
    auto id = vec.push_back(999);
    auto handle = vec.createHandle(id);

    ASSERT_TRUE(handle.isValid());
    
    vec.erase(id);
    
    // Handle should now be invalid
    ASSERT_FALSE(handle.isValid());
}

// ============================================================================
// ROGUE WORKER TESTS
// ============================================================================

void test_rogue_worker_basic_execution() {
    const uint32_t thread_count = 4;
    Rowk::RogueWorker worker(thread_count);

    std::atomic<int> counter{0};

    auto job = [&counter](uint32_t worker_id, uint32_t worker_count) {
        counter++;
    };

    Rowk::WorkGroup group = worker.execute(job, thread_count);
    group.waitExecutionDone();

    ASSERT_EQUAL(counter.load(), static_cast<int>(thread_count));
}

void test_rogue_worker_parallel_computation() {
    const uint32_t thread_count = 4;
    Rowk::RogueWorker worker(thread_count);

    const size_t vec_size = 10000;
    std::vector<float> v1(vec_size, 1.0f);
    std::vector<float> v2(vec_size, 2.0f);
    std::vector<float> v3(vec_size, 0.0f);

    auto job = [&](uint32_t worker_id, uint32_t worker_count) {
        const uint32_t step = vec_size / worker_count;
        const uint32_t start_index = worker_id * step;
        const uint32_t end_index = (worker_id < worker_count - 1) 
            ? start_index + step 
            : vec_size;

        for (uint32_t i = start_index; i < end_index; ++i) {
            v3[i] = v1[i] + v2[i];
        }
    };

    Rowk::WorkGroup group = worker.execute(job, thread_count);
    group.waitExecutionDone();

    // Verify all elements are computed correctly
    for (size_t i = 0; i < vec_size; ++i) {
        ASSERT_NEAR(v3[i], 3.0f, 1e-6f);
    }
}

void test_rogue_worker_multiple_groups() {
    const uint32_t thread_count = 2;
    Rowk::RogueWorker worker(thread_count);

    std::atomic<int> counter1{0};
    std::atomic<int> counter2{0};

    auto job1 = [&counter1](uint32_t, uint32_t) { counter1++; };
    auto job2 = [&counter2](uint32_t, uint32_t) { counter2++; };

    Rowk::WorkGroup group1 = worker.execute(job1, thread_count);
    Rowk::WorkGroup group2 = worker.execute(job2, thread_count);

    group1.waitExecutionDone();
    group2.waitExecutionDone();

    ASSERT_EQUAL(counter1.load(), static_cast<int>(thread_count));
    ASSERT_EQUAL(counter2.load(), static_cast<int>(thread_count));
}

// ============================================================================
// PERFORMANCE BENCHMARKS
// ============================================================================

void benchmark_vec2_operations() {
    const int iterations = 1000000;
    std::vector<Vec2> vectors;
    vectors.reserve(iterations);

    auto start = std::chrono::high_resolution_clock::now();
    
    for (int i = 0; i < iterations; ++i) {
        Vec2 v1(static_cast<double>(i), static_cast<double>(i + 1));
        Vec2 v2(static_cast<double>(i + 2), static_cast<double>(i + 3));
        vectors.push_back(v1 + v2);
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration<double, std::milli>(end - start).count();

    std::cout << "  Vec2 operations: " << iterations << " iterations in " 
        << duration << " ms\n";
}

void benchmark_civ_insertion_deletion() {
    const int iterations = 10000;
    civ::IndexVector<int> vec;

    auto start = std::chrono::high_resolution_clock::now();
    
    std::vector<civ::ID> ids;
    for (int i = 0; i < iterations; ++i) {
        ids.push_back(vec.push_back(i));
    }

    for (int i = 0; i < iterations / 2; ++i) {
        vec.erase(ids[i * 2]);
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration<double, std::milli>(end - start).count();

    std::cout << "  CIV insert/delete: " << iterations << " operations in " 
        << duration << " ms\n";
}

void benchmark_siv_operations() {
    const int iterations = 10000;
    siv::Vector<int> vec;

    auto start = std::chrono::high_resolution_clock::now();
    
    std::vector<siv::ID> ids;
    for (int i = 0; i < iterations; ++i) {
        ids.push_back(vec.push_back(i));
    }

    for (int i = 0; i < iterations / 2; ++i) {
        vec.erase(ids[i * 2]);
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration<double, std::milli>(end - start).count();

    std::cout << "  SIV insert/delete: " << iterations << " operations in " 
        << duration << " ms\n";
}

// ============================================================================
// MAIN TEST RUNNER
// ============================================================================

int main() {
    std::cout << "========================================\n";
    std::cout << "ROGUE CITY CORE TESTS\n";
    std::cout << "========================================\n\n";

    // Vec2 Tests
    std::cout << "--- Vec2 Tests ---\n";
    TEST(test_vec2_construction);
    TEST(test_vec2_length);
    TEST(test_vec2_normalize);
    TEST(test_vec2_operators);
    TEST(test_vec2_dot_product);
    TEST(test_vec2_cross_product);
    TEST(test_vec2_angle);
    TEST(test_vec2_lerp_free_function);
    TEST(test_vec2_distance);

    // Tensor2D Tests
    std::cout << "\n--- Tensor2D Tests ---\n";
    TEST(test_tensor2d_fromAngle);
    TEST(test_tensor2d_fromVector);
    TEST(test_tensor2d_eigenvector);

    // CIV Tests
    std::cout << "\n--- Constant Index Vector Tests ---\n";
    TEST(test_civ_basic_operations);
    TEST(test_civ_erase);
    TEST(test_civ_reference);
    TEST(test_civ_iteration);

    // SIV Tests
    std::cout << "\n--- Stable Index Vector Tests ---\n";
    TEST(test_siv_basic_operations);
    TEST(test_siv_erase);
    TEST(test_siv_handle);
    TEST(test_siv_handle_validity);

    // RogueWorker Tests
    std::cout << "\n--- RogueWorker Tests ---\n";
    TEST(test_rogue_worker_basic_execution);
    TEST(test_rogue_worker_parallel_computation);
    TEST(test_rogue_worker_multiple_groups);

    // Performance Benchmarks
    std::cout << "\n--- Performance Benchmarks ---\n";
    benchmark_vec2_operations();
    benchmark_civ_insertion_deletion();
    benchmark_siv_operations();

    TestRunner::printSummary();

    return 0;
}
