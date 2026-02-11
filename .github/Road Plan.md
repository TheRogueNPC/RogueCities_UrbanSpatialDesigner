
**StreetSweeper → Data Model → Scoring → Policies → Implementation Steps → Code Scaffolds**

---
# ***For Agents***
Goal: Implament StreetSweeper.


```json
{
  "title": "RogueCities — RoadMaff",
  "sections": [
    {
      "id": "StreetSweeper",
      "label": "StreetSweeper",
      "summary": "End-to-end road generation order from tensor field to layered graph.",
      "core_thesis": [
        "Tensor streamlines propose geometry.",
        "The city is stored as a graph.",
        "Intersections are resolved outcomes of hierarchy, flow, and policy."
      ],
      "StreetSweeper_steps": [
        "trace",
        "snap",
        "split",
        "graph",
        "classify",
        "simplify",
        "score",
        "control",
        "layer/ramps"
      ],
      "generator_order": [
        "Build tensor field (axioms + basis fields)",
        "Trace road candidates (tensor → polylines)",
        "Node roads into a graph (snap/split/build topology)",
        "Simplify topology (weld, micro-edge removal, degree-2 collapse)",
        "Analyze + classify (topology-derived major/minor signals)",
        "Flow + controls (frontage-style scoring → stop/signal/roundabout)",
        "Verticality pass (layering + ramps/portals under policy)"
      ]
    },
    {
      "id": "data_model",
      "label": "Data Model",
      "summary": "Candidate polylines promoted into a layered road graph with flow payloads.",
      "candidate_geometry": {
        "concept": "Traced roads are candidate polylines + trace metadata; topology is not yet committed.",
        "fields": [
          "polyline points",
          "curvature",
          "slope",
          "flood",
          "termination_reason",
          "type_hint",
          "layer_hint"
        ]
      },
      "graph_topology": {
        "concept": "Authoritative representation as vertices + edges.",
        "vertex_kinds": [
          "Normal",
          "DeadEnd",
          "Intersection",
          "RoundaboutCenter",
          "Portal"
        ],
        "edge_fields": [
          "RoadType",
          "layer_id",
          "shape (render polyline)",
          "flow payload (FlowStats)"
        ],
        "vertex_fields": [
          "position",
          "layer_id",
          "adjacent edge list"
        ]
      },
      "intersection_structures": {
        "segment_grid": {
          "purpose": "Local O(k) neighborhood intersection tests (avoid N²).",
          "stores": [
            "segment endpoints",
            "edge/candidate id",
            "layer_id"
          ]
        },
        "road_noding": {
          "responsibilities": [
            "snap near-misses by type-specific snap_radius",
            "split at intersections",
            "resolve overlaps by hierarchy",
            "build welded vertices/edges with tolerances"
          ],
          "deterministic_rules": [
            "crossing becomes intersection only if same layer_id",
            "near-misses snap via typed snap_radius",
            "overlaps merge/trim with higher class winning"
          ]
        }
      }
    },
    {
      "id": "scoring",
      "label": "Scoring",
      "summary": "Deterministic frontage-style scoring for edge flow and node control.",
      "edge_flow": {
        "effective_speed_equation": "V_eff = V_base · district_speed_mult · zone_speed_mult · curvature_mult · slope_mult · intersection_density_mult · control_delay_mult",
        "flow_score_inputs": [
          "edge length",
          "endpoint degrees",
          "sampled betweenness-like centrality",
          "proximity to axioms/district cores (future)"
        ]
      },
      "node_demand_risk": {
        "demand_equation": "D(n) = sum( flow_score(edge_i) · district_activity(n) · frontage_pressure(n) )",
        "risk_equation": "R(n) = (sum V_i^2) · conflict_complexity(n) · sight_penalty(n)",
        "conflict_complexity_inputs": [
          "degree (3/4/5+ way)",
          "angle sanity (avoid acute)",
          "turn penalty from simplest-path cost"
        ]
      },
      "control_feedback": {
        "concept": "Chosen control devices modify control_delay_mult, feeding back into V_eff and flow_score."
      }
    },
    {
      "id": "policies",
      "label": "Policies",
      "summary": "Data-driven knobs turning one road engine into multiple styles.",
      "road_type_rules": {
        "description": "Per-road-type parameters in a single config source.",
        "parameters": [
          "snap radius",
          "weld radius",
          "min separation",
          "min edge length",
          "node spacing rhythm",
          "base speed (V_base)",
          "capacity proxy (Cap_base)",
          "access control",
          "grade separation preference"
        ]
      },
      "simplification_policy": {
        "steps": [
          "weld vertices within radius",
          "remove micro-edges under length threshold",
          "collapse degree-2 vertices if near-straight (angle threshold)"
        ]
      },
      "classification_policy": {
        "description": "Topology-derived classification; graph-driven.",
        "algorithms": [
          "shortest path",
          "simplest path (turn-penalized)",
          "sampled betweenness approximation"
        ]
      },
      "control_ladder": {
        "description": "Deterministic control selection from D(n), R(n), and road class.",
        "order": [
          "Very low D & low R → Uncontrolled",
          "Low D & moderate R → Yield / Two-way stop",
          "Moderate D, balanced → All-way stop",
          "High D or high R → Signal",
          "High D & high R with space → Roundabout",
          "Extreme D or access-controlled → Interchange / Grade separation"
        ],
        "hard_rule": "Intersection must obey the fastest incident road’s design speed."
      },
      "verticality_policy": {
        "principles": [
          "Most logic stays 2D; layer_id is the verticality primitive.",
          "Crossings only create intersections when layer_id matches.",
          "Portals connect layers intentionally (few, explicit)."
        ],
        "grade_separation_decision": {
          "if": "at_grade_cost > grade_sep_cost and policy allows",
          "then": "assign different layers, no at-grade intersection",
          "else": "create at-grade intersection + control device",
          "at_grade_cost_terms": [
            "delay",
            "conflict",
            "frontage disruption",
            "signal penalty"
          ],
          "grade_sep_cost_terms": [
            "construction",
            "slope",
            "ramp complexity",
            "visual intrusion"
          ]
        }
      },
      "templates_policy": {
        "rule": "Templates beat improvisation for major junctions.",
        "archetypes": [
          "diamond",
          "folded diamond",
          "cloverleaf",
          "directional T/stack"
        ]
      },
      "furniture_greenspace_policy": {
        "emitted_polygons": [
          "paved intersection area",
          "keep-out islands/medians",
          "supports (piers/columns)",
          "buffer polygons as greenspace candidates"
        ],
        "greenspace_scoring_inputs": [
          "visibility",
          "adjacency to high-frontage edges",
          "district policy",
          "noise/shadow from elevated segments"
        ]
      },
      "style_profiles": {
        "concept": "Realistic vs futuristic as profile swap, not code change.",
        "examples": {
          "realistic_CA": [
            "signals common",
            "grade separation on major-major",
            "wide buffers/interchanges",
            "strict highway access control"
          ],
          "futuristic_megacity": [
            "more layers (2–5)",
            "more portals/elevated collectors",
            "green decks and uninterrupted corridors"
          ]
        }
      },
      "known_fix_policy": {
        "note": "Tensor field fade bug: apply bilinear clamp + scale-then-add fix in TensorFieldGenerator."
      }
    },
    {
      "id": "implementation_steps",
      "label": "Implementation Steps",
      "summary": "Ship-safe phased rollout, each step independently useful.",
      "phases": [
        {
          "phase": 1,
          "name": "No visual crossings without a vertex",
          "goals": [
            "SegmentGridStorage",
            "intersection detection",
            "endpoint snapping",
            "split polylines at intersections",
            "build graph"
          ]
        },
        {
          "phase": 2,
          "name": "Classification + topology sanity",
          "goals": [
            "GraphAlgorithms (betweenness approx, path helpers)",
            "RoadClassifier uses topology scores",
            "GraphSimplify (weld, micro-edge removal, degree-2 collapse)"
          ]
        },
        {
          "phase": 3,
          "name": "Flow + control devices",
          "goals": [
            "per-road-type FlowStats defaults",
            "node D/R scoring",
            "control ladder assignment",
            "control delay feeds back into V_eff"
          ]
        },
        {
          "phase": 4,
          "name": "Multi-level + portals",
          "goals": [
            "enforce layer_id in noder",
            "grade separation on major-major crossings",
            "Portal vertex kind + basic ramps"
          ]
        },
        {
          "phase": 5,
          "name": "Interchange templates + furniture/greenspace",
          "goals": [
            "archetype placement",
            "keep-out + supports",
            "buffer scoring for greens"
          ]
        }
      ]
    },
    {
      "id": "code_scaffolds",
      "label": "Code Scaffolds",
      "summary": "Header/source entrypoints and config schema for the road system.",
      "files": [
        {
          "name": "PolylineRoadCandidate.hpp",
          "path": "generators/include/RogueCity/Generators/Roads/PolylineRoadCandidate.hpp",
          "role": "Defines traced candidate polylines + metadata."
        },
        {
          "name": "Graph.hpp (upgrade)",
          "path": "generators/include/RogueCity/Generators/Urban/Graph.hpp",
          "role": "Layered road graph with FlowStats and ControlType."
        },
        {
          "name": "SegmentGridStorage.hpp/.cpp",
          "path": "generators/include/RogueCity/Generators/Roads/SegmentGridStorage.hpp",
          "role": "Spatial grid index for candidate/edge segments."
        },
        {
          "name": "RoadNoder.hpp/.cpp",
          "path": "generators/include/RogueCity/Generators/Roads/RoadNoder.hpp",
          "role": "Snap/split candidates into a welded graph."
        },
        {
          "name": "GraphSimplify.hpp/.cpp",
          "path": "generators/include/RogueCity/Generators/Roads/GraphSimplify.hpp",
          "role": "Topology cleanup and degree-2 collapse."
        },
        {
          "name": "GraphAlgorithms.hpp/.cpp",
          "path": "generators/include/RogueCity/Generators/Urban/GraphAlgorithms.hpp",
          "role": "Shortest/simplest paths and sampled centrality."
        },
        {
          "name": "FlowAndControl.hpp/.cpp",
          "path": "generators/src/Generators/Roads/FlowAndControl.cpp",
          "role": "Apply type defaults, compute V_eff, D/R, controls, and delay feedback."
        },
        {
          "name": "Verticality.hpp/.cpp",
          "path": "generators/src/Generators/Roads/Verticality.cpp",
          "role": "Grade separation decisions, layer_id, portals, ramps."
        }
      ],
      "config_schema": {
        "road_type_defaults": "Per-road-type physical and topological tolerances.",
        "district_modifiers": "Multipliers for speed, activity, frontage, visual cost.",
        "control_thresholds": "D/R bands for ladder selection.",
        "verticality_policy": "Layer limits and grade-sep biases.",
        "template_costs": "Cost heuristics for signals, roundabouts, grade-sep, interchanges."
      },
      "guardrails": [
        "UI/picking queries topology/index, never generators directly.",
        "Intersections are negotiated outcomes of hierarchy, speed, and flow.",
        "Layering stays cheap by using integer layer_id + portals, not full 3D.",
        "Templates are preferred over ad-hoc geometry for complex junctions."
      ]
    }
  ]
}
```
---

