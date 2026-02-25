```json
{
  "codexForwardNavigation": {
    "title": "RogueCities World-Axiom → Tensor → District → Lot Generation Plan",
    "documentIntent": "This document defines World Constraints, Environmental Layers, and Axiom Interactions that must be applied BEFORE and AFTER tensor/road generation in RogueCities.",
    "primaryPipeline": [
      "World Constraints (Terrain/Age/Nature/Zoning)",
      "Tensor Axioms (Block/Radial/Delta/GridCorrective)",
      "Streamline Tracing (Roads)",
      "Block Extraction",
      "AESP District Classification",
      "Lot/Parcel Carving",
      "Building Placement"
    ],
    "sections": [
      {
        "id": "terrain_scope",
        "label": "Terrain Scope – Pre-Axiom Constraint Fields",
        "pattern": "^##\\s+Terrain Scope",
        "meaning": "Defines slope, flood, soil, and utility fields that MUST be sampled by tensor and streamline integrators before roads are placed.",
        "mapsTo": [
          "core/RcWorldConstraintField",
          "TerrainConstraintGenerator",
          "TensorFieldInput.constraints",
          "Streamline RK4/A* cost sampling"
        ],
        "tools": [
          "GEOS union/difference/buffer for no-build polygons",
          "Heightmap → slope raster",
          "Flow accumulation for flood paths"
        ]
      },
      {
        "id": "age_history",
        "label": "Age & History – District and Zoning Bias Tags",
        "pattern": "^##\\s+Age/History",
        "meaning": "History tags do NOT affect geometry; they bias AESP district scoring and zoning interpretation after blocks are formed.",
        "mapsTo": [
          "RcWorldConstraintField.historyTags",
          "AESP district scoring",
          "InfluencerType in AxiomInput",
          "District classification weights"
        ],
        "tools": [
          "GEOS contains/intersects for lot-in-history-zone tests",
          "AESP weighting modifiers"
        ]
      },
      {
        "id": "district_zoning",
        "label": "District Zoning – Post-Road Validation and Caps",
        "pattern": "^##\\s+District Zoning",
        "meaning": "Zoning acts as a validator and limiter after districts are classified; it may create policy friction with terrain.",
        "mapsTo": [
          "PlanValidatorGenerator",
          "RcZoningPlan",
          "RcSiteProfile.policyFriction",
          "Height/density caps per district"
        ],
        "tools": [
          "GEOS intersection for zone clipping",
          "Plan violation checks"
        ]
      },
      {
        "id": "tree_nature",
        "label": "Tree & Nature Span – Local No-Build and Frontage Flavor",
        "pattern": "^##\\s+Tree/Nature Span",
        "meaning": "Nature modifies frontage desirability and introduces small no-build buffers within otherwise buildable zones.",
        "mapsTo": [
          "RcWorldConstraintField.natureScore",
          "AESP frontage scoring",
          "Lot carving adjustments"
        ],
        "tools": [
          "GEOS buffer for root zones",
          "Spatial index for habitat overlap tests"
        ]
      },
      {
        "id": "axioms",
        "label": "Axioms – City Intent Glyphs (Tensor Drivers)",
        "pattern": "^##\\s+Axioms",
        "meaning": "These are planner intent glyphs. They define desired network structure but are clamped by Terrain Scope constraints.",
        "mapsTo": [
          "AxiomInput",
          "TensorField generation",
          "Streamline tracing",
          "Road network topology"
        ],
        "tools": [
          "Tensor weighting by constraint fields",
          "RK4 streamline deflection/termination"
        ]
      },
      {
        "id": "geos_build_boxes",
        "label": "GEOS Build Boxes – Atomic Polygons for Lots/Blocks",
        "pattern": "^#\\s+GEOS",
        "meaning": "Explains how GEOS overlay operations create buildable polygons and atomic build boxes for block/lot generation.",
        "mapsTo": [
          "WorldConstraintBuilder",
          "Buildable MultiPolygon",
          "Block/Lot carving input"
        ],
        "tools": [
          "GEOS union",
          "GEOS difference",
          "GEOS intersection",
          "Prepared geometries + STRtree"
        ]
      },
      {
        "id": "site_profile_modes",
        "label": "Site Profile & Generation Modes – Edge Case Routing",
        "pattern": "^##\\s+Pattern behind all edge cases",
        "meaning": "Defines how hostile terrain, fragmentation, or policy friction change generation mode before city generation.",
        "mapsTo": [
          "RcSiteProfile",
          "RcGenerationMode",
          "CityGenerator mode switch"
        ],
        "tools": [
          "Site diagnostics pass",
          "Mode-based parameter tuning"
        ]
      },
      {
        "id": "implementation_plan",
        "label": "Implementation Plan – Where to Put the Code",
        "pattern": "^##\\s+1\\. Core layer",
        "meaning": "Concrete file locations and struct definitions for integrating World Axioms into RogueCities.",
        "mapsTo": [
          "core/",
          "generators/",
          "app/HFSM",
          "PlanValidatorGenerator"
        ],
        "tools": [
          "RogueWorker tasks",
          "HFSM states",
          "Index panels and overlays"
        ]
      }
    ]
  }
}
```

