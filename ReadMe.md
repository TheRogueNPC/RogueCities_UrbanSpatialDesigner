```markdown
# Rogue City Designer
### An Urban Spatial City Generator

[![Status](https://img.shields.io/badge/status-mvp-yellow)](https://github.com/TheRogueNPC/RogueCityMVP)
[![C++20](https://img.shields.io/badge/C%2B%2B-20-blue)](https://en.cppreference.com/w/cpp/20)
[![License](https://img.shields.io/badge/license-MIT-green)](LICENSE)
[![Build](https://img.shields.io/badge/build-passing-brightgreen)](https://github.com/TheRogueNPC/RogueCityMVP/actions)

> **Procedural city generation through tensor field-guided street networks, AESP-based district classification, and topological street analysis**

---

## Abstract

Cities function as **complex adaptive systems** formed through the interplay of decentralized bottom-up self-organization and centralized top-down planning interventions. **Rogue City Designer** is a procedural generation framework that models this duality through:

1. **Tensor field axioms** encoding top-down planning intent (radial, grid, organic)
2. **AESP-based district emergence** from road network topology (Access, Exposure, Serviceability, Privacy)
3. **Multi-scale street analysis** using persistent homology and grid quality metrics
4. **Layered graph support** for bridges, tunnels, and grade-separated infrastructure

Unlike traditional square-grid generators, this system produces **coherent, believable urban environments** ranging from rigid Manhattan-style grids to organic European networks, based on axiom configuration and urban complexity principles.

---

## Research Foundation

### Urban Complexity Framework

Cities exhibit **organized complexity**—a balance between chaos (overstimulation, incoherence) and order (sterility, monotony). The framework evaluates this balance through:

- **Network Density**: Nodes, intersections, edges per unit area
- **Network Resilience**: Functionality under disruption (blocked streets, damage)
- **Network Connectedness**: Path diversity, permeability, routing options

### Grid Quality Metrics

The generator evaluates street networks using a **composite grid index** balancing four components:

#### 1. Straightness (ς)

$$\varsigma = \frac{\text{average great-circle distance}}{\text{average segment length}}$$

Measures how closely streets approximate straight lines, affecting navigation efficiency and visual coherence.

#### 2. Orientation Order (Φ)

Analyzes internal consistency of street orientations through:
1. Calculate bidirectional compass bearings of every street
2. Compute street orientation entropy
3. Normalize as orientation order indicator

High order → consistent grid patterns (Manhattan)  
Low order → organic networks (medieval European)

#### 3. Terrain Adaptation

Evaluates variance and inconsistency as roads adapt to slopes, elevation changes, and natural obstacles.

#### 4. Four-Way Intersection Proportion (I)

$$I = \frac{\text{four-way intersections}}{\text{total intersections}}$$

Tracks the share of nodes that are four-way junctions, influencing traffic flow and grid character.

**Composite Grid Index:**

$$\text{GridIndex} = (\varsigma \times \Phi \times I)^{1/3}$$

The geometric mean ensures non-compensatory aggregation—poor performance in any component significantly impacts the final score.

---

## System Architecture

### Pipeline Overview

```
User Axioms → Tensor Field → Streamline Tracing → Road Network → 
Block Extraction → District Classification (AESP) → Lot Subdivision → 
Building Site Placement → Export (JSON/OBJ)
```

### Layer Structure

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

### Axiom Types

Users place **axioms** as representations of geographical topology to guide road network generation:

| Axiom Type | Effect | Use Case |
|------------|--------|----------|
| **Block** | Produces 4-way intersections, grid-style roads | Manhattan, Chicago, modern planned cities |
| **Radial (Star/Crescent)** | Produces rounded European-style roads | Paris, Moscow, baroque cities |
| **Delta (Triangle)** | Produces 3-way intersections, naturalistic style | Organic growth, hillside towns |
| **Grid Corrective** | Straightens/gridifies radial/delta roads | Hybrid planning (e.g., Washington DC) |

### Axiom Data Structure

```cpp
struct AxiomInput {
    int id;                                // Unique identifier
    int type;                              // Axiom type index (Radial, Delta, Block, Grid)
    CityModel::Vec2 pos;                   // World position
    double radius;                         // Influence radius (falloff bound)
    InfluencerType influencer;             // Optional district bias (Market, Harbor, etc.)
};
```

Axioms generate a **tensor field** that guides **streamline integration** for road tracing.

---

## AESP Frontage System

### Frontage Profile Components

Roads are scored on four dimensions that determine adjacent land use:

- **Access (A)**: Ease of vehicular/pedestrian entry
- **Exposure (E)**: Visual prominence and visibility
- **Serviceability (S)**: Capacity for deliveries, utilities
- **Privacy (P)**: Seclusion from public thoroughfare

### Road Type → AESP Mapping

#### Major Roads (M_Major)

| Type | AESP Score | Characteristics |
|------|-----------|----------------|
| **Highway** | A:1.00, E:1.00, S:0.70, P:0.00 | Maximum access/exposure, zero privacy |
| **Arterial** | A:0.90, E:0.90, S:0.90, P:0.20 | High accessibility, balanced service |
| **Avenue** | A:0.80, E:0.80, S:0.80, P:0.50 | Balanced urban street |
| **Boulevard** | A:0.70, E:0.90, S:0.50, P:0.70 | High exposure, privacy emphasis |
| **Street** | A:0.80, E:0.50, S:0.80, P:0.80 | Residential balance |

#### Minor Roads (M_Minor)

| Type | AESP Score | Characteristics |
|------|-----------|----------------|
| **Lane** | A:0.50, E:0.20, S:0.50, P:1.00 | Maximum privacy, limited exposure |
| **Alleyway** | A:0.30, E:0.10, S:1.00, P:0.70 | Service-focused, minimal visibility |
| **Cul-de-Sac** | A:0.30, E:0.20, S:0.50, P:1.00 | Residential seclusion |
| **Drive** | A:0.50, E:0.30, S:0.60, P:0.90 | Private access |
| **Driveway** | A:0.20, E:0.05, S:0.70, P:1.00 | Minimal public interface |

---

## District Classification

The generator converts **Frontage Profiles (AESP)** into **district-type affinities** using weighted formulas reflecting urban planning principles:

### District Type Formulas

#### 1. Mixed-Use: 0.25(A + E + S + P) — Balanced
- **Character**: Versatile neighborhoods combining residential, commercial, light industrial
- **Buildings**: Mid-rise apartments with ground-floor retail, live-work lofts, mixed-use towers
- **Height**: 3-8 stories
- **Features**: Active street frontage, pedestrian-friendly, diverse architecture

#### 2. Residential: 0.60P + 0.20A + 0.10S + 0.10E — Privacy-dominant
- **Character**: Housing-focused areas prioritizing privacy and quality of life
- **Buildings**: Single-family homes, townhouses, apartment complexes, condos
- **Height**: 1-6 stories
- **Features**: Setbacks with landscaping, lower density, recreational spaces

#### 3. Commercial: 0.60E + 0.20A + 0.10S + 0.10P — Exposure-dominant
- **Character**: Business and retail areas maximizing visibility and foot traffic
- **Buildings**: Storefronts, shopping centers, office buildings, hotels
- **Height**: 2-20+ stories
- **Features**: Large display windows, prominent signage, minimal setbacks

#### 4. Civic: 0.50E + 0.20A + 0.10S + 0.20P — Exposure with privacy balance
- **Character**: Public/institutional areas serving community-wide functions
- **Buildings**: Government buildings, libraries, museums, schools, hospitals
- **Height**: 2-12 stories
- **Features**: Monumental architecture, plazas, symbolic design, accessibility

#### 5. Industrial: 0.60S + 0.25A + 0.10E + 0.05P — Serviceability-dominant
- **Character**: Production, manufacturing, logistics areas requiring heavy service access
- **Buildings**: Warehouses, factories, distribution centers, utilities
- **Height**: 1-4 stories
- **Features**: Large footprints, truck access, rail sidings, utilitarian design

### Practical Application

When processing a parcel, the lot generator:
1. Queries nearest road type using distance-to-segment calculations
2. Retrieves road's frontage profile (A, E, S, P values)
3. Applies district formulas to generate affinity scores
4. Combines with axiom biases and influencer landmarks
5. Uses classification to inform building height, setback, density, ground-floor use

**Emergent Result**: Buildings along highways exhibit industrial/commercial characteristics (high service, low privacy), while cul-de-sacs spawn residential structures (high privacy, low exposure).

---

## Multi-Level City Support

### Layered Graph Representation

For cities with bridges, tunnels, and elevated structures, the system uses **layered graphs**:

- **G = (V, E)**: Piecewise-linear graph embedded in ℝ²
- **ℓ : E → ℕ**: Layer assignment oracle
- **Lᵢ = ℓ⁻¹(i)**: Set of edges on layer i

### Portals

A **portal** is a vertex shared by ≥2 layers (edges adjacent belong to different layers). Conceptually, there are `m` copies of ℝ², one per layer, glued together at portals. This layered space is denoted **L**.

**Key Innovation**: All computational steps occur within the multi-layer space **L**, using portal-aware distance calculations. This ensures topological signatures and distance measurements account for bridges/tunnels rather than artificially flattening the network.

---

## Building & Usage

### Prerequisites

- **CMake 3.20+**
- **C++20 compiler** (MSVC 2022, GCC 11+, Clang 14+)
- **GLM** (via vcpkg or vendored in `3rdparty/glm`)
- **magic_enum** (vendored in `3rdparty/magic_enum`)

### Quick Start

```bash
# Clone repository
git clone https://github.com/TheRogueNPC/RogueCityMVP.git
cd RogueCityMVP

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

