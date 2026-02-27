# RogueCities — Domain Knowledge & Generation Pipeline

## Core Data Types (core/include/RogueCity/Core/Data/CityTypes.hpp)

### Key Enumerations
```cpp
enum RoadType: uint8_t {
    Highway, Arterial, Avenue, Boulevard, Street,
    Lane, Alleyway, CulDeSac, Drive, Driveway, M_Major, M_Minor  // 12 types
};
enum DistrictType: uint8_t { Mixed, Residential, Commercial, Civic, Industrial };
enum LotType: uint8_t {
    None, Residential, RowhomeCompact, RetailStrip, MixedUse,
    LogisticsIndustrial, CivicCultural, LuxuryScenic, BufferStrip
};
enum BuildingType: uint8_t {
    None, Residential, Rowhome, Retail, MixedUse, Industrial, Civic, Luxury, Utility
};
enum WaterType: uint8_t { Lake, River, Ocean, Pond };
enum GenerationMode: uint8_t {
    Standard, HillTown, ConservationOnly, BrownfieldCore, CompromisePlan, Patchwork
};
enum GenerationTag: uint8_t { Generated = 0, M_user };
```

## GlobalState (core/include/RogueCity/Core/Editor/GlobalState.hpp)
```cpp
struct GlobalState {
    fva::Container<Road> roads;
    fva::Container<District> districts;
    fva::Container<BlockPolygon> blocks;
    fva::Container<LotToken> lots;
    fva::Container<EditorAxiom> axioms;
    fva::Container<WaterBody> waterbodies;
    siv::Vector<BuildingSite> buildings;
    RenderSpatialGrid render_spatial_grid;   // CSR-format spatial index
    std::vector<VpProbeData> viewport_index;
    std::unique_ptr<Data::TextureSpace> texture_space;
    Bounds texture_space_bounds;
    int texture_space_resolution{0};
    double city_meters_per_pixel{2.0};
    DirtyLayerState dirty_layers;
    ValidationOverlayState validation_overlay;
};
```

## EditorState HFSM (core/include/RogueCity/Core/Editor/EditorState.hpp)
```
Startup → ProjectLoading | NoProject
Main: Idle ↔ Editing → {Editing_Axioms, Editing_Roads, Editing_Districts,
                         Editing_Lots, Editing_Buildings, Editing_Water}
Viewport: {Viewport_Pan, Viewport_Select, Viewport_PlaceAxiom,
           Viewport_DrawRoad, Viewport_BoxSelect}
Simulation: Simulating ↔ {Simulation_Paused, Simulation_Stepping}
Playback: Playback ↔ {Playback_Paused, Playback_Scrubbing}
Modal: {Modal_Exporting, Modal_ConfirmQuit}
Exit: Shutdown
```
Implementation: `core/src/Core/Editor/EditorHFSM.cpp`

## Tool Domains & Subtools
```
ToolDomain: Axiom, Water, Road, District, Zone, Lot, Building, FloorPlan, Paths, Flow, Furnature
WaterSubtool: Flow, Contour, Erode, Select, Mask, Inspect
RoadSubtool: Spline, Grid, Bridge, Select, Disconnect, Stub, Curve, Strengthen, Inspect
DistrictSubtool: Zone, Paint, Split, Select, Merge, Inspect
LotSubtool: Plot, Slice, Align, Select, Merge, Inspect
BuildingSubtool: Place, Scale, Rotate, Select, Assign, Inspect
```

## CityGenerator Pipeline (generators/include/RogueCity/Generators/Pipeline/CityGenerator.hpp)

### Generation Stages (in order)
```cpp
enum GenerationStage: uint8_t {
    Terrain, TensorField, Roads, Districts, Blocks, Lots, Buildings, Validation
};
```

### Key API
```cpp
class CityGenerator {
    CityOutput generate(const vector<AxiomInput>&);
    CityOutput generate(const vector<AxiomInput>&, const Config&);
    CityOutput GenerateWithContext(axioms, config, GenerationContext*, GlobalState*);
    CityOutput GenerateStages(axioms, config, StageOptions, GlobalState*, context*);
    CityOutput RegenerateIncremental(axioms, config, StageMask dirty, GlobalState*, context*);
    static ValidationResult ValidateAndClampConfig(const Config&);
    static bool ValidateAxioms(const vector<AxiomInput>&, const Config&, vector<string>* errors);
};
```

### Config fields
width, height, cell_size, seed, num_seeds, max_districts, max_lots, max_buildings,
enable_world_constraints, max_texture_resolution, incremental_mode, adaptive_tracing,
min/max_trace_step_size, trace_curvature_gain

### AxiomInput Types (10 types)
```cpp
enum Type: uint8_t {
    Organic=0, Grid=1, Radial=2, Hexagonal=3, Stem=4,
    LooseGrid=5, Suburban=6, Superblock=7, Linear=8, GridCorrective=9
};
```

### CityOutput fields
roads (fva), districts, blocks, lots, buildings (siv), city_boundary (vector<Vec2>),
tensor_field, world_constraints, plan_violations, grid_quality, plan_approved

## Tensor Field (generators/include/RogueCity/Generators/Tensors/)

### Tensor2D — direction field element
```cpp
struct Tensor2D {
    double r;           // anisotropy magnitude
    double m0, m1;      // basis coefficients
    static Tensor2D fromAngle(double radians);
    static Tensor2D fromVector(const Vec2& v);
    Vec2 majorEigenvector() const;
    Vec2 minorEigenvector() const;
    Tensor2D& add(const Tensor2D& other, bool smooth = false);
    Tensor2D& scale(double s);
    Tensor2D& rotate(double radians);
};
```