## Terrain Scope (Expanded)

**Hills, valleys, steep drops/Cliffs**  
Raw elevation mesh → slope % raster (0-5% flat, 5-15% moderate, 15-25% tough, 25%+ no-build). Cliffs as hard barriers; valleys funnel roads/rivers. Variability: erosion scars for "aged" look.

**Wet lands, flood paths**  
Simulated tensor paths (flow accumulation from DEM) + pool logic (low points aggregate). River/ocean inlets spawn flood cones (1%/100yr events). Output: dynamic flood overlay that shifts build envelopes yearly.

**Ground strength, sink spots**  
Toggleable geotech layer: stable (green), weak clay (yellow), collapse voids (red). User-placeable sinkholes trigger:

- Random building decay (lean/crack visuals).
- Zone downgrades (lux → industrial).
- Special "disaster zone" types: elevated pilings, geo-grid lots, bunker districts.

**Land lines, power lines, water lines**  
Smart noding from road/river solver:

- Frontage roads → snap utility spines (ROW offsets).
- Node clusters at intersections → substations/pumps.
- Flow logic: gravity mains downhill, pressure jumps uphill. Procedural: density drives pipe sizing (low-density = skinny, high = fat).

**Build rules, city plans**  
Seed districts from edge landmarks (old mill → industrial, chapel → residential). Zones expand radially:

- Historical cores lock typology (Victorian blocks).
- Growth rings adapt (sprawl vs compact).
- Procedural: rule gradients (strict core → loose edge).

## Age/History (Tagged Generator)

Random history tags (roll 1-5 per chunk, weight by biome/era):

- **Old farm dumps, chem spills**: Brownfield stigma → industrial bias, low lot values, hazmat icons.
- **Buried junk, ghost wells**: Mystery voids → underground quests, pump failures, faction hideouts.
- **Old graves, tribe spots**: Sacred no-build + cultural districts (totem poles, memorials spawn).  
    Bonus: Timeline slider → tag decay (WW2 bunkers rust, gold rush → tourist traps).

## District Zoning

**Edge stakes, height marks**  
Boundary mesh (ALTA-grade) + datum TIN. Height caps as extrusion limits (zoning = soft, cliffs = hard).

**Slope grids, water paths**  
1m DEM grid → contour polylines. Water paths as blue lines (min 1% grade flow).

**Tree trunks, root rings**  
DBH circles + 1:1 root CRZ halos. Procedural clustering (oak groves vs lone pines).

## Tree/Nature Span

**Big trees**  
Path blockers (root deflection for roads). Nature score → policy:

- High respect: preserve 80%, bonus density elsewhere.
- Low: clear-cut, tree farms regrow slow.

**Little trees**  
Filler scatter (density = nature score). Edge softening, park understory.

**Wild critters, no-touch zones**  
Animal heatmaps: deer trails → green corridors, wolf packs → buffer zones. Invasion fronts spawn encounters (bear raids = barricades).

**Rain flow, soak pits**  
Overland flow vectors → swales/basins. Soak pits auto-place in low permeability zones.

## Axioms (Global Locks)

