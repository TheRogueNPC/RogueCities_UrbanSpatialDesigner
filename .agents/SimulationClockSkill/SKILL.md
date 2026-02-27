---
name: SimulationClockSkill
description: |
  Provides utilities to replace direct `std::chrono` usage with a centralized `App::SimulationClock`. Scans the codebase for `std::chrono` calls and suggests or applies replacements.
---

# Overview
The contract requires time‑translation symmetry. This skill automates detection and replacement of wall‑clock time calls.

# Steps
1. Search the `app` directory for `std::chrono::` patterns.
2. For each match, generate a diff that replaces the call with `SimulationClock::Now()`.
3. Optionally apply the diff if the user approves.
4. Update a summary section in the audit report.

# Usage
```
run_skill SimulationClockSkill
```

# Dependencies
- `grep_search`
- `multi_replace_file_content` for applying fixes.