## 1. StreetSweeper (The StreetSweeper Protocol)

Authoritative road StreetSweeper, in generator order:

1. Build tensor field (axioms + basis fields)
2. Trace road **candidates** (tensor → polylines)
3. Node candidates into a **graph** (snap → split → weld)
4. Simplify topology (micro-edge removal, degree‑2 collapse)
5. Classify edges (topology + AESP + district/zone context)
6. Score flow on edges (`V_eff`, `flow_score`)
7. Score nodes (Demand `D`, Risk `R`)
8. Assign controls and verticality (control ladder + `layer_id`/portals)

This plugs directly into the existing “User Axioms → Tensor → Roads → Blocks → Districts…” StreetSweeper described in your README.

---

## 2. Data Model (What Each Pass Owns)

### Candidate Geometry (trace)

- `PolylineRoadCandidate`: polyline + `TraceMeta` (curvature, slope, flood, termination reason, type/layer hints).
- Candidates are **proposals**; no topology yet.

### Graph (authoritative city road representation)

- `Graph::Vertex`: `pos`, `layer_id`, `VertexKind` (`Normal`, `DeadEnd`, `Intersection`, `RoundaboutCenter`, `Portal`), adjacency list.
- `Graph::Edge`: `a`, `b`, `RoadType`, `layer_id`, `shape` (polyline), `length`, `FlowStats`.
- `FlowStats`: `v_base`, `cap_base`, `access_control`, `v_eff`, `flow_score`.
- `ControlType` on vertices: `Uncontrolled`, `Yield`, `Stop`, `Signal`, `Roundabout`, `GradeSep`, `Interchange`.

