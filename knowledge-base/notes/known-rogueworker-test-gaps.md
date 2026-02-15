---
tags: [roguecity, rogueworker, threading, known-issues, tests]
type: reference
created: 2026-02-15
---

# Known RogueWorker Test Gaps (Multithreading Reliability)

Current testing docs record intermittent RogueWorker failures and linkage concerns, indicating multithreaded worker execution paths are not yet as stable as the single-threaded generation path.

## Reported Gaps
- Some RogueWorker tests fail to complete expected jobs
- Historical linker concerns around header-only implementation choices
- Multithreading issues documented as medium-priority follow-up

## Operational Impact
- Core and single-thread workflows remain usable
- Parallel execution reliability needs targeted hardening

## Source Files
- `docs/Tests/TestingQuickStart.md`
- `docs/Tests/TestingSummary.md`

## Related
- [[topics/testing-and-quality-assurance]]
- [[notes/testing-suite-coverage-and-execution]]
- [[notes/core-library-data-and-editor-types]]
