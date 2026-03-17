# Phase 3: Setup UI and Verification Loop - Context

**Gathered:** 2026-03-18
**Status:** Ready for planning

<domain>
## Phase Boundary

Deliver a dedicated heliostat setup workflow that lets users configure mount orientation, apply/test/observe its effect, and verify settings remain correct after refresh/reload.

</domain>

<decisions>
## Implementation Decisions

### Setup location and information architecture
- Add orientation setup directly in the existing heliostat page flow, as a dedicated section within `interface/src/routes/heliostat/Heliostat.svelte`.
- Keep orientation setup separate from source/target editing and separate from existing sun tracker location/time controls.
- Reuse existing settings-card structure and grid layout patterns already used in the page.

### Input controls and validation UX
- Show both orientation inputs: `tiltDeg` and `tiltAzimuthDeg` with explicit degree semantics and bounds.
- Reuse existing form components (`Slider`, `GridForm`, `Button`) for consistency with current UI behavior.
- Validation messaging should be inline and immediate in the setup section (do not introduce a new global error system in this phase).

### Apply/test/observe interaction loop
- Provide explicit apply action for orientation changes instead of silent background mutation.
- After apply, immediately re-read heliostat state and show updated observable tracking values in the same page context.
- Keep verification interaction lightweight and operator-focused: configure -> apply -> observe current tracker/target behavior.

### Refresh/reload persistence confirmation
- Orientation values must be loaded from `/rest/heliostat` on page refresh/reload and shown in the setup section.
- The user verification loop includes refresh confirmation as part of setup completion criteria.
- No additional persistence mechanism is added in this phase; use existing REST + fs-backed service behavior from prior phases.

### Claude's Discretion
- Exact visual wording for helper text and section labels.
- Whether to use one shared save/apply button or per-field immediate apply inside the orientation section.

</decisions>

<canonical_refs>
## Canonical References

**Downstream agents MUST read these before planning or implementing.**

### Scope and requirement contract
- `.planning/ROADMAP.md` - Phase 3 goal and success criteria for setup/apply/test/refresh behavior.
- `.planning/REQUIREMENTS.md` - UI-01 and UI-02 requirements and traceability.
- `.planning/PROJECT.md` - core product value and compatibility constraints to preserve.

### Prior phase decisions that constrain UI
- `.planning/phases/01-orientation-configuration-contract/01-CONTEXT.md` - orientation field names, bounds, and semantics that UI must represent.
- `.planning/phases/02-tilt-aware-control-behavior/02-CONTEXT.md` - control behavior expectations and live-update semantics UI must verify.
- `.planning/phases/02-tilt-aware-control-behavior/02-01-SUMMARY.md` - transform contract behavior and test coverage anchors relevant to user-facing verification.
- `.planning/phases/02-tilt-aware-control-behavior/02-02-SUMMARY.md` - reflect-path integration and runtime update behavior confirmation.

### UI and API implementation anchors
- `interface/src/routes/heliostat/Heliostat.svelte` - primary setup page where orientation UI and verification loop are implemented.
- `interface/src/lib/stores/rest.ts` - REST helper behavior for request/response error handling.
- `src/HeliostatService.cpp` - current `/rest/heliostat` data shape including `mountOrientation` and live tracker fields.
- `src/orientation_contract.h` - authoritative orientation bounds and field names used by API.

### Existing architecture and conventions
- `.planning/codebase/ARCHITECTURE.md` - UI to REST/event data flow and service boundaries.
- `.planning/codebase/CONVENTIONS.md` - established frontend component and state-management patterns.

</canonical_refs>

<code_context>
## Existing Code Insights

### Reusable Assets
- `interface/src/routes/heliostat/Heliostat.svelte` already has card-based sections, form grids, and apply actions that can be extended.
- Shared UI components (`Slider`, `GridForm`, `Button`, `SettingsCard`) are already integrated in this route and should be reused.
- Existing REST helpers (`getJsonRest`, `postJsonRest`) already support the read/write loop needed for apply/test/observe.

### Established Patterns
- Heliostat page currently hydrates state from `/rest/heliostat` and updates specific branches with targeted REST posts.
- Current page logic uses immediate state refresh after side-effect actions (e.g., GPS/time operations), matching verification loop needs.
- UI page favors in-context status display (alerts/info sections) over global notification frameworks.

### Integration Points
- Extend `HeliostatControllerState` typing in `interface/src/routes/heliostat/Heliostat.svelte` to include `mountOrientation`.
- Add orientation setup controls and apply workflow in the same route, using existing rest path `/rest/heliostat`.
- Reuse existing sun tracker info panel as observation context after orientation apply, with minimal incremental UI additions.

</code_context>

<specifics>
## Specific Ideas

Auto-selected decisions log (`--auto`):
- [auto] Setup placement -> dedicated mount orientation section within existing heliostat page.
- [auto] Inputs -> explicit tilt and direction controls with inline validation and degree semantics.
- [auto] Interaction loop -> explicit apply, immediate read-back, observe tracking response in-page.
- [auto] Reload verification -> include refresh/reload confirmation in setup completion flow.

</specifics>

<deferred>
## Deferred Ideas

- Multi-step guided wizard remains deferred (v2 scope).
- Advanced residual error metrics and profile management remain deferred (v2 scope).

</deferred>

---

*Phase: 03-setup-ui-and-verification-loop*
*Context gathered: 2026-03-18*