### Intersection / Noding Support

- `SegmentGridStorage`: grid spatial index for segments with `layer_id`, enabling O(k) local intersection checks instead of N².
- `RoadNoder`: turns candidates into a welded graph respecting snap radii, min separations, and layer rules.

---

## 3. Scoring (Deterministic, Frontier-Style)

### Edge Flow

Per edge:

- Start from `RoadType` defaults (`V_base`, capacity, access control).
- Apply multiplicative modifiers: district, zone, curvature, slope, intersection density, control delay.
- Compute `flow_score` using length, degree of endpoints, sampled centrality, proximity to axioms/cores (later).

### Node Demand & Risk

For each vertex:

- Demand (D(n)): sum of incident `flow_score` × `district_activity` × `frontage_pressure`.
- Risk (R(n)): sum of squared speeds × conflict complexity × sight penalty.

These use AESP and district concepts already present in your docs, so they stay consistent with frontage/district logic.

---

## 4. Policies (All Data-Driven)

### Road Type Policy

Per `RoadType` JSON profile:

- `snapRadius`, `weldRadius`, `minSeparation`, `minEdgeLength`, `minNodeSpacing`.
- `V_base`, `Cap_base`, `AccessControl`, `SignalAllowed`, `RoundaboutAllowed`, `GradeSepPreferred`.

### Simplification Policy

- Global weld radius.
- Micro-edge removal threshold.
- Degree‑2 collapse angle (near‑straight).

### Classification Policy

- Uses centrality/simplest‑path approximations to separate Highway / Arterial / Street / Lane from the same graph.

### Control Device Ladder

Given `D(n)`, `R(n)`, and highest incident class:

1. Very low D & R → Uncontrolled
2. Low D, moderate R → Yield / Two‑way stop
3. Moderate D, balanced → All‑way stop
4. High D or R → Signal
5. High D & R with space → Roundabout
6. Extreme D or access‑controlled majors → Interchange / Grade separation

Control choice feeds back into `control_delay_mult` → `V_eff` → `flow_score`.

### Verticality Policy

- Intersections only form on same `layer_id`.
- Grade separation when `grade_sep_cost < at_grade_cost` and policy allows; otherwise at‑grade with control.
- Portals are explicit vertex kind connecting layers.