**Slope map**: Raster driver—flattens dictate road curves, lot tilts.  
**No-go zones**: Boolean carve—wetlands/roots/sacred erase volume.  
**View lines**: Corridor cones from peaks—protected sightlines ban tall builds.  
**Road hooks**: Mandatory tie-ins—arterial spines force grid alignment.  
**Build boxes** = Hierarchical carve:

- **Districts** → macro zones (ind/res/mix).
- **Zones** → sub-rules (height/density).
- **Blocks** → street-bounded chunks.
- **Lots/Parcels** → final carve (post-roads, fit typology).

Axioms cascade down: mutate layouts inside boxes only. Budget/approval knobs scale box count (tight = elite, loose = sprawl).


Your current axioms are already the “top‑down planning” layer; the new stuff we’ve been talking about is mostly **pre‑axiom constraint fields and post‑axiom interpretation**.

Here’s the clean mapping.

## 1. What your axioms are now

From your README:

- **AxiomInput** = “planner intent glyph”
    - `type` = Block / Radial / Delta / GridCorrective
    - `pos`, `radius` = spatial influence
    - `influencer` = district bias (Market, Harbor, etc.)
- Pipeline:  
    `User Axioms → Tensor Field → Streamline Tracing → Road Network → Block Extraction → AESP → Districts → Lots → Buildings`

So: **your axioms = road-structure & land‑use tendencies**, not physical constraints.

## 2. Where Terrain / Age / Nature live

Those “Terrain Scope / Age / Nature / Zoning” things should become **separate layers that modulate axiom effect**, not new axiom types.

Think of three strata:

1. **Pre‑Axiom Constraint Fields** (terrain, flood, soils, habitats, sacred sites)
2. **Axiom Field** (your current tensor axioms)
3. **Post‑Axiom Classifiers** (AESP → districts, zoning rules)

Concrete placement in your architecture:

- `core/`
    - Add data fields: `SlopeField`, `FloodMask`, `SoilStrengthField`, `NoBuildMask`, `ViewCorridorMask`, `HistoryTags`, `NatureScoreField`.
- `generators/`
    - Terrain/constraint generators that fill those fields from a heightmap + random history/nature seeds.
    - Modified tensor/road generator that **samples these fields before laying or continuing a streamline** (costs, hard blocks).
- `app/`
    - “Axiom placement UI” already exists; add overlays to visualize slope/no‑build/nature while placing axioms.

## 3. How they correlate to your Axiom system

### Terrain Scope → tensor weighting / hard masks

- High slope, flood, weak soil, protected habitat → **raise streamline cost or block integration**.
- Implementation: when computing the tensor field or integrating RK4, sample constraint fields:
    - If `NoBuildMask` or extreme slope: kill or deflect streamline.
    - If bad but not fatal (weak soil), let road pass but mark segment as “high infrastructure cost”.

Effect: your **Block/Radial/Delta axioms still define directionality**, but terrain decides _where they actually manifest_.

### Age/History → AESP & district bias

- History tags (old dumps, tribe sites, ghost wells) are **district and zoning modifiers**, not geometry drivers.
- Wire into:
    - `InfluencerType influencer` in `AxiomInput` (e.g., “Industrial bias” for old dump; “Civic/Cultural” for sacred site).
    - District formulas (AESP) as extra weights: history tag can nudge classification towards Industrial, Civic, etc.

Effect: **same road network, different story and land‑use skin**.

### District Zoning → post‑road filters + caps

- Zoning becomes a **validator** and **modifier**:
    - Given AESP district affinities, clamp heights/densities per zone.
    - If planned zone conflicts with emergent AESP (e.g., high A/E road but Residential zone), either:
        - Penalize grid index / desirability, or
        - Flag “policy conflict” → feedback for regen / user decision.

Effect: your **district classification already exists**, zoning is a constraint layer on top.

### Tree / Nature Span → local no‑build + frontage flavor

- Big trees / habitats → **small‑scale no‑build or high‑cost** cells within the buildable envelope.
- Nature score → perturb frontage use:
    - High nature: more open space, park‑tilted frontage even along decent A/E roads.
    - Low nature: trees cleared, more “hard” urban frontage.

Hook: Nature fields feed into your AESP → district scoring as a small modifier.