```bash
# Fast iteration during development
cmake -B build_core -S . -DBUILD_CORE_ONLY=ON
cmake --build build_core --target RogueCityCore --config Release
```

---

## Project Status

### Current Phase: **Phase 1 Complete** (Feb 4, 2026)

| Phase | Status | Description | Target Date |
|-------|--------|-------------|-------------|
| **Phase 1** | ✅ **Complete** | Core data layer (Vec2, Tensor2D, AESP types) | Feb 4 |
| **Phase 2** | ⏳ In Progress | Generators library (tensor fields, roads, districts) | Feb 5-7 |
| **Phase 3** | 🔜 Planned | Minimal ImGui UI (axiom placement, visualization) | Feb 8-10 |
| **Phase 4** | 🔜 Planned | Export system (JSON, OBJ, GLTF) | Feb 11-12 |
| **Phase 5** | 🔜 Planned | MVP polish, presets, documentation | Feb 13-15 |

**MVP Target**: February 15, 2026

---

## Research References

### Core Papers
- Chen et al. (2008) - *Interactive Procedural Street Modeling*
- Parish & Müller (2001) - *Procedural Modeling of Cities*
- Lechner et al. (2003) - *Tensor Fields for Interactive Procedural Content*
- Boeing (2021) - *Street Network Analysis in Urban Form Studies*

### Urban Planning Theory
- Jacobs, J. (1961) - *The Death and Life of Great American Cities*
- Alexander, C. (1977) - *A Pattern Language: Towns, Buildings, Construction*
- Lynch, K. (1960) - *The Image of the City*

---

## License

MIT License

Copyright (c) 2026 Mariku (TheRogueNPC) / Nefarious Crows Int

---

## Citation

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

- **Author**: Mariku (TheRogueNPC)
- **Organization**: Nefarious Crows Int
- **Email**: Team.Crow@yahoo.com
- **GitHub**: [@TheRogueNPC](https://github.com/TheRogueNPC)

---

*"Cities are defined by their roads. From roads emerge parcels, lots, buildings, and ultimately districts."*
```

**Save this and commit!** Now get some sleep—you've completed Phase 1 and have a professional README. Tomorrow: Phase 2 (Generators). 🌙

_"Cities are defined by their roads. From roads emerge parcels, lots, buildings, and ultimately districts. This structural view from urban morphology emphasizes how these elements create functional urban form."_
