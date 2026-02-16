# Rogue City Designer

[](https://github.com/TheRogueNPC/RogueCities_UrbanSpatialDesigner#rogue-city-designer)

### An Urban Spatial City Generator

[](https://github.com/TheRogueNPC/RogueCities_UrbanSpatialDesigner#an-urban-spatial-city-generator)

[![Status](https://camo.githubusercontent.com/7d9bc271925c22866e74f7a1594a7993330e59c2a35bc746cf8b8a25562f8b91/68747470733a2f2f696d672e736869656c64732e696f2f62616467652f7374617475732d6d76702d79656c6c6f77)](https://github.com/TheRogueNPC/RogueCityMVP) [![C++20](https://camo.githubusercontent.com/3e9ad02fc877600426092da0156c8ac59a5521404e2213e1274f0fdae8b3930f/68747470733a2f2f696d672e736869656c64732e696f2f62616467652f432532422532422d32302d626c7565)](https://en.cppreference.com/w/cpp/20) [![License](https://camo.githubusercontent.com/f8df3091bbe1149f398a5369b2c39e896766f9f6efba3477c63e9b4aa940ef14/68747470733a2f2f696d672e736869656c64732e696f2f62616467652f6c6963656e73652d4d49542d677265656e)](https://github.com/TheRogueNPC/RogueCities_UrbanSpatialDesigner/blob/main/LICENSE) [![Build](https://camo.githubusercontent.com/b0c6c6845a74cb65a7f0a32bdcfd8fbf80eeb40026c4029af424ab371c94b8bd/68747470733a2f2f696d672e736869656c64732e696f2f62616467652f6275696c642d70617373696e672d627269676874677265656e)](https://github.com/TheRogueNPC/RogueCityMVP/actions)

> **Procedural city generation through tensor field-guided street networks, AESP-based district classification, and topological street analysis**

---

## R0 Release: AI-Assisted Development Integration ✨

[](https://github.com/TheRogueNPC/RogueCities_UrbanSpatialDesigner#r0-release-ai-assisted-development-integration-)

> **NEW**: Integrated AI assistant for layout optimization, city design, and code refactoring

### What's New in R0

[](https://github.com/TheRogueNPC/RogueCities_UrbanSpatialDesigner#whats-new-in-r0)

**🤖 AI Bridge System** - One-click AI assistant startup with automatic PowerShell detection and health monitoring

**🎨 UI Agent** - Natural language UI layout optimization ("optimize layout for road editing") with real-time command generation

**🏙️ CitySpec Generator** - AI-driven city design from natural language descriptions ("a coastal tech city with dense downtown")

**🔧 Design Assistant** - Code-shape aware refactoring suggestions with pattern extraction and architecture analysis

**📊 Pattern Catalog** - Canonical UI patterns (InspectorPanel, DataIndexPanel) with AI-guided refactoring opportunities

**Status**: All 4 phases complete • Build: Passing • Integration: Working-WIP not all tools are properly hooked in yet, but the AI bridge and clients are functional.

[→ Read Full AI Integration Documentation](https://github.com/TheRogueNPC/RogueCities_UrbanSpatialDesigner/blob/main/docs/AI_Integration_Summary.md)

---

## Abstract

[](https://github.com/TheRogueNPC/RogueCities_UrbanSpatialDesigner#abstract)

Cities function as **complex adaptive systems** formed through the interplay of decentralized bottom-up self-organization and centralized top-down planning interventions. **Rogue City Designer** is a procedural generation framework that models this duality through:

1. **Tensor field axioms** encoding top-down planning intent (radial, grid, organic)
2. **AESP-based district emergence** from road network topology (Access, Exposure, Serviceability, Privacy)
3. **Multi-scale street analysis** using persistent homology and grid quality metrics
4. **Layered graph support** for bridges, tunnels, and grade-separated infrastructure

Unlike traditional square-grid generators, this system produces **coherent, believable urban environments** ranging from rigid Manhattan-style grids to organic European networks, based on axiom configuration and urban complexity principles.

---

## Research Foundation

[](https://github.com/TheRogueNPC/RogueCities_UrbanSpatialDesigner#research-foundation)

### Urban Complexity Framework

[](https://github.com/TheRogueNPC/RogueCities_UrbanSpatialDesigner#urban-complexity-framework)

Cities exhibit **organized complexity**—a balance between chaos (overstimulation, incoherence) and order (sterility, monotony). The framework evaluates this balance through:

- **Network Density**: Nodes, intersections, edges per unit area
- **Network Resilience**: Functionality under disruption (blocked streets, damage)
- **Network Connectedness**: Path diversity, permeability, routing options

### Grid Quality Metrics

[](https://github.com/TheRogueNPC/RogueCities_UrbanSpatialDesigner#grid-quality-metrics)

The generator evaluates street networks using a **composite grid index** balancing four components:

#### 1. Straightness (ς)

[](https://github.com/TheRogueNPC/RogueCities_UrbanSpatialDesigner#1-straightness-%CF%82)

ς=average great-circle distanceaverage segment length

Measures how closely streets approximate straight lines, affecting navigation efficiency and visual coherence.

#### 2. Orientation Order (Φ)

[](https://github.com/TheRogueNPC/RogueCities_UrbanSpatialDesigner#2-orientation-order-%CF%86)

Analyzes internal consistency of street orientations through:

1. Calculate bidirectional compass bearings of every street
2. Compute street orientation entropy
3. Normalize as orientation order indicator

High order → consistent grid patterns (Manhattan)  
Low order → organic networks (medieval European)

#### 3. Terrain Adaptation

[](https://github.com/TheRogueNPC/RogueCities_UrbanSpatialDesigner#3-terrain-adaptation)

Evaluates variance and inconsistency as roads adapt to slopes, elevation changes, and natural obstacles.

#### 4. Four-Way Intersection Proportion (I)

[](https://github.com/TheRogueNPC/RogueCities_UrbanSpatialDesigner#4-four-way-intersection-proportion-i)

I=four-way intersectionstotal intersections

Tracks the share of nodes that are four-way junctions, influencing traffic flow and grid character.

**Composite Grid Index:**

GridIndex=(ς×Φ×I)1/3

The geometric mean ensures non-compensatory aggregation—poor performance in any component significantly impacts the final score.

---

## System Architecture

[](https://github.com/TheRogueNPC/RogueCities_UrbanSpatialDesigner#system-architecture)

### Pipeline Overview

[](https://github.com/TheRogueNPC/RogueCities_UrbanSpatialDesigner#pipeline-overview)

```
User Axioms → Tensor Field → Streamline Tracing → Road Network → 
Block Extraction → District Classification (AESP) → Lot Subdivision → 
Building Site Placement → Export (JSON/OBJ)
```

### Layer Structure

[](https://github.com/TheRogueNPC/RogueCities_UrbanSpatialDesigner#layer-structure)

```
core/          Pure data structures (Vec2, Tensors, AESP types)
               ├─ Zero UI dependencies
               └─ Fast builds (~20 seconds)

generators/    Generation algorithms (tensor fields, roads, districts)
               ├─ Tensor field generation from axioms
               ├─ Streamline integration (RK4)
               ├─ AESP district classification
               └─ OBB lot subdivision

app/           ImGui-based interactive editor
               ├─ Axiom placement UI
               ├─ Real-time visualization
               └─ Export workflows
```

---

## Axiom System

[](https://github.com/TheRogueNPC/RogueCities_UrbanSpatialDesigner#axiom-system)

### Axiom Types

[](https://github.com/TheRogueNPC/RogueCities_UrbanSpatialDesigner#axiom-types)

Users place **axioms** as representations of geographical topology to guide road network generation:

|Axiom Type|Effect|Use Case|
|---|---|---|
|**Block**|Produces 4-way intersections, grid-style roads|Manhattan, Chicago, modern planned cities|
|**Radial (Star/Crescent)**|Produces rounded European-style roads|Paris, Moscow, baroque cities|
|**Delta (Triangle)**|Produces 3-way intersections, naturalistic style|Organic growth, hillside towns|
|**Grid Corrective**|Straightens/gridifies radial/delta roads|Hybrid planning (e.g., Washington DC)|

### Axiom Data Structure

[](https://github.com/TheRogueNPC/RogueCities_UrbanSpatialDesigner#axiom-data-structure)

```c
struct AxiomInput {
    int id;                                // Unique identifier
    int type;                              // Axiom type index (Radial, Delta, Block, Grid)
    CityModel::Vec2 pos;                   // World position
    double radius;                         // Influence radius (falloff bound)
    InfluencerType influencer;             // Optional district bias (Market, Harbor, etc.)
};
```

Axioms generate a **tensor field** that guides **streamline integration** for road tracing.

---

## AESP Frontage System

[](https://github.com/TheRogueNPC/RogueCities_UrbanSpatialDesigner#aesp-frontage-system)

### Frontage Profile Components

[](https://github.com/TheRogueNPC/RogueCities_UrbanSpatialDesigner#frontage-profile-components)

Roads are scored on four dimensions that determine adjacent land use:

- **Access (A)**: Ease of vehicular/pedestrian entry
- **Exposure (E)**: Visual prominence and visibility
- **Serviceability (S)**: Capacity for deliveries, utilities
- **Privacy (P)**: Seclusion from public thoroughfare

### Road Type → AESP Mapping

[](https://github.com/TheRogueNPC/RogueCities_UrbanSpatialDesigner#road-type--aesp-mapping)

#### Major Roads (M_Major)

[](https://github.com/TheRogueNPC/RogueCities_UrbanSpatialDesigner#major-roads-m_major)

|Type|AESP Score|Characteristics|
|---|---|---|
|**Highway**|A:1.00, E:1.00, S:0.70, P:0.00|Maximum access/exposure, zero privacy|
|**Arterial**|A:0.90, E:0.90, S:0.90, P:0.20|High accessibility, balanced service|
|**Avenue**|A:0.80, E:0.80, S:0.80, P:0.50|Balanced urban street|
|**Boulevard**|A:0.70, E:0.90, S:0.50, P:0.70|High exposure, privacy emphasis|
|**Street**|A:0.80, E:0.50, S:0.80, P:0.80|Residential balance|

#### Minor Roads (M_Minor)

[](https://github.com/TheRogueNPC/RogueCities_UrbanSpatialDesigner#minor-roads-m_minor)

|Type|AESP Score|Characteristics|
|---|---|---|
|**Lane**|A:0.50, E:0.20, S:0.50, P:1.00|Maximum privacy, limited exposure|
|**Alleyway**|A:0.30, E:0.10, S:1.00, P:0.70|Service-focused, minimal visibility|
|**Cul-de-Sac**|A:0.30, E:0.20, S:0.50, P:1.00|Residential seclusion|
|**Drive**|A:0.50, E:0.30, S:0.60, P:0.90|Private access|
|**Driveway**|A:0.20, E:0.05, S:0.70, P:1.00|Minimal public interface|

---

## District Classification

[](https://github.com/TheRogueNPC/RogueCities_UrbanSpatialDesigner#district-classification)

The generator converts **Frontage Profiles (AESP)** into **district-type affinities** using weighted formulas reflecting urban planning principles:

### District Type Formulas

[](https://github.com/TheRogueNPC/RogueCities_UrbanSpatialDesigner#district-type-formulas)

#### 1. Mixed-Use: 0.25(A + E + S + P) — Balanced

[](https://github.com/TheRogueNPC/RogueCities_UrbanSpatialDesigner#1-mixed-use-025a--e--s--p--balanced)

- **Character**: Versatile neighborhoods combining residential, commercial, light industrial
- **Buildings**: Mid-rise apartments with ground-floor retail, live-work lofts, mixed-use towers
- **Height**: 3-8 stories
- **Features**: Active street frontage, pedestrian-friendly, diverse architecture

#### 2. Residential: 0.60P + 0.20A + 0.10S + 0.10E — Privacy-dominant

[](https://github.com/TheRogueNPC/RogueCities_UrbanSpatialDesigner#2-residential-060p--020a--010s--010e--privacy-dominant)

- **Character**: Housing-focused areas prioritizing privacy and quality of life
- **Buildings**: Single-family homes, townhouses, apartment complexes, condos
- **Height**: 1-6 stories
- **Features**: Setbacks with landscaping, lower density, recreational spaces

#### 3. Commercial: 0.60E + 0.20A + 0.10S + 0.10P — Exposure-dominant

[](https://github.com/TheRogueNPC/RogueCities_UrbanSpatialDesigner#3-commercial-060e--020a--010s--010p--exposure-dominant)

- **Character**: Business and retail areas maximizing visibility and foot traffic
- **Buildings**: Storefronts, shopping centers, office buildings, hotels
- **Height**: 2-20+ stories
- **Features**: Large display windows, prominent signage, minimal setbacks

#### 4. Civic: 0.50E + 0.20A + 0.10S + 0.20P — Exposure with privacy balance

[](https://github.com/TheRogueNPC/RogueCities_UrbanSpatialDesigner#4-civic-050e--020a--010s--020p--exposure-with-privacy-balance)

- **Character**: Public/institutional areas serving community-wide functions
- **Buildings**: Government buildings, libraries, museums, schools, hospitals
- **Height**: 2-12 stories
- **Features**: Monumental architecture, plazas, symbolic design, accessibility

#### 5. Industrial: 0.60S + 0.25A + 0.10E + 0.05P — Serviceability-dominant

[](https://github.com/TheRogueNPC/RogueCities_UrbanSpatialDesigner#5-industrial-060s--025a--010e--005p--serviceability-dominant)

- **Character**: Production, manufacturing, logistics areas requiring heavy service access
- **Buildings**: Warehouses, factories, distribution centers, utilities
- **Height**: 1-4 stories
- **Features**: Large footprints, truck access, rail sidings, utilitarian design

### Practical Application

[](https://github.com/TheRogueNPC/RogueCities_UrbanSpatialDesigner#practical-application)

When processing a parcel, the lot generator:

1. Queries nearest road type using distance-to-segment calculations
2. Retrieves road's frontage profile (A, E, S, P values)
3. Applies district formulas to generate affinity scores
4. Combines with axiom biases and influencer landmarks
5. Uses classification to inform building height, setback, density, ground-floor use

**Emergent Result**: Buildings along highways exhibit industrial/commercial characteristics (high service, low privacy), while cul-de-sacs spawn residential structures (high privacy, low exposure).

---

## Multi-Level City Support

[](https://github.com/TheRogueNPC/RogueCities_UrbanSpatialDesigner#multi-level-city-support)

### Layered Graph Representation

[](https://github.com/TheRogueNPC/RogueCities_UrbanSpatialDesigner#layered-graph-representation)

For cities with bridges, tunnels, and elevated structures, the system uses **layered graphs**:

- **G = (V, E)**: Piecewise-linear graph embedded in ℝ²
- **ℓ : E → ℕ**: Layer assignment oracle
- **Lᵢ = ℓ⁻¹(i)**: Set of edges on layer i

### Portals

[](https://github.com/TheRogueNPC/RogueCities_UrbanSpatialDesigner#portals)

A **portal** is a vertex shared by ≥2 layers (edges adjacent belong to different layers). Conceptually, there are `m` copies of ℝ², one per layer, glued together at portals. This layered space is denoted **L**.

**Key Innovation**: All computational steps occur within the multi-layer space **L**, using portal-aware distance calculations. This ensures topological signatures and distance measurements account for bridges/tunnels rather than artificially flattening the network.

---

## Building & Usage

[](https://github.com/TheRogueNPC/RogueCities_UrbanSpatialDesigner#building--usage)

### Prerequisites

[](https://github.com/TheRogueNPC/RogueCities_UrbanSpatialDesigner#prerequisites)

- **CMake 3.20+**
- **C++20 compiler** (MSVC 2022, GCC 11+, Clang 14+)
- **Git submodules** (all dependencies are vendored in `3rdparty/`)
    - GLM, ImGui, GLFW, magic_enum, sol2, etc.

### Quick Start

[](https://github.com/TheRogueNPC/RogueCities_UrbanSpatialDesigner#quick-start)

```shell
# Clone repository with submodules
git clone --recursive https://github.com/TheRogueNPC/RogueCities_UrbanSpatialDesigner.git
cd RogueCities_UrbanSpatialDesigner

# Build core library
cmake -B build -S .
cmake --build build --target RogueCityCore --config Release

# Run tests (Phase 4)
ctest --test-dir build --output-on-failure

# Build full application (Phase 3+)
cmake --build build --target RogueCityDesigner --config Release
./build/bin/RogueCityDesigner
```

### Development Build (Core-Only)

[](https://github.com/TheRogueNPC/RogueCities_UrbanSpatialDesigner#development-build-core-only)

```shell
# Fast iteration during development
cmake -B build_core -S . -DBUILD_CORE_ONLY=ON
cmake --build build_core --target RogueCityCore --config Release
```

---

## Project Status

[](https://github.com/TheRogueNPC/RogueCities_UrbanSpatialDesigner#project-status)

### Current Phase: **R0 Complete** + **Phase 3 In Progress** (Feb 6, 2026)

[](https://github.com/TheRogueNPC/RogueCities_UrbanSpatialDesigner#current-phase-r0-complete--phase-3-in-progress-feb-6-2026)

|Phase|Status|Description|Target Date|
|---|---|---|---|
|**Phase 1**|✅ **Complete**|Core data layer (Vec2, Tensor2D, AESP types)|Feb 4|
|**Phase 2**|✅ **Complete**|Generators library (tensor fields, roads, districts)|Feb 5-7|
|**Phase 3**|💭 **Functional-WIP**|Minimal ImGui UI (axiom placement, visualization)|Feb 8-10|
|**Phase 4**|⏳ In Progress|Export system (JSON, OBJ, GLTF)|Feb 11-12|
|**Phase 5**|🔜 Planned|MVP polish, presets, documentation|Feb 13-15|
|**R0**|✅ **Complete**|AI-assisted development integration (4 sub-phases)|Feb 6|

**R0 Integration**: AI Bridge + UI Agent + CitySpec + Design Assistant  
**MVP Target**: February 15, 2026

---

## Research References

[](https://github.com/TheRogueNPC/RogueCities_UrbanSpatialDesigner#research-references)

### Core Papers

[](https://github.com/TheRogueNPC/RogueCities_UrbanSpatialDesigner#core-papers)

- Chen et al. (2008) - _Interactive Procedural Street Modeling_
- Parish & Müller (2001) - _Procedural Modeling of Cities_
- Lechner et al. (2003) - _Tensor Fields for Interactive Procedural Content_
- Boeing (2021) - _Street Network Analysis in Urban Form Studies_

### Urban Planning Theory

[](https://github.com/TheRogueNPC/RogueCities_UrbanSpatialDesigner#urban-planning-theory)

- Jacobs, J. (1961) - _The Death and Life of Great American Cities_
- Alexander, C. (1977) - _A Pattern Language: Towns, Buildings, Construction_
- Lynch, K. (1960) - _The Image of the City_

---

## License

[](https://github.com/TheRogueNPC/RogueCities_UrbanSpatialDesigner#license)

MIT License

Copyright (c) 2026 Mariku (TheRogueNPC) / Nefarious Crows Int

---

## Citation

[](https://github.com/TheRogueNPC/RogueCities_UrbanSpatialDesigner#citation)

If you use this research or implementation in academic work, please cite:

```bibtex
@software{roguecity2026,
  author = {Mariku (TheRogueNPC)},
  title = {Rogue City Designer: Tensor Field-Guided Urban Spatial Generation},
  year = {2026},
  publisher = {GitHub},
  url = {https://github.com/TheRogueNPC/RogueCityMVP}
}
```

---

## Contact

[](https://github.com/TheRogueNPC/RogueCities_UrbanSpatialDesigner#contact)

- **Author**: Mariku (TheRogueNPC)
- **Organization**: Nefarious Crows Int
- **Email**: [Team.Crow@yahoo.com](mailto:Team.Crow@yahoo.com)
- **GitHub**: [@TheRogueNPC](https://github.com/TheRogueNPC)

---

_"Cities are defined by their roads. From roads emerge parcels, lots, buildings, and ultimately districts. This structural view from urban morphology emphasizes how these elements create functional urban form."_```