## 4. “Axioms at Heart” vs your AxiomInput

Two distinct meanings of “axiom,” but they nest nicely:

- **World Axioms (ours)**: slope, flood, no‑build, history, nature, view corridors → these define the **global feasible volume**.
- **City Axioms (yours)**: Block/Radial/Delta/GridCorrective glyphs → these define **desired network structure** inside that volume.

Pattern in your pipeline:

```text
World Axioms (Terrain / Age / Nature / Zoning)
    ↓  (generate fields/masks in core + generators)
Tensor Axioms (AxiomInput list)
    ↓  (tensor field, but with world-field-aware weighting)
Streamlines / road network
    ↓
Blocks → AESP → Districts → Lots → Buildings
```

So correlation is:

- Use **World Axioms as pre‑filters** on your tensor/streamline step.
- Keep **AxiomInput as-is** for planner intent, but always sampled through those constraint fields.
- Let **AESP + zoning + history** be the post‑hoc rules that turn frontage → districts, flavored by those tags.

If you want, next step we can sketch a small C++ interface change for `AxiomInput` and a `WorldConstraintField` struct that lives in `core/` and is passed into your tensor and road generators.

## Pattern behind all edge cases

They all boil down to one of three systemic failures:

1. **Not enough feasible area** after masks.
    
2. **High friction** between intent (City Axioms) and reality (World Axioms).
    
3. **Shape mismatch** between standard building blocks and leftover geometry.
    

So before we touch your C++ interfaces, we should:

- Add a cheap “site diagnostics” step:
    
    - `% buildable`, average slope of buildable, fragmentation index, conflict score between major intent directions and constraints.
        
- Route sites into **modes**: normal, constrained hill town, waterfront on stilts, conservation hamlet, brownfield core, etc.
  
  
  
  #  GEOS is basically your **world-axiom geometry engine**: it gives you all the boolean / overlay / containment ops you need to build and apply those masks cleanly and fast.

Here’s what it offers that’s directly useful for the system we just outlined:

## Core geometry + predicates

- Geometry model: `Point`, `LineString`, `Polygon`, `MultiPolygon`, `GeometryCollection`.
- Predicates: `intersects`, `contains`, `within`, `overlaps`, `touches`, `disjoint`, `crosses`, `covers`. Use these to:
- Check if roads/lots enter no‑build polygons.
- Test if parcels are inside district/zoning polygons.
- Find which “history” polygon (dump, graveyard) a lot falls into.

## Overlay + boolean ops (for masks)

- Ops: `intersection`, `union`, `difference`, `symDifference`, `buffer`, `convexHull`, `simplify`, `valid`.

These are exactly what you need for:

- Building **no‑build masks**:
    - `noBuild = wetlands.union(floodplain).union(sacred).union(canopyBuffers)`
- Carving **buildable area**:
    - `buildable = sitePolygon.difference(noBuild)`
- Clipping zones/districts to the site or each other.
- Generating **buffers** for view corridors, root zones, utility easements, safety belts.

This is the vector equivalent of the boolean mask math we’ve been describing.

## Prepared geometries + spatial index

- Prepared geometry: pre‑process polygons for faster repeated `contains`/`intersects` checks.
- STRtree spatial index for fast spatial querying of many geometries.

Use this when:

- You’re testing thousands of candidate road segments or lots against the same set of constraint polygons (wetlands, habitats, zoning).
- You’re doing AESP / district classification: “which district poly covers this frontage poly?”

Pattern: prepare your big constraint layers once, then hammer them from your generators.

## Overlay as “build boxes” engine

GEOS is tailor‑made for what I called **build boxes**:

- Start with site polygon → subtract constraints (difference).
- Overlay with zoning grid / district seeds to split into smaller units (intersection / polygon overlay).
- What falls out are **atomic polygons with consistent attributes** (slope band tag, zone tag, history tag, etc.).

That’s exactly how GIS does “least common geographic units”; you can treat each resulting polygon as a candidate **lot / block fragment** with a full attribute stack.

---

next step  sketch a small GEOS‑backed `WorldConstraintBuilder` that:

