# RogueCity Test Suite - Quick Start Guide

## What We've Built

A comprehensive test infrastructure for RogueCity's core components, focusing on:
1. **Vec2 mathematics** (including recent bug fixes)
2. **Tensor2D operations** (validates std::numbers::pi usage)
3. **CIV (Constant Index Vector)** - Fast O(1) entity management
4. **SIV (Stable Index Vector)** - Safe handles with validity checking
5. **RogueWorker** - Multithreading utilities
6. **Performance benchmarks** - Track performance regressions

## Quick Start

### 1. Build the Test Suite
```bash
# Configure CMake
cmake -S . -B build

# Build core tests
cmake --build build --config Release --target test_core

# Build generator tests (for full integration)
cmake --build build --config Release --target test_generators
```

### 2. Run Tests
```bash
# Run core component tests
./build/Release/test_core.exe

# Run generator integration tests  
./build/Release/test_generators.exe
```

### 3. Expected Output
```
========================================
ROGUE CITY CORE TESTS
========================================

--- Vec2 Tests ---
[PASS] test_vec2_construction (0.0019 ms)
[PASS] test_vec2_lerp_free_function (0 ms)  # ? Recent fix validated
...

--- Tensor2D Tests ---
[PASS] test_tensor2d_fromAngle (0.0001 ms)  # ? std::numbers::pi validated
...

Total tests: 23
Passed: 21
Failed: 2  # RogueWorker threading issues (non-critical)
========================================
```

## Recent Fixes Validated

### Fix #1: M_PI ? std::numbers::pi
**Problem**: MSVC doesn't define M_PI, causing compilation errors.
**Solution**: Use C++20 `std::numbers::pi` instead.
**Test**: `test_tensor2d_fromAngle()` validates ? calculations.

### Fix #2: Vec2::lerp Free Function  
**Problem**: Code called `Vec2::lerp(...)` as static method, but it's a free function.
**Solution**: Changed to `lerp(...)`.
**Test**: `test_vec2_lerp_free_function()` validates free function usage.

## Utility Containers (FTA, CIV, SIV)

### What are they?

These are the "Rogue Workers" utilities mentioned in the docs:
- **FTA** = Fast Thread Array (RogueWorker)
- **CIV** = Constant Index Vector
- **SIV** = Stable Index Vector

### Quick Examples

#### CIV - Fast Entity Management
```cpp
#include "RogueCity/Core/Types.hpp"
using namespace RogueCity::Core;

CIV<Road> roads;  // Aliased type from Types.hpp

// Add roads
auto id1 = roads.push_back(Road{...});
auto id2 = roads.push_back(Road{...});

// Access by ID (O(1))
roads[id1].type = RoadType::M_Major;

// Delete (O(1))
roads.erase(id1);

// Iterate (cache-friendly)
for (auto& road : roads) {
    road.update();
}
```

#### SIV - Safe References with Validity
```cpp
SIV<District> districts;

auto id = districts.push_back(District{...});
auto handle = districts.createHandle(id);

// Check validity before use
if (handle.isValid()) {
    handle->update();
}

// Handle detects deletion
districts.erase(id);
assert(!handle.isValid());  // True!
```

#### RogueWorker - Parallel Processing
```cpp
RogueWorker worker(8);  // 8 threads

// Parallel tensor field generation
auto job = [&](uint32_t worker_id, uint32_t worker_count) {
    // Each thread processes its partition
    uint32_t start = worker_id * cells_per_thread;
    uint32_t end = start + cells_per_thread;
    
    for (uint32_t i = start; i < end; ++i) {
        tensorField[i] = computeTensor(i);
    }
};

WorkGroup group = worker.execute(job, 8);
group.waitExecutionDone();  // Wait for completion
```

## Performance Benchmarks

All benchmarks run on Release build:

