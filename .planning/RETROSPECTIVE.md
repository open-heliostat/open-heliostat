# Project Retrospective

*A living document updated after each milestone. Lessons feed forward into future planning.*

## Milestone: v1.0 - MVP

**Shipped:** 2026-03-19
**Phases:** 3 | **Plans:** 6 | **Sessions:** 1

### What Was Built
- Added a complete mount-orientation firmware contract with persisted `tiltDeg` and `tiltAzimuthDeg` fields.
- Integrated tilt-aware transform behavior into reflector control flow with zero-tilt compatibility guarantees.
- Delivered a dedicated heliostat setup UI with apply/readback/observe workflow and refresh persistence confirmation.

### What Worked
- Phase-by-phase context capture reduced rework and kept implementation tightly aligned with requirements.
- TDD-style progression (RED -> GREEN) for geometry and UI behavior made regressions visible early.

### What Was Inefficient
- Milestone audit was skipped before completion, leaving explicit gaps assessment to post-hoc review.
- Runtime verification had environment friction in some paths, causing reliance on build-only checks at times.

### Patterns Established
- Keep orientation semantics explicit and consistent from API contract through control math and UI.
- Use route-level Vitest/jsdom tests for heliostat UI flows with deterministic REST mocks.

### Key Lessons
1. Locking coordinate/frame semantics in context before coding prevents expensive control-path churn later.
2. Setup UX should verify real operator outcomes (apply/readback/refresh), not just form submission.

### Cost Observations
- Model mix: planner-heavy Opus with Sonnet researcher/checker/executor support
- Sessions: 1 milestone cycle
- Notable: Front-loaded context and phase summaries reduced downstream ambiguity.

---

## Cross-Milestone Trends

### Process Evolution

| Milestone | Sessions | Phases | Key Change |
|-----------|----------|--------|------------|
| v1.0 | 1 | 3 | Introduced end-to-end GSD phase pipeline with context/research/plan/execute artifacts |

### Cumulative Quality

| Milestone | Tests | Coverage | Zero-Dep Additions |
|-----------|-------|----------|-------------------|
| v1.0 | Firmware + route-level UI checks | Targeted requirement coverage | 0 |

### Top Lessons (Verified Across Milestones)

1. Explicit context contracts materially improve planning quality and execution speed.
2. Closing the loop with user-observable verification criteria produces more reliable feature outcomes.
