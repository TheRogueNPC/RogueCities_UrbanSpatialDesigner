# Type Alias Migration Guide

## Overview

This guide helps you migrate existing code to use the new type aliases defined in `RogueCity/Core/Types.hpp`.

## Benefits of Using Aliases

1. **Cleaner code**: `CIV<Road>` vs `civ::IndexVector<Road>`
2. **Consistent API**: All types under `RogueCity::Core` namespace
3. **Future-proof**: Easier to swap implementations
4. **Better IDE support**: Single-include for all core types

## Before and After Examples

### Example 1: Constant Index Vector

#### Before
```cpp
#include "RogueCity/Core/Util/IndexVector.hpp"

civ::IndexVector<Road> roads;
civ::ID road_id = roads.push_back(Road{...});
civ::Ref<Road> road_ref = roads.createRef(road_id);
```

#### After
```cpp
#include "RogueCity/Core/Types.hpp"

using namespace RogueCity::Core;

CIV<Road> roads;
civ::ID road_id = roads.push_back(Road{...});
CIVRef<Road> road_ref = roads.createRef(road_id);
```

### Example 2: Stable Index Vector

#### Before
```cpp
#include "RogueCity/Core/Util/StableIndexVector.hpp"

siv::Vector<District> districts;
siv::ID dist_id = districts.push_back(District{...});
siv::Handle<District> handle = districts.createHandle(dist_id);
```

#### After
```cpp
#include "RogueCity/Core/Types.hpp"

using namespace RogueCity::Core;

SIV<District> districts;
siv::ID dist_id = districts.push_back(District{...});
SIVHandle<District> handle = districts.createHandle(dist_id);
```

### Example 3: RogueWorker

#### Before
```cpp
#include "RogueCity/Core/Util/RogueWorker.hpp"

Rowk::RogueWorker worker(8);
Rowk::WorkerFunction job = [](uint32_t id, uint32_t count) {
    // Work here
};
Rowk::WorkGroup group = worker.execute(job, 8);
group.waitExecutionDone();
```

#### After
```cpp
#include "RogueCity/Core/Types.hpp"

using namespace RogueCity::Core;

RogueWorker worker(8);
WorkerFunc job = [](uint32_t id, uint32_t count) {
    // Work here
};
WorkGroup group = worker.execute(job, 8);
group.waitExecutionDone();
```

### Example 4: Complete Pipeline

#### Before
```cpp
#include "RogueCity/Core/Math/Vec2.hpp"
#include "RogueCity/Core/Data/CityTypes.hpp"
#include "RogueCity/Core/Data/TensorTypes.hpp"
#include "RogueCity/Core/Util/IndexVector.hpp"
#include "RogueCity/Core/Util/StableIndexVector.hpp"
#include "RogueCity/Core/Util/RogueWorker.hpp"

class CityGenerator {
    civ::IndexVector<Road> roads_;
    siv::Vector<District> districts_;
    Rowk::RogueWorker worker_;
    
    void generateRoads() {
        auto job = [this](uint32_t id, uint32_t count) {
            // Generate roads in parallel
            for (uint32_t i = id; i < road_count; i += count) {
                RogueCity::Core::Vec2 pos(i * 10.0, i * 10.0);
                RogueCity::Core::Tensor2D tensor = /* ... */;
                // ...
            }
        };
        Rowk::WorkGroup group = worker_.execute(job);
        group.waitExecutionDone();
    }
};
```

#### After
```cpp
#include "RogueCity/Core/Types.hpp"

using namespace RogueCity::Core;

class CityGenerator {
    CIV<Road> roads_;
    SIV<District> districts_;
    RogueWorker worker_;
    
    void generateRoads() {
        auto job = [this](uint32_t id, uint32_t count) {
            // Generate roads in parallel
            for (uint32_t i = id; i < road_count; i += count) {
                Vec2 pos(i * 10.0, i * 10.0);
                Tensor2D tensor = /* ... */;
                // ...
            }
        };
        WorkGroup group = worker_.execute(job);
        group.waitExecutionDone();
    }
};
```

## Complete Alias Reference

### Mathematics
```cpp
// Before
RogueCity::Core::Vec2

// After
using namespace RogueCity::Core;
Vec2
```

### Data Structures
```cpp
// Before
RogueCity::Core::Road
RogueCity::Core::RoadType
RogueCity::Core::District
RogueCity::Core::LotToken
RogueCity::Core::Tensor2D

// After
using namespace RogueCity::Core;
Road
RoadType
District
LotToken
Tensor2D
```

### Containers
```cpp
// Before                              // After
civ::IndexVector<T>                    CIV<T>
civ::Ref<T>                           CIVRef<T>
siv::Vector<T>                        SIV<T>
siv::Handle<T>                        SIVHandle<T>
```

### Threading
```cpp
// Before                              // After
Rowk::RogueWorker                     RogueWorker
Rowk::WorkGroup                       WorkGroup
Rowk::ExecutionGroup                  WorkGroup
Rowk::WorkerFunction                  WorkerFunc
```

## Migration Strategy

### Step 1: Update Includes
Replace multiple includes with single include:
```cpp
// OLD - Multiple includes
#include "RogueCity/Core/Math/Vec2.hpp"
#include "RogueCity/Core/Data/CityTypes.hpp"
#include "RogueCity/Core/Util/IndexVector.hpp"

// NEW - Single include
#include "RogueCity/Core/Types.hpp"
```

### Step 2: Add Namespace
```cpp
using namespace RogueCity::Core;
```