- Takes raw input layers (heightmap-derived contours, simple polygons for wetlands/flood/habitats/history).
- Produces: `noBuild MultiPolygon`, `buildable MultiPolygon`, and a vector of **build box** polygons with attached tags.
  
  Here’s a concrete, codebase‑aligned plan to add **World Axioms (terrain/age/nature/zoning)** and edge‑case handling into RogueCities, using your existing layering, HFSM, and generator scaffolding.

## 1. Core layer: new world‑constraint data

Add minimal, UI‑free types in `core/include/RogueCityCore`.

**a. World constraint fields**

```cpp
struct RcWorldConstraintField {
    RcGrid<float> slope;          // deg or %
    RcGrid<uint8_t> floodMask;    // 0=none, 1=100yr, 2=10yr...
    RcGrid<float> soilStrength;   // 0–1, 0 = terrible
    RcGrid<uint8_t> noBuildMask;  // wetlands, sacred, hard no-go
    RcGrid<float> natureScore;    // 0–1
    RcGrid<uint8_t> historyTags;  // bitfield: dump, graves, tribe, etc.
};
```

**b. Site profile + mode flags**

```cpp
enum class RcGenerationMode {
    Standard,
    HillTown,
    ConservationOnly,
    BrownfieldCore,
    CompromisePlan,
    Patchwork
};

struct RcSiteProfile {
    float buildableFraction;        // 0–1
    float avgBuildableSlope;        // of buildable only
    float buildableFragmentation;   // 0–1
    float policyFriction;           // 0–1

    bool hostileTerrain;
    bool policyVsPhysics;
    bool awkwardGeometry;
    bool brownfieldPockets;

    RcGenerationMode mode;
};
```

Add both to `EditorGlobalState` as new fields, so generators and UI can inspect them without breaking layer rules.

## 2. Generators: world‑constraint builder + diagnostics

Follow your “UIGenerator Scaffolding” pattern: new generator in `generators/` with explicit config/input/output.

**a. Files**

- `generators/include/RogueCityGenerators/TerrainConstraintGenerator.hpp`
- `generators/src/Generators/TerrainConstraintGenerator.cpp`

**b. Types**

```cpp
struct TerrainConstraintConfig {
    float maxBuildableSlopeDeg;
    float hostileTerrainSlopeDeg;
    float minBuildableFraction;
    float fragmentationThreshold;
};

struct TerrainConstraintInput {
    RcHeightField height;                  // existing DEM/grid
    RcPolygon siteBoundary;                // GEOS-backed
    std::vector<RcPolygon> wetlands;
    std::vector<RcPolygon> habitats;
    std::vector<RcPolygon> sacredSites;
    std::vector<RcPolygon> brownfields;
    RcZoningPlan zoning;                   // whatever you already use for AESP/zones
};

struct TerrainConstraintOutput {
    RcWorldConstraintField constraints;
    RcSiteProfile profile;
    RcMultiPolygon buildablePoly;          // GEOS MultiPolygon
};
```

**c. Implementation sketch**

Inside `TerrainConstraintGenerator`:

1. From `height` → compute `slope` grid.
2. Derive `floodMask`, `soilStrength` (procedural or from maps).
3. Union wetlands + habitats + sacred into `noBuildMask` / polygons using GEOS `union` / `difference`.
4. Derive `buildablePoly = siteBoundary - noBuild`; rasterize to `buildableMask`.
5. Compute `buildableFraction`, `avgBuildableSlope`, `buildableFragmentation`.
6. Overlay `zoning` on `buildablePoly` to compute `policyFriction`.
7. Set boolean flags and pick `RcGenerationMode`:

```cpp
RcGenerationMode selectMode(const RcSiteProfile& p, const TerrainConstraintConfig& cfg);
```

Populate `RcWorldConstraintField` and `RcSiteProfile` and store into `EditorGlobalState` and output struct.

Wire this as the **first stage** in `CityGenerator` before tensor/axioms.

## 3. Generators: make tensor/road stages world‑aware

Update existing tensor/road generators (where AxiomInput is used) to accept a pointer/ref to `RcWorldConstraintField` and `RcSiteProfile` (read‑only), **without** adding UI deps.

Example signature change in `CityGenerator` pipeline:

```cpp
struct TensorFieldInput {
    // existing axiom inputs...
    const RcWorldConstraintField* constraints; // optional
    const RcSiteProfile* siteProfile;          // optional
};
```

Inside streamline tracing:

- Sample `constraints` grids each step.
- If `noBuildMask` or insane `slope`/`floodMask`: terminate or deflect streamline.
- If `soilStrength` low or `natureScore` high: raise local cost so A* / RK4 prefers other paths.

This makes **World Axioms clamp your City Axioms** with minimal code changes.

## 4. Mode‑based behavior in generators

In `CityGenerator` (pipeline orchestrator), after `TerrainConstraintGenerator` runs:

```cpp
RcSiteProfile profile = terrainOut.profile;

switch (profile.mode) {
    case RcGenerationMode::ConservationOnly:
        runConservationPattern(...); // few roads, trails, 1–2 enclaves
        break;
    case RcGenerationMode::HillTown:
        configureHillTownParams(...); // fewer strong axioms, contour-following
        runStandardPipeline(...);
        break;
    case RcGenerationMode::BrownfieldCore:
        tagBrownfieldDistricts(...);  // historyTags → industrial/park core
        runStandardPipeline(...);
        break;
    case RcGenerationMode::CompromisePlan:
        softenDensityTargets(...); // adjust AESP/zoning config
        runStandardPipeline(...);
        break;
    case RcGenerationMode::Patchwork:
        enableMicroParcelPatterns(...); // special handling for tiny shards
        runStandardPipeline(...);
        break;
    case RcGenerationMode::Standard:
    default:
        runStandardPipeline(...);
        break;
}
```

You can keep these pattern helpers small at first (just tweak a few params), then deepen them later.

## 5. Validator / “approval” generator

Add a **PlanValidatorGenerator** stage after roads/blocks/lots.

**a. Types**

```cpp
struct PlanValidatorInput {
    const RcWorldConstraintField* constraints;
    const RcSiteProfile* siteProfile;
    RcRoadNetworkView roads;
    RcBlockView blocks;
    RcLotView lots;
    RcZoningPlan zoning;
};

struct PlanViolation {
    RcEntityId id;
    RcViolationType type;   // e.g. NoBuild, SlopeTooHigh, ZoningMismatch
    float severity;
};

struct PlanValidatorOutput {
    std::vector<PlanViolation> violations;
    bool approved;
};
```

**b. Logic**

- Check each road/lot against `noBuildMask`, slope limits, zoning rules.
- Accumulate violations; fail if above thresholds.
- This mirrors your “Approval system” as a deterministic pass in `generators/`.

Later, you can hook this into HFSM to show “Plan Denied / Plan Approved” in UI, but the core stays app‑free.

## 6. App: HFSM + UI integration (Cockpit Doctrine)

Using the “UIGenerator Scaffolding” workflow in `Agents.md` / `copilot-instructions.md`:

**a. HFSM**

- Add states in `app/src/HFSM/HFSMStates.cpp`:
    
    - `State::TerrainAnalyze` – runs `TerrainConstraintGenerator` via RogueWorker.
    - `State::GenerateCity` – runs tensor/roads/blocks/lots.
    - `State::ValidatePlan` – runs PlanValidator, shows approval result.

Keep heavy work offloaded to `RogueWorker` per Rogue Protocol.

**b. Index panels + overlays**

- New “Site Diagnostics” panel using `RcDataIndexPanelT` listing:
    
    - buildable %, avg slope, fragmentation, mode.
    - counts of violations by type.
- Viewport overlays:
    
    - Slope heatmap, no‑build polygons, buildable area outline.
    - Toggle for history/nature overlays.

This is exactly the kind of “data structure mirrored in UI” your Cockpit Doctrine calls for.

## 7. AI integration 
You already have **CitySpec** and AI toolserver. Two useful hooks:

- Extend `CitySpec` with optional constraints (min buildable %, preferred mode, “respects nature” flag).
- Let the AI “City Planner Agent” propose default `TerrainConstraintConfig` / thresholds or suggest mode switches when policy friction is high.

This stays within your AI integration framework and doesn’t touch core performance contracts.