---

## 5. Implementation Steps (Ship‑Safe Milestones)

**Phase 1 – No visual crossings without vertices**

- Implement `PolylineRoadCandidate`, `SegmentGridStorage`, `RoadNoder`, and `Graph` upgrade.
- Ensure every same‑layer crossing gets a node and split edges.

**Phase 2 – Classification + simplification**

- Add `GraphAlgorithms` for centrality/paths.
- Implement `GraphSimplify` (weld, micro‑edge removal, degree‑2 collapse).
- Populate `RoadType` on edges using topology + AESP.

**Phase 3 – Flow & controls**

- `FlowAndControl`: apply type defaults, compute `V_eff`, `flow_score`, `D/R`, control ladder, and delay feedback.

**Phase 4 – Multi‑layer + portals**

- Enforce `layer_id` in noder and intersection checks.
- Add basic grade‑sep decisions and portal nodes for major–major crossings.

**Phase 5 – Templates & furniture**

- Archetypal interchanges/roundabouts emitting roadway + furniture/greenspace polygons.

---

## 6. Integration Hooks

- Road StreetSweeper lives under `generators/` and writes into `Graph`, then to `EditorGlobalState`.
- Viewport never walks the graph; it reads pre‑baked `FlowStats`, `ControlType`, and lineage from the **Viewport Index** we defined earlier.


---
# RogueCities — RoadMaff

## StreetSweeper

### Core Thesis

- **Tensor streamlines propose geometry.**
    
- **The city is stored as a graph.**
    
- **Intersections are not “placed” — they are resolved outcomes of hierarchy + flow + policy.**
    

### Non-negotiable StreetSweeper

`trace → snap → split → graph → classify → simplify → score → control → layer/ramps`

### Road StreetSweeper Integration (City Generator)

High-level orchestration (the “one true order” your generator should follow):


   
2. **Build tensor field** (axioms + basis fields)
    
3. **Trace road candidates** (tensor → polylines)
    
4. **Node roads into a graph** (snap/split/build topology)
    
5. **Simplify topology** (weld, micro-edge removal, degree-2 collapse)
    
6. **Analyze + classify** (topology-derived major/minor signals)
    
7. **Flow + controls** (frontage-style scoring → stop/signal/roundabout)
    
8. **Verticality pass** (layering + ramps/portals under policy)
    

---

## Data Model

### Candidate Geometry (trace output)

A traced road is a **candidate polyline + trace metadata** (curvature, slope, flood, termination reason, hints for type/layer).

Key idea: candidates are “proposals” — **topology becomes real only after noding**.

### Graph Topology (authoritative city representation)

Upgrade the graph to explicit **vertices + edges** with:

- `layer_id` on both vertices/edges (multi-level without forcing “3D everywhere”)
    
- typed edges (`RoadType`)
    
- adjacency stored at vertices
    
- `Edge.shape` for render geometry payload
    
- `Edge.flow` payload for scoring/controls
    

Vertex kinds include:

- `Normal`, `DeadEnd`, `Intersection`, `RoundaboutCenter`, **`Portal`** (layer connector)
    

### Intersection StreetSweeper Data Structures

#### Spatial Index: Segment Grid

Intersection work must be local-query based (**O(k)** neighborhood checks) and never devolve into N².

Segment grid stores:

- segment endpoints
    
- associated edge/candidate id
    
- `layer_id` for “same-layer only intersects” enforcement
    

#### Road Noding (candidates → snapped/split graph)

The noder is responsible for:

- snapping near-misses (per road type)
    
- splitting at intersections
    
- resolving overlaps by hierarchy
    
- building graph vertices/edges with consistent welding/tolerances
    

Deterministic rules:

- crossing becomes an intersection **only if same `layer_id`**
    
- near-misses snap via `snap_radius` (typed)
    
- overlaps merge/trim via hierarchy (higher class wins)
    

---

## Scoring

### 1) Edge Flow Scoring (frontage-style)

Each edge starts from **road type defaults** and gets an effective speed & demand proxy.

**Effective speed (deterministic multiplier chain):**  
[  
V_{eff} = V_{base}  
\cdot district_speed_mult  
\cdot zone_speed_mult  
\cdot curvature_mult  
\cdot slope_mult  
\cdot intersection_density_mult  
\cdot control_delay_mult  
]  

**Flow score starts from topology proxies** (until you have richer demand simulation):

- length
    
- endpoint degrees
    
- betweenness-ish centrality (sampled)
    
- proximity to axioms / district cores (later)
    

### 2) Node Demand (D) + Risk (R)

You do _not_ place intersections. You detect + build nodes, then score them like frontage pressure.

**Demand proxy:**  
[  
D(n) = \sum flow_score(edge_i)\cdot district_activity(n)\cdot frontage_pressure(n)  
]  

**Risk proxy:**  
[  
R(n) = \left(\sum V_i^2\right)\cdot conflict_complexity(n)\cdot sight_penalty(n)  
]  

Conflict complexity can incorporate:

- degree (3-way vs 4-way vs >4)
    
- angle sanity (acute intersections are costly/weird)
    
- turn penalty derived from “simplest path” cost model
    

### 3) Control Device Scoring Feedback

Controls don’t just label intersections — they push back into the network by affecting `control_delay_mult`, which feeds back into `V_eff`.

---

## Policies

### A) Road Type Rules + Tolerances (data-driven)

Do not hardcode tolerances; keep them in one config source so “historic vs futuristic” becomes **profile swap**, not code swamp.

Per-road-type params include:

- snap radius, weld radius
    
- min separation, min edge length
    
- node spacing rhythm (signals/interchanges)
    
- base speed/capacity/access control
    
- grade separation preference
    

### B) Simplification Policy

After noding:

- weld vertices within radius
    
- remove micro-edges under length threshold
    
- collapse degree-2 vertices if near-straight (angle threshold)
    

### C) Classification Policy (topology-derived)

Classification should be graph-driven; use lightweight algorithms:

- shortest path
    
- simplest path (turn penalty)
    
- sampled betweenness approximation
    

### D) Control Device Ladder (deterministic)

Given `D(n)`, `R(n)`, plus incident road class:

- low D, low R → Uncontrolled
    
- low D, moderate R → Yield / Two-way stop
    
- moderate D, balanced approaches → All-way stop
    
- high D or high R → Signal (if allowed)
    
- high D + moderate speeds + space → Roundabout
    
- extreme speeds + access control → Grade separation / interchange
    

Hard rule:

> **The intersection must obey the fastest road’s design speed. Physics vetoes cute geometry.**

### E) Verticality Policy (layers + portals)

Keep most logic 2D; use `layer_id` as the core verticality primitive:

- crossings intersect only if same layer
    
- portals connect layers intentionally (few, expensive, explicit)
    

**Grade separation decision policy:**  
If `at_grade_cost > grade_sep_cost` and policy allows:

- assign different layers (no intersection)  
    Else:
    
- create at-grade intersection + control device
    

Where:

- `at_grade_cost` = delay + conflict + frontage disruption + signal penalty
    
- `grade_sep_cost` = construction + slope + ramp complexity + visual intrusion
    

### F) Template Policy: “templates beat improvisation”

Interchanges should be archetype templates (diamond, cloverleaf, directional T/stack, folded diamond), then fit to terrain/corridor/ROW budget.

### G) Structural Furniture / Greenspace Policy

Intersection templates emit polygons:

- paved polygon
    
- keep-out islands/medians
    
- supports (piers/columns)
    
- leftover buffers → greenspace candidates  
    Greenspace scoring: visibility, adjacency to high-frontage edges, district policy, noise/shadow from elevated segments.
    

### H) “Realistic vs Futuristic” is a Policy Profile

Single generator; different weights.

- Realistic CA-ish: signals common, grade separation on major-major, wide buffers/interchanges, strict access control on highways
    
- Futuristic megacity: more layers (2–5), more portals, elevated collectors, green decks, uninterrupted corridors
    

### I) Known Fix Policy (Tensor field fade bug)

Apply bilinear clamp + scale-then-add fix in:  
`generators/src/Generators/Tensors/TensorFieldGenerator.cpp`

---

## Implementation Steps

### Ship-it Ladder (stays shippable)

**Phase 1 — “Roads never visually cross without a vertex”**

- SegmentGridStorage
    
- intersection detection
    
- endpoint snapping
    
- split polyline at intersections
    
- build graph
    

**Phase 2 — Classification + topology sanity**

- GraphAlgorithms (betweenness approx)
    
- RoadClassifier uses topology scores
    
- Simplify pass (weld, micro-edge removal, degree-2 collapse)
    

**Phase 3 — Flow + control devices**

- per-road-type FlowStats
    
- node D/R scoring
    
- control ladder assignment
    
- control delay feeds back into `v_eff`
    

**Phase 4 — Multi-level (2 layers) + portals**

- enforce `layer_id`
    
- grade separation on major-major crossings
    
- portal vertex kind + simple ramps as connectors
    

**Phase 5 — Interchange templates + furniture/greenspace**

- archetype placement
    
- keep-out + supports
    
- buffer scoring for greens
    

---

## Code Scaffolds

> Notes: kept your file paths and intent; this section is the “Codex-ready skeleton map” of what exists/gets added.

### 1) Candidate Geometry

- **NEW** `generators/include/RogueCity/Generators/Roads/PolylineRoadCandidate.hpp`
    

```cpp
// File: generators/include/RogueCity/Generators/Roads/PolylineRoadCandidate.hpp
#pragma once

#include "RogueCity/Core/Types.hpp"
#include "RogueCity/Generators/Roads/RoadTypes.hpp"
#include <vector>

namespace RogueCity::Generators::Roads {

    enum class TerminationReason {
        HitBoundary,
        HitNoBuild,
        ConstraintViolation,
        MaxLength,
        DegenerateTensor,
        IntersectedRoad,
        SnappedToNetwork,
        BudgetExhausted
    };

    struct TraceMeta {
        float avg_curvature = 0.0f;
        float avg_tensor_strength = 0.0f;
        float avg_slope = 0.0f;
        float avg_flood = 0.0f;
        TerminationReason reason = TerminationReason::MaxLength;
    };

    struct PolylineRoadCandidate {
        RoadType type_hint = RoadType::Street;
        bool is_major_hint = false;

        int seed_id = -1;
        int layer_hint = 0; // supports multi-level later

        std::vector<Core::Vec2> pts;
        TraceMeta meta;
    };

} // namespace RogueCity::Generators::Roads
```