### Step 3: Find and Replace

Use these search/replace patterns:

| Search | Replace |
|--------|---------|
| `civ::IndexVector<` | `CIV<` |
| `civ::Ref<` | `CIVRef<` |
| `siv::Vector<` | `SIV<` |
| `siv::Handle<` | `SIVHandle<` |
| `Rowk::RogueWorker` | `RogueWorker` |
| `Rowk::WorkGroup` | `WorkGroup` |
| `Rowk::ExecutionGroup` | `WorkGroup` |
| `Rowk::WorkerFunction` | `WorkerFunc` |

### Step 4: Verify Build
```bash
cmake --build build --config Release
```

## Important Notes

### ID Types Unchanged
The ID types remain in their original namespaces:
```cpp
civ::ID    // For CIV
siv::ID    // For SIV
```

This is intentional to avoid confusion between different ID types.

### Namespace Usage
You can choose your namespace strategy:

```cpp
// Option 1: Full using directive (recommended for .cpp files)
using namespace RogueCity::Core;
CIV<Road> roads;

// Option 2: Selective using (recommended for headers)
using RogueCity::Core::CIV;
using RogueCity::Core::SIV;
CIV<Road> roads;

// Option 3: Full qualification (most explicit)
RogueCity::Core::CIV<Road> roads;
```

### Backward Compatibility
All old type names still work! The aliases are additions, not replacements:
```cpp
// Both work:
civ::IndexVector<Road> roads1;  // Old style
CIV<Road> roads2;                // New style
```

## Common Pitfalls

### Pitfall 1: Forgetting Namespace
```cpp
#include "RogueCity/Core/Types.hpp"

// ERROR: CIV not found
CIV<Road> roads;

// FIX: Add namespace
using namespace RogueCity::Core;
CIV<Road> roads;
```

### Pitfall 2: Mixing ID Types
```cpp
// WRONG: Using siv::ID with CIV
CIV<Road> roads;
siv::ID id = roads.push_back(Road{});  // Type mismatch!

// CORRECT: Use civ::ID with CIV
CIV<Road> roads;
civ::ID id = roads.push_back(Road{});
```

### Pitfall 3: Circular Dependencies
```cpp
// Header file - DON'T use "using namespace" in headers!
// WRONG:
// MyClass.hpp
#pragma once
#include "RogueCity/Core/Types.hpp"
using namespace RogueCity::Core;  // DON'T DO THIS IN HEADERS!

class MyClass {
    CIV<Road> roads_;
};

// CORRECT:
// MyClass.hpp
#pragma once
#include "RogueCity/Core/Types.hpp"

class MyClass {
    RogueCity::Core::CIV<Road> roads_;  // Fully qualified
};
```

## Testing Your Migration

After migrating, verify with these checks:

### Compile Check
```bash
cmake --build build --config Release
```

### Test Check
```bash
./build/Release/test_core.exe
./build/Release/test_generators.exe
```

### Code Review Checklist
- [ ] All includes updated to `RogueCity/Core/Types.hpp`
- [ ] Namespace added appropriately (cpp files only)
- [ ] All type names use new aliases
- [ ] ID types are correct (`civ::ID` vs `siv::ID`)
- [ ] No `using namespace` in header files
- [ ] Code compiles without warnings
- [ ] Tests pass

## Example Migration: Full File

### Before
```cpp
// CityBuilder.cpp
#include "CityBuilder.hpp"
#include "RogueCity/Core/Math/Vec2.hpp"
#include "RogueCity/Core/Data/CityTypes.hpp"
#include "RogueCity/Core/Util/IndexVector.hpp"

void CityBuilder::buildRoads() {
    civ::IndexVector<RogueCity::Core::Road> roads;
    
    for (int i = 0; i < 100; ++i) {
        RogueCity::Core::Road road;
        road.polyline.push_back(RogueCity::Core::Vec2(i * 10.0, 0.0));
        road.polyline.push_back(RogueCity::Core::Vec2(i * 10.0, 100.0));
        road.type = RogueCity::Core::RoadType::M_Major;
        
        civ::ID id = roads.push_back(road);
        road_ids_.push_back(id);
    }
}
```

### After
```cpp
// CityBuilder.cpp
#include "CityBuilder.hpp"
#include "RogueCity/Core/Types.hpp"

using namespace RogueCity::Core;

void CityBuilder::buildRoads() {
    CIV<Road> roads;
    
    for (int i = 0; i < 100; ++i) {
        Road road;
        road.polyline.push_back(Vec2(i * 10.0, 0.0));
        road.polyline.push_back(Vec2(i * 10.0, 100.0));
        road.type = RoadType::M_Major;
        
        civ::ID id = roads.push_back(road);
        road_ids_.push_back(id);
    }
}
```

## Benefits Summary

| Aspect | Before | After | Improvement |
|--------|--------|-------|-------------|
| Include count | 5+ headers | 1 header | 80% fewer includes |
| Type length | `civ::IndexVector<Road>` | `CIV<Road>` | 60% shorter |
| Namespace clutter | 3+ namespaces | 1 namespace | Cleaner code |
| Readability | Verbose | Concise | Much better |
| Maintainability | Scattered includes | Centralized | Easier to update |

## Questions?

If you encounter issues during migration:

1. Check this guide's examples
2. Look at `tests/test_core.cpp` for working examples
3. Review `core/include/RogueCity/Core/Types.hpp` for alias definitions
4. See `docs/TestingQuickStart.md` for more examples

---

**Happy migrating! Your code will thank you for it.** ?
