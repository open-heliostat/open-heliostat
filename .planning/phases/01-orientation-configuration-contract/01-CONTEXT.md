# Phase 1: Orientation Configuration Contract - Context

**Gathered:** 2026-03-17
**Status:** Ready for planning

<domain>
## Phase Boundary

Define, validate, persist, and expose mount orientation settings through firmware APIs so users can configure tilt-aware installs safely, while preserving level-install backward compatibility.

</domain>

<decisions>
## Implementation Decisions

### Orientation schema shape
- Add a dedicated `mountOrientation` object in heliostat state (do not nest in `sunTracker`).
- Use explicit degree-based fields: `tiltDeg` and `tiltAzimuthDeg`.
- Do not add a separate `enabled` flag in v1; behavior is driven directly by numeric values.
- Define `tiltAzimuthDeg` as the compass azimuth of downslope direction.

### Validation behavior
- Validation handling for out-of-range values: clamp to nearest valid value.
- Accepted ranges: `tiltDeg` in `[-90, 90]`, `tiltAzimuthDeg` in `[0, 360]`.
- Public API and UI use degrees only.
- Error payload style: single generic error string when validation fails.

### Persistence and migration policy
- If orientation fields are missing in legacy config, auto-fill level-install defaults at runtime.
- Persist orientation immediately after a valid change.
- For combined updates: if one orientation field is invalid, reject the whole orientation update.
- Legacy clients that do not send orientation fields continue working unchanged with level behavior.

### API update granularity
- v1 write path: single object write via main heliostat payload (`/rest/heliostat`).
- v1 read path: include orientation data in main `/rest/heliostat` response.
- Support patch semantics for `mountOrientation` (missing fields keep prior values).
- Reuse existing `heliostat-service` event stream for update propagation.

### Claude's Discretion
- Exact naming of internal helper functions for parse/serialize defaults and clamping utilities.
- Exact wording/text of generic validation error messages.

</decisions>

<canonical_refs>
## Canonical References

**Downstream agents MUST read these before planning or implementing.**

### Scope and acceptance
- `.planning/ROADMAP.md` - Phase 1 goal, requirements mapping, and success criteria boundary.
- `.planning/REQUIREMENTS.md` - ORNT-01, ORNT-02, CONF-01, API-01 definitions and traceability.
- `.planning/PROJECT.md` - project-level constraints and non-negotiables (backward compatibility, explicit orientation model).

### Existing firmware contracts
- `src/HeliostatService.h` - heliostat REST/event/fs persistence plumbing and save-map pattern.
- `src/HeliostatService.cpp` - current JSON routing, parse/serialize behavior, and `/rest/heliostat` contract surface.
- `src/heliostat.h` - controller state shape and core heliostat model integration point.

### Prior analysis
- `.planning/codebase/ARCHITECTURE.md` - service boundaries, control/data flow, and integration layering.
- `.planning/codebase/CONVENTIONS.md` - established code and state-management patterns.
- `.planning/research/SUMMARY.md` - recommended orientation-contract direction and migration risks.

</canonical_refs>

<code_context>
## Existing Code Insights

### Reusable Assets
- `HeliostatControllerJsonRouter` in `src/HeliostatService.cpp`: direct location to add `mountOrientation` parse/serialize handling.
- Save-map helpers in `src/HeliostatService.h`: existing mechanism for selecting persisted fields.
- `FSPersistence` usage at `/config/heliostat.json` in `src/HeliostatService.h`: existing persistence path to reuse without new storage systems.

### Established Patterns
- Stateful service contract uses single `/rest/heliostat` payload for read/write with router-based patch updates.
- Existing services favor additive schema evolution and keeping legacy behavior when fields are absent.
- Event propagation uses existing `heliostat-service` endpoint rather than feature-specific streams.

### Integration Points
- Add `mountOrientation` fields to `HeliostatController` in `src/heliostat.h`.
- Extend JSON routing + validation + save-map in `src/HeliostatService.cpp` and `src/HeliostatService.h`.
- Ensure defaults are applied during FS load/read path and remain compatible with old config files.

</code_context>

<specifics>
## Specific Ideas

- Orientation setup is explicitly separate from existing controller offset semantics.
- A single tilt parameter was rejected; v1 must carry both amount and direction.
- Direction convention is anchored to real-world compass semantics for installers.

</specifics>

<deferred>
## Deferred Ideas

- Guided multi-step setup wizard (tracked in v2 requirements).
- Live residual pointing error metric and profile management (tracked in v2 requirements).

</deferred>

---

*Phase: 01-orientation-configuration-contract*
*Context gathered: 2026-03-17*
