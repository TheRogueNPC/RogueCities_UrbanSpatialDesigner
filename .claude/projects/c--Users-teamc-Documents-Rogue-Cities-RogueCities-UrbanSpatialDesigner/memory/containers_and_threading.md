# RogueCities — Container Internals & Threading

## fva::Container<T> (FastVectorArray)
File: `core/include/RogueCity/Core/Util/FastVectorArray.hpp` (fast_array.hpp)

Dense slot-based array with stable handles. Swap-with-end removal. No heap fragmentation.

```cpp
fva::Container<T>
  Handle<T> add(Args&&...);           // returns stable handle
  void remove(Handle<T>&);            // swap-with-end, invalidates last handle
  void remove(ID index);
  T& operator[](ID index);
  bool isValidIndex(ID index);
  uint64_t size();        // active count
  uint64_t indexCount();  // total slot count
  void clear();
  // begin()/end() iterate active data only

fva::Handle<T>  — stable ref; operator*, operator->, getIndex()
fva::GenericHandle<U> — type-erased handle
```

Internals: m_data (dense), m_index (id→data_idx), m_active_index (bitfield),
m_reverse_index (data_idx→id), m_free_indexes (reuse queue)

Use for: Roads, Districts, Lots, Axioms, WaterBodies (editor-facing, mutation-heavy)

---

## civ::IndexVector<T> (ConstantIndexVector)
File: `core/include/RogueCity/Core/Util/IndexVector.hpp` (constant_index_vector.hpp)

IDs never change after insertion. Validity counter incremented on erase.

```cpp
civ::IndexVector<T>
  ID push_back(const T&);
  ID emplace_back(Args&&...);
  void erase(ID); void erase(const Ref<T>&);
  Ref<T> createRef(ID);
  bool isValid(ID, ID validity_id) const;
  T& operator[](ID);

civ::Ref<T> — ref with validity check; operator*, operator->, getID(), destroy()
```

Internals: m_data, m_metadata{rid, validity_id}, m_indexes (id→data_idx),
operation_count (global validity counter), swap-with-end removal

Use for: internal/scratch sets, pathfinding buffers, meshing scratch

---

## siv::Vector<T> (StableIndexVector)
File: `core/include/RogueCity/Core/Util/StableIndexVector.hpp` (stable_index_vector.hpp)

Explicit Handle lifetime. Validity-checked across container mutations.

```cpp
siv::Vector<T>
  static constexpr ID InvalidID = numeric_limits<ID>::max();
  ID push_back(const T&);
  ID emplace_back(Args&&...);
  void erase(ID); void erase(const Handle<T>&);
  void eraseViaData(uint32_t idx);
  Handle<T> createHandle(ID);
  Handle<T> createHandleFromData(uint64_t idx);
  bool isValid(ID, ID validity_id) const;
  bool isValidID(ID) const;
  void remove_if(TCallback&&);
  void reserve(size_t); void clear();
  ID getNextID() const;

siv::Handle<T> — non-copyable; embeds validity_id at creation; operator*, operator->
```

Internals: m_data, m_metadata{rid, validity_id}, m_indexes (id→data_idx)

Use for: BuildingSites, Agents, Props — long-lived refs with validity checks

---

## RogueWorker (Thread Pool)
File: `core/include/RogueCity/Core/Util/RogueWorker.hpp`
Namespace: `Rowk`

```cpp
using WorkerFunction = std::function<void(uint32_t job_id, uint32_t group_size)>;

class RogueWorker {
    RogueWorker(uint32_t thread_count);
    WorkGroup execute(WorkerFunction job, uint32_t group_size = 0);
};

class WorkGroup {
    void waitExecutionDone();
};

class ExecutionGroup {
    void start(); void waitExecutionDone();
    // private: notifyWorkerDone(), waitWorkersDone()
    // atomic<uint32_t> m_done_count + condition_variable
};

class Worker {
    void setJob(uint32_t id, ExecutionGroup* group);
    void stop(); void join();
    // binary barriers: m_ready_mutex, m_done_mutex
};

class Synchronizer {
    static void lockAtReady(list<Worker*>&);
    static void unlockAtReady(list<Worker*>&);
    static void lockAtDone(list<Worker*>&);
    static void stop/join(list<Worker*>&);
};
```

Sync pattern: Worker waits-ready → executes job → waits-done → returns to pool
10ms Rule: any operation >10ms must use RogueWorker or it will feel laggy
Context-aware threshold: enable threading when axiom_count * district_count > 100

---

## DeterminismHash
File: `core/include/RogueCity/Core/Validation/DeterminismHash.hpp`

```cpp
struct DeterminismHash {
    uint64_t roads_hash, districts_hash, lots_hash, buildings_hash, tensor_field_hash;
    bool operator==(const DeterminismHash&) const noexcept;
    std::string to_string() const;
};
DeterminismHash ComputeDeterminismHash(const GlobalState&);
bool SaveBaselineHash(const DeterminismHash&, const std::string& filepath);
bool ValidateAgainstBaseline(const DeterminismHash&, const std::string& filepath);
```

Used by `tests/unit/test_determinism_baseline.cpp` and `tests/test_determinism_comprehensive.cpp`.

---

## Road Generation Policy Types
File: `generators/include/RogueCity/Generators/Roads/Policies.hpp`
```cpp
// All derive from Core::Roads::IRoadPolicy
class GridPolicy        { RoadAction ChooseAction(const RoadState&, uint32_t rng_seed) const; };
class OrganicPolicy     { RoadAction ChooseAction(const RoadState&, uint32_t rng_seed) const; };
class FollowTensorPolicy{ RoadAction ChooseAction(const RoadState&, uint32_t rng_seed) const; };
```

File: `generators/include/RogueCity/Generators/Roads/FlowAndControl.hpp`
```cpp
struct RoadFlowDefaults {
    float v_base{13.0f};       // km/h base velocity
    float cap_base{1.0f};      // capacity factor
    float access_control{0.0f};// 0..1 (freeway-ness)
    bool signal_allowed{true}, roundabout_allowed{true};
};
struct ControlThresholds {
    // D=demand, R=risk; thresholds in ascending control severity:
    float uncontrolled_d_max{10.0f}, uncontrolled_r_max{10.0f};
    float yield_d_max{25.0f}, yield_r_max{25.0f};
    float allway_d_max{45.0f}, allway_r_max{45.0f};
    float signal_d_max{75.0f}, signal_r_max{75.0f};
    float roundabout_d_min{60.0f}, roundabout_r_min{60.0f};
    float interchange_d_min{120.0f}, interchange_r_min{120.0f};
};
struct FlowControlConfig {
    vector<RoadFlowDefaults> road_defaults;
    ControlThresholds thresholds;
    double turn_penalty{12.0};
    float district_speed_mult, zone_speed_mult, control_delay_mult;
};
void applyFlowAndControl(Urban::Graph&, const FlowControlConfig&);
```

File: `generators/include/RogueCity/Generators/Roads/GraphSimplify.hpp`
```cpp
struct SimplifyConfig { float weld_radius{5.0f}, min_edge_length{20.0f}, collapse_angle_deg{10.0f}; };
void simplifyGraph(Urban::Graph&, const SimplifyConfig&);
```

File: `generators/include/RogueCity/Generators/Roads/RoadClassifier.hpp`
```cpp
class RoadClassifier {
    static void classifyNetwork(fva::Container<Road>&);
    static void classifyGraph(Urban::Graph&, uint32_t centrality_samples = 64u);
    static RoadType classifyRoad(const Road&, double avg_length);
    // Uses betweenness-approximation sampling
};
```
