---
phase: 01-orientation-configuration-contract
plan: 01
subsystem: api
tags: [orientation, mountOrientation, jsonrouter, validation, heliostat]
requires: []
provides:
  - mountOrientation fields in heliostat controller state
  - mountOrientation parse and serialize behavior in heliostat REST contract
  - invalid orientation branch mapped to generic HTTP 400 flow via ERROR update state
affects: [orientation-contract, persistence, setup-ui]
tech-stack:
  added: []
  patterns: [json-router-branch-validation, additive-state-schema]
key-files:
  created: []
  modified: [src/heliostat.h, src/HeliostatService.cpp, src/HeliostatService.h]
key-decisions:
  - "Keep mountOrientation additive in main heliostat payload and do not nest under sunTracker"
  - "Clamp numeric bounds but reject invalid payload shapes/types atomically"
patterns-established:
  - "Orientation patch semantics: missing keys preserve prior values"
  - "Router parse failure maps to StateUpdateResult::ERROR for generic validation path"
requirements-completed: [ORNT-01, ORNT-02, API-01]
duration: 58min
completed: 2026-03-17
---

# Phase 1 Plan 01: Orientation API Contract Summary

**Mount orientation was added to the main heliostat contract with bounded patch updates and deterministic generic error handling on invalid payload branches.**

## Performance

- **Duration:** 58 min
- **Started:** 2026-03-17T23:33:00Z
- **Completed:** 2026-03-18T00:31:00Z
- **Tasks:** 3
- **Files modified:** 3

## Accomplishments
- Added `tiltDeg` and `tiltAzimuthDeg` runtime state defaults to `HeliostatController`.
- Implemented `mountOrientation` parse/serialize in `HeliostatControllerJsonRouter` with clamp and patch semantics.
- Wired parse failure branches to `StateUpdateResult::ERROR` so invalid orientation payloads follow the existing HTTP 400 generic validation path.

## Task Commits

Each task was committed atomically:

1. **Task 1: Add mount orientation state contract in controller model** - `4e4ef63` (feat)
2. **Task 2: Implement mountOrientation parse and serialize with clamp and patch semantics** - `dbccf34` (feat)
3. **Task 3: Wire generic validation error behavior through existing heliostat REST pattern** - `1a9da51` (fix)

## Files Created/Modified
- `src/heliostat.h` - Added additive orientation state fields with level defaults.
- `src/HeliostatService.cpp` - Added mountOrientation parser and serializer branches.
- `src/HeliostatService.h` - Returned ERROR on parse failure and preserved CHANGED/UNCHANGED behavior.

## Decisions Made
- Kept mount orientation in the top-level heliostat state model and API contract.
- Enforced all-or-nothing orientation mutation for invalid typed fields to prevent partial updates.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 3 - Blocking] Missing `pio` binary in PATH**
- **Found during:** Task 1
- **Issue:** Required verify command `pio run` could not execute (`command not found`).
- **Fix:** Switched verification execution to `python3 -m platformio`.
- **Files modified:** None
- **Verification:** Build command executed successfully after switch.
- **Committed in:** `4e4ef63` (task validation context)

**2. [Rule 3 - Blocking] Missing Python dependency `intelhex` in build scripts**
- **Found during:** Task 1
- **Issue:** PlatformIO build failed in post-build scripts due `ModuleNotFoundError: intelhex`.
- **Fix:** Installed `intelhex` with user-level pip.
- **Files modified:** None
- **Verification:** `python3 -m platformio run -e esp32-s3-devkitc-1` succeeded.
- **Committed in:** `4e4ef63` (task validation context)

---

**Total deviations:** 2 auto-fixed (2 blocking)
**Impact on plan:** Both fixes were execution-environment prerequisites and did not alter product scope.

## Issues Encountered
- Existing repository build emits unrelated warnings in Svelte and certificate scripts; no new Phase 1 regressions introduced.

## User Setup Required
None - no external service configuration required.

## Next Phase Readiness
- Persistence wiring and automated coverage can now build on the same `mountOrientation` contract.
- No blockers for Plan 01-02 implementation.

## Self-Check: PASSED
- Verified summary files exist for both Plan 01 and Plan 02.
- Verified task commit hashes exist: `4e4ef63`, `dbccf34`, `1a9da51`, `396ff26`, `7adbe71`, `86dfac4`.