### 2) Graph Topology Upgrade

- **MODIFY** `generators/include/RogueCity/Generators/Urban/Graph.hpp`
    

```cpp
// File: generators/include/RogueCity/Generators/Urban/Graph.hpp
#pragma once

#include "RogueCity/Core/Types.hpp"
#include "RogueCity/Generators/Roads/RoadTypes.hpp"
#include <vector>

namespace RogueCity::Generators::Urban {

    using VertexID = uint32_t;
    using EdgeID   = uint32_t;

    enum class VertexKind {
        Normal,
        DeadEnd,
        Intersection,
        RoundaboutCenter,
        Portal   // layer connector node
    };

    struct FlowStats {
        float v_base = 0.0f;         // design speed
        float cap_base = 0.0f;       // capacity proxy
        float access_control = 0.0f; // 0..1 (freeway high)
        float v_eff = 0.0f;          // computed after modifiers + controls
        float flow_score = 0.0f;     // demand proxy (like frontage score)
    };

    struct Vertex {
        Core::Vec2 pos{};
        VertexKind kind = VertexKind::Normal;
        int layer_id = 0;                  // planar graph per layer
        std::vector<EdgeID> edges;         // adjacency
    };

    struct Edge {
        VertexID a = 0;
        VertexID b = 0;

        Roads::RoadType type = Roads::RoadType::Street;
        int layer_id = 0;

        float length = 0.0f;

        // Render/shape payload (from snapped/split polyline)
        std::vector<Core::Vec2> shape;

        // Flow payload
        FlowStats flow{};
    };

    class Graph {
    public:
        VertexID addVertex(const Vertex& v);
        EdgeID   addEdge(const Edge& e);

        const std::vector<Vertex>& vertices() const { return vertices_; }
        const std::vector<Edge>& edges() const { return edges_; }

        const Vertex* getVertex(VertexID id) const;
        const Edge*   getEdge(EdgeID id) const;

        Vertex* getVertexMutable(VertexID id);
        Edge*   getEdgeMutable(EdgeID id);

    private:
        std::vector<Vertex> vertices_;
        std::vector<Edge> edges_;
    };

} // namespace RogueCity::Generators::Urban
```

### 3) Segment Spatial Index

- **NEW** `generators/include/RogueCity/Generators/Roads/SegmentGridStorage.hpp`
    

```cpp
// File: generators/include/RogueCity/Generators/Roads/SegmentGridStorage.hpp
#pragma once

#include "RogueCity/Core/Types.hpp"
#include <vector>

namespace RogueCity::Generators::Roads {

    struct SegmentRef {
        uint32_t edge_id; // or candidate id during build
        Core::Vec2 a;
        Core::Vec2 b;
        int layer_id = 0;
    };

    class SegmentGridStorage {
    public:
        SegmentGridStorage(int w, int h, float cell_size);

        void clear();

        void insert(const SegmentRef& seg);
        void queryRadius(const Core::Vec2& p, float r, int layer_id, std::vector<SegmentRef>& out) const;

    private:
        int w_, h_;
        float cell_size_;

        std::vector<std::vector<SegmentRef>> cells_;

        int cellX(float x) const;
        int cellY(float y) const;
        bool inBounds(int cx, int cy) const;
        int idx(int cx, int cy) const;
    };

} // namespace RogueCity::Generators::Roads
```

### 4) Road Noder (snap/split/build graph)

- **NEW** `generators/include/RogueCity/Generators/Roads/RoadNoder.hpp`
    