### BasisField Strategy (14+ implementations)
Override fields take precedence over additive fields.
Confidence [0,1] indicates field definition quality.
Weight decays from center (smooth Hermite falloff).

Available fields: Organic, Radial, RadialHubSpoke, Grid, Hexagonal, Stem, LooseGrid,
Suburban, Superblock, Linear, GridCorrective, StrongLinearCorridor, ShearPlane,
BoundarySeam, CenterVoidOverride, NoisePatch, ParallelLinearBundle, PeriodicRung

### TensorFieldGenerator API
```cpp
class TensorFieldGenerator {
    void addOrganicField(center, radius, theta, curviness);
    void addRadialField(center, radius, spokes=8);
    void addGridField(center, radius, theta);
    // ... one add* per axiom type
    void addOverrideField(unique_ptr<BasisField>);  // Takes precedence
    void addAdditiveField(unique_ptr<BasisField>);
    Tensor2D sampleTensor(const Vec2& world_pos) const;
    double sampleTensorConfidence(const Vec2& world_pos) const;
    void generateField();
    void writeToTextureSpace(TextureSpace&) const;
};
```

## Terminal Features (40 flags, 4 per axiom type)
```
Organic: TopologicalFlow, MeanderBias, VoronoiRelaxation, CulDeSacPruning
Grid: AxisAlignmentLock, DiagonalSlicing, AlleywayBisection, BlockFusion
Radial: SpiralDominance, CoreVoiding, SpokePruning, ConcentricWaveDensity
Hexagonal: HoneycombStrictness, TriangularSubdivision, OffsetStagger, OrganicEdgeBleed
Stem: FractalRecursion, DirectionalFlowBias, CanopyWeave, TerminalLoops
LooseGrid: HistoricalFaultLines, JitterPersistence, TJunctionForcing, CenterWeightedDensity
Suburban: LollipopTerminals, ArterialIsolation, TerrainAvoidance, HierarchicalStrictness
Superblock: PedestrianMicroField, ArterialTrenching, CourtyardVoid, PermeableEdges
Linear: RibbonBraiding, ParallelCascading, PerpendicularRungs, TaperedTerminals
GridCorrective: AbsoluteOverride, MagneticAlignment, OrthogonalCull, BoundaryStitching
```

## Road Generation (StreetSweeper Protocol)

### Canonical Stage Order
1. Build tensor field (axioms + basis fields)
2. Trace road candidates (tensor → polylines via RK4)
3. Node into graph (snap/split/weld intersections)
4. Simplify topology (weld, micro-edge removal, degree-2 collapse)
5. Classify edges (topology + AESP + district context)
6. Score flow (V_eff, flow_score on edges; Demand D, Risk R on nodes)
7. Assign controls (control ladder: uncontrolled→yield→stop→signal→roundabout→interchange)
8. Verticality (layer_id, grade separation, portals)

### Key Road Data Types
- PolylineRoadCandidate: traced polyline + TraceMeta (curvature, slope, termination)
- FlowStats: v_base, cap_base, access_control, v_eff (computed), flow_score
- ControlType: Uncontrolled, Yield, TwoWayStop, AllWayStop, Signal, Roundabout, GradeSep, Interchange
- SegmentGridStorage: O(k) local intersection checks
- Graph: Vertices + Edges with layer_id, FlowStats, ControlType

### Control Ladder (deterministic, from demand D and risk R)
Very low D & R → Uncontrolled
Low D, moderate R → Yield/Two-Way Stop
Moderate D, balanced → All-Way Stop
High D or R → Signal
High D & R with space → Roundabout
Extreme D or access-controlled → Interchange/Grade Sep

### Road Policy Config
File: `generators/config/road_policy_defaults.json`
Contains: V_base/Cap_base per road type, district modifiers, control thresholds,
verticality policy, intersection template costs (at-grade, roundabout, grade sep, interchange)

## WorldConstraintField
```cpp
struct WorldConstraintField {
    int width, height; double cell_size;
    vector<float> height_meters, slope_degrees, soil_strength, nature_score;
    vector<uint8_t> flood_mask, no_build_mask, history_tags;
    bool worldToGrid(const Vec2&, int& gx, int& gy) const;
    float sampleHeightMeters(const Vec2&) const;
    bool sampleNoBuild(const Vec2&) const;
};
```

## TextureSpace (core/include/RogueCity/Core/Data/TextureSpace.hpp)
Layers: Height, Material, Zone, Tensor, Distance
Maps world coords to UV texture space.
`dirty_regions` tracks changed areas for efficient updates.
`city_meters_per_pixel` from GlobalState controls resolution.

## AESP Zoning Standard
Access (road connectivity), Exposure (visibility/prominence),
Serviceability (infrastructure reach), Privacy (noise/intrusion)
File: `generators/include/RogueCity/Generators/Districts/AESPClassifier.hpp`

## Grid Index Metrics (Analytics)
Straightness (ς), Orientation Order (Φ), Intersection Proportion (I)
File: `core/include/RogueCity/Core/Analytics/GridMetrics.hpp`
Generator: `generators/include/RogueCity/Generators/Scoring/GridAnalytics.hpp`

## Test Suite (tests/ — 38 files)
Key tests to know:
- `test_editor_hfsm.cpp` — HFSM state machine
- `test_city_generator_validation.cpp` — generator validation
- `test_full_pipeline.cpp` — full pipeline integration
- `test_determinism_comprehensive.cpp` — seed reproducibility
- `test_incremental_parity.cpp` — incremental generation
- `test_viewport_interaction_selection.cpp` — selection system
- `test_aesp_scoring_profiles.cpp` — AESP scoring
- `unit/test_determinism_baseline.cpp` — determinism baseline