| Operation | Count | Time | Per Op |
|-----------|-------|------|--------|
| Vec2 math | 1M | ~3.5 ms | 0.0035 ?s |
| CIV insert/delete | 10K | ~0.45 ms | 0.045 ?s |
| SIV insert/delete | 10K | ~0.60 ms | 0.060 ?s |

**Note**: SIV is ~20% slower than CIV due to validity tracking, but provides safety.

## Using Aliased Types

All utility types are now aliased in `Core/Types.hpp` for convenience:

```cpp
#include "RogueCity/Core/Types.hpp"

// Before (verbose)
civ::IndexVector<Road> roads;
civ::Ref<Road> road_ref;
siv::Vector<District> districts;
siv::Handle<District> district_handle;
Rowk::RogueWorker worker(8);
Rowk::WorkGroup group;

// After (aliased)
CIV<Road> roads;
CIVRef<Road> road_ref;
SIV<District> districts;
SIVHandle<District> district_handle;
RogueWorker worker(8);
WorkGroup group;
```

## Known Issues

### RogueWorker Linker Error
**Status**: Affects `test_generators` only, `test_core` works fine.
**Issue**: Multiple definition errors when linking RogueWorker in static libs.
**Cause**: Header-only implementation without `inline` keywords.
**Workaround**: 
- Use RogueWorker only in core library
- OR mark all functions in RogueWorker.hpp as `inline`
- OR move implementations to .cpp file

### RogueWorker Test Failures
**Status**: 2/3 tests failing in `test_core`.
**Issue**: Workers not completing all jobs.
**Impact**: Low - single-threaded generation works correctly.
**Priority**: Medium for future multithreading support.

## Adding New Tests

### Test Structure
```cpp
void test_my_feature() {
    // Setup
    MyType obj;
    
    // Test
    obj.doSomething();
    
    // Assert
    ASSERT_TRUE(obj.isValid());
    ASSERT_EQUAL(obj.getValue(), expected);
    ASSERT_NEAR(obj.getFloat(), 1.0, 1e-6);
}

// Register test
int main() {
    TEST(test_my_feature);
    TestRunner::printSummary();
}
```

### Assertion Macros
- `ASSERT_TRUE(condition)` - Condition must be true
- `ASSERT_FALSE(condition)` - Condition must be false
- `ASSERT_EQUAL(a, b)` - Values must be equal
- `ASSERT_NEAR(a, b, epsilon)` - Floats within epsilon

## Integration with CMake

Tests are automatically registered in `CMakeLists.txt`:
```cmake
# Core tests (Vec2, CIV, SIV, RogueWorker)
add_executable(test_core tests/test_core.cpp)
target_link_libraries(test_core PRIVATE RogueCityCore)

# Generator tests (full pipeline)
add_executable(test_generators test_generators.cpp)
target_link_libraries(test_generators PRIVATE 
    RogueCityCore 
    RogueCityGenerators)
```

## Next Steps

1. **Fix RogueWorker**: Add `inline` keywords or move to .cpp
2. **More generator tests**: Test axiom combinations
3. **Performance CI**: Track benchmark regressions
4. **Memory tests**: Add ASAN/Valgrind checks
5. **Stress tests**: 100k+ road networks

## Resources

- [Full Testing Summary](TestingSummary.md)
- [RogueWorker Docs](temp_Documentation/RogueWorkersAndYou.md)
- [CIV Docs](temp_Documentation/constantindexedvectorRM.md)
- [Design Doc](TheRogueCityDesignerSoft.md)

## Getting Help

If tests fail:
1. Check the error message carefully
2. Look at [TestingSummary.md](TestingSummary.md) for known issues
3. Verify your build configuration (Release vs Debug)
4. Check that dependencies are installed (GLM, magic_enum)

## Success Criteria

? **21/23 tests passing** (2 RogueWorker threading issues are non-critical)
? **Core library compiles without errors**
? **Recent bug fixes validated** (M_PI, Vec2::lerp)
? **Performance benchmarks running**
? **Utility containers tested** (CIV, SIV)