```cpp
// File: generators/include/RogueCity/Generators/Roads/RoadNoder.hpp
#pragma once

#include "RogueCity/Generators/Roads/PolylineRoadCandidate.hpp"
#include "RogueCity/Generators/Roads/SegmentGridStorage.hpp"
#include "RogueCity/Generators/Urban/Graph.hpp"
#include <vector>

namespace RogueCity::Generators::Roads {

    enum class IntersectionControl {
        Uncontrolled,
        Yield,
        TwoWayStop,
        AllWayStop,
        Signal,
        Roundabout,
        GradeSeparated, // no connection
        Interchange     // ramps/portals cluster
    };

    struct RoadTypeParams {
        float snap_radius = 8.0f;
        float weld_radius = 5.0f;
        float min_separation = 60.0f;
        float min_edge_length = 40.0f;

        float min_node_spacing = 50.0f; // signals/interchanges rhythm
        float v_base = 12.0f;
        float cap_base = 1.0f;
        float access_control = 0.0f;     // 0..1
        bool  signal_allowed = true;
        float grade_sep_preferred = 0.0f;// 0..1
    };

    struct NoderConfig {
        std::vector<RoadTypeParams> type_params; // indexed by RoadType enum value
        float global_weld_radius = 5.0f;
        float global_min_edge_length = 20.0f;
        int max_layers = 2;
    };

    class RoadNoder {
    public:
        explicit RoadNoder(NoderConfig cfg);

        void buildGraph(
            const std::vector<PolylineRoadCandidate>& candidates,
            Urban::Graph& out_graph
        );

    private:
        NoderConfig cfg_;
        SegmentGridStorage seg_index_;

        void snapAndSplitCandidate(const PolylineRoadCandidate& c, Urban::Graph& g);

        Urban::VertexID getOrCreateVertex(
            Urban::Graph& g,
            const Core::Vec2& p,
            int layer_id,
            float weld_radius
        );

        bool segmentIntersect(
            const Core::Vec2& a0, const Core::Vec2& a1,
            const Core::Vec2& b0, const Core::Vec2& b1,
            Core::Vec2& out_p,
            float tol
        ) const;

        float polylineLength(const std::vector<Core::Vec2>& pts) const;
    };

} // namespace RogueCity::Generators::Roads
```

### 5) Graph Simplification

- **NEW** `generators/include/RogueCity/Generators/Roads/GraphSimplify.hpp`
    

```cpp
// File: generators/include/RogueCity/Generators/Roads/GraphSimplify.hpp
#pragma once

#include "RogueCity/Generators/Urban/Graph.hpp"

namespace RogueCity::Generators::Roads {

    struct SimplifyConfig {
        float weld_radius = 5.0f;
        float min_edge_length = 20.0f;
        float collapse_angle_deg = 10.0f; // near-straight
    };

    void simplifyGraph(Urban::Graph& g, const SimplifyConfig& cfg);

} // namespace RogueCity::Generators::Roads
```



Data-Driven Profile (JSON) — the “frontage config” equivalent

This is what makes “realistic vs futuristic” a style swap.
# Schema (authoritative knobs)

```json
{
  "road_type_defaults": {
    "Highway":   { "V_base": 33.0, "Cap_base": 6.0, "AccessControl": 1.0, "SignalAllowed": false, "RoundaboutAllowed": false, "GradeSepPreferred": 1.0,
                  "MinNodeSpacing": 500.0, "snapRadius": 12.0, "weldRadius": 8.0, "minSeparation": 200.0, "minEdgeLength": 120.0 },
    "Arterial":  { "V_base": 22.0, "Cap_base": 4.0, "AccessControl": 0.6, "SignalAllowed": true,  "RoundaboutAllowed": true,  "GradeSepPreferred": 0.6,
                  "MinNodeSpacing": 250.0, "snapRadius": 10.0, "weldRadius": 6.0, "minSeparation": 120.0, "minEdgeLength": 80.0 },
    "Street":    { "V_base": 13.0, "Cap_base": 2.0, "AccessControl": 0.2, "SignalAllowed": true,  "RoundaboutAllowed": true,  "GradeSepPreferred": 0.1,
                  "MinNodeSpacing": 120.0, "snapRadius": 8.0,  "weldRadius": 5.0, "minSeparation": 60.0,  "minEdgeLength": 40.0 },
    "Lane":      { "V_base": 8.0,  "Cap_base": 1.0, "AccessControl": 0.0, "SignalAllowed": false, "RoundaboutAllowed": false, "GradeSepPreferred": 0.0,
                  "MinNodeSpacing": 60.0,  "snapRadius": 6.0,  "weldRadius": 4.0, "minSeparation": 30.0,  "minEdgeLength": 20.0 }
  },

  "district_modifiers": {
    "DowntownCore": { "speed_mult": 0.65, "activity_mult": 1.35, "frontage_pressure_mult": 1.40, "visual_intrusion_penalty": 1.25 },
    "Residential":  { "speed_mult": 0.85, "activity_mult": 0.85, "frontage_pressure_mult": 0.95, "visual_intrusion_penalty": 1.00 },
    "Industrial":   { "speed_mult": 0.95, "activity_mult": 1.05, "frontage_pressure_mult": 0.80, "visual_intrusion_penalty": 0.85 },
    "Megastructure":{ "speed_mult": 1.20, "activity_mult": 1.10, "frontage_pressure_mult": 1.10, "visual_intrusion_penalty": 0.60 }
  },

  "control_thresholds": {
    "uncontrolled": { "D_max": 10.0,  "R_max": 10.0 },
    "yield_stop":   { "D_max": 25.0,  "R_max": 25.0 },
    "all_way_stop": { "D_max": 45.0,  "R_max": 45.0 },
    "signal":       { "D_max": 75.0,  "R_max": 75.0 },
    "roundabout":   { "D_min": 60.0,  "R_min": 60.0 },
    "interchange":  { "D_min": 120.0, "R_min": 120.0 }
  },

  "verticality_policy": {
    "max_layers": 3,
    "grade_sep_bias": 1.0,
    "ramp_cost_mult": 1.0,
    "visual_intrusion_weight": 1.0,
    "allow_grade_sep_without_ramps": true
  },

  "template_costs": {
    "at_grade_signal":   { "base": 30.0, "space": 1.0 },
    "roundabout":        { "base": 45.0, "space": 1.4 },
    "grade_separation":  { "base": 80.0, "space": 1.2 },
    "interchange_basic": { "base": 140.0,"space": 2.0 }
  }
}
```



