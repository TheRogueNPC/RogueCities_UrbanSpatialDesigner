# Policy Auditors

## Purpose
Policy auditors run after generation as analysis/enforcement passes, not as full agent simulation.

## Auditors
- `ConnectivityAuditor`
  - Detects disconnected road components.
  - Flags districts that appear unreachable from the road graph.
- `ZoningEnforcer`
  - Evaluates FBCZ rule compatibility (height and frontage signals).
  - Emits deterministic adjustment proposals/flags.

## BehaviorTree Integration
- When `ROGUECITY_ENABLE_BEHAVIORTREE_AUDITORS` and BehaviorTree.CPP are available, auditors execute via a minimal BT sequence.
- Without the dependency, fallback deterministic sequential execution is used.

## Outputs
- JSON report (`AuditReport::ToJson`)
- Human-readable report (`AuditReport::ToHumanReadable`)
- Optional plan-violation promotion in `CityGenerator` when policy auditors are enabled.