### Graph metadata

```cpp
// File: generators/include/RogueCity/Generators/Urban/Graph.hpp (UPGRADE)

// Why: flow/control/verticality all need typed edges + nodes.
// Added: layer_id, flow stats, control device selection.

enum class ControlType : uint8_t { None, Uncontrolled, Yield, TwoWayStop, AllWayStop, Signal, Roundabout, GradeSep, Interchange };

struct FlowStats {
    float v_base = 0.0f;
    float cap_base = 0.0f;
    float access_control = 0.0f;

    float v_eff = 0.0f;       // computed
    float flow_score = 0.0f;  // computed
};

struct Vertex {
    Core::Vec2 pos;
    std::vector<EdgeID> edges;
    int layer_id = 0;

    // computed
    float demand_D = 0.0f;
    float risk_R = 0.0f;
    ControlType control = ControlType::None;
};

struct Edge {
    VertexID a, b;
    RoadType type;
    int layer_id = 0;

    std::vector<Core::Vec2> shape;
    float length = 0.0f;

    FlowStats flow;
};
```

# Flow/control evaluation entrypoints
```cpp
// File: generators/src/Generators/Roads/RoadClassifier.cpp (EXTEND)

// Why: classification shouldn't just be topology/length; it should also attach flow defaults
// and compute V_eff using district/zone sampling.

void applyRoadTypeDefaults(Graph& g, const RoadProfileDB& db);
void computeEdgeEffectiveSpeed(Graph& g, const DistrictField& districts, const WorldConstraintField& world);
void computeNodeDemandRisk(Graph& g, const DistrictField& districts);
void assignIntersectionControls(Graph& g, const ControlPolicy& policy);
void decideGradeSeparation(Graph& g, const VerticalityPolicy& vpol);

```


## ) Control Device Ladder (deterministic policy)

Use a policy ladder similar to MUTCD/AASHTO logic, but implemented as thresholds.

### Control selection (C)

Given `D(n)`, `R(n)`, and the highest road class incident to `n`:

1. Very low D & low R → **Uncontrolled**
2. Low D & moderate R → **Yield / Two-way Stop**
3. Moderate D with balanced approaches → **All-way Stop**
4. High D or high R → **Signals**
5. High D & high R with space available → **Roundabout**
6. Extreme D or access-controlled roads → **Interchange / Grade Separation**

### Apply delays back into edges (feedback)

Once control is selected, compute a deterministic delay:

- Uncontrolled: small yield friction
- Stop: fixed stop penalty (+ extra for multi-way)
- Signal: phase-based penalty (doesn’t require sim; use a configurable fixed delay by node class)
- Roundabout: moderate penalty (usually < signal)
- Interchange: near-zero at-grade delay, but ramps add curvature/time

Then bake it into:

control_delay_mult(edge) = clamp( V_base / (V_base + Delay(edge_endpoints)), 0..1 )


This makes control devices *shape the effective flow field*.

---
### 6) Graph Algorithms (classification support)

- **NEW**
    
    - `generators/include/RogueCity/Generators/Urban/GraphAlgorithms.hpp`
        
    - `generators/src/Generators/Urban/GraphAlgorithms.cpp`
        

Purpose: shortest/simplest paths + sampled betweenness-like centrality to classify roads.

### 7) Flow + Control + Verticality Modules

- **NEW**
    
    - `FlowAndControl.hpp/.cpp`
        
    - `Verticality.hpp/.cpp`
        

These own:

- applying road type defaults
    
- computing `V_eff`
    
- computing node `D/R`
    
- selecting controls and feeding delays back
    
- making grade-sep + portal/ramp decisions
    

### 8) Required Modules Checklist (authoritative)

Full checklist from your plan:

- ✅ `PolylineRoadCandidate.hpp`
    
- ✅ `SegmentGridStorage.hpp/.cpp`
    
- ✅ `RoadNoder.hpp/.cpp`
    
- ✅ `Graph.hpp` upgrade
    
- ✅ `GraphSimplify.hpp/.cpp`
    
- ✅ `GraphAlgorithms.hpp/.cpp`
    
- ✅ `FlowAndControl.hpp/.cpp`
    
- ✅ `Verticality.hpp/.cpp`
    
- ✅ `IntersectionTemplates/` (later)
    

### 9) “Strong Opinion” Rules (guardrails)

- Picking + UI must query topology, not generator (index, don’t ask)
    
- Intersections are negotiated outcomes of hierarchy + speed + flow
    
- Layering is cheap if it’s integer IDs + portals; expensive if everything becomes 3D
    
- Templates beat improvisation for roundabouts/interchanges
    

---

 