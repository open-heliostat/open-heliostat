# Phase 2: Tilt-Aware Control Behavior - Context

**Gathered:** 2026-03-17
**Status:** Ready for planning

<domain>
## Phase Boundary

Apply mount-orientation transforms in tracking/control math so outputs are correct on tilted mounts, while preserving equivalent behavior for level installations and immediate runtime effect after config updates.

</domain>

<decisions>
## Implementation Decisions

### Transform placement in control flow
- Apply orientation correction inside heliostat control behavior path, before actuator target assignment.
- Keep service/API/persistence contract unchanged in this phase; consume existing `mountOrientation` values from controller state.
- Preserve current control cadence (`run()` update and control loop structure) and avoid introducing new tasks/services.

### Frame convention and direction semantics
- Carry forward Phase 1 direction convention: `tiltAzimuthDeg` represents compass azimuth of downslope.
- Use degree-based external semantics consistently (`tiltDeg`, `tiltAzimuthDeg`) and keep any radian conversions internal to math helpers only.
- Treat `tiltAzimuthDeg=360` as valid contract input; normalization behavior can remain internal and must not change external contract semantics.

### Zero-tilt compatibility behavior
- `tiltDeg == 0` must produce behavior equivalent to current level-install output path.
- Compatibility is defined by unchanged actuator targeting outcomes for identical inputs when tilt is zero.
- No user-facing migration steps are introduced for Phase 2.

### Runtime update behavior
- Updated orientation settings must affect subsequent tracking targets without firmware restart.
- Runtime behavior should use current in-memory controller state each cycle, not cached startup-only transform values.
- Maintain existing event/update flow from Phase 1; no additional event channels required.

### Claude's Discretion
- Exact math decomposition details (rotation order implementation, helper function names, and where to place reusable transform utilities).
- Specific tolerance thresholds used in plan-level verification criteria for zero-tilt equivalence checks.

</decisions>

<canonical_refs>
## Canonical References

**Downstream agents MUST read these before planning or implementing.**

### Scope and success criteria
- `.planning/ROADMAP.md` - Phase 2 goal, dependencies, and required success criteria.
- `.planning/REQUIREMENTS.md` - CTRL-01 and CTRL-02 requirement definitions and traceability status.
- `.planning/PROJECT.md` - project constraints (backward compatibility and explicit orientation model expectations).

### Prior phase decisions and outcomes
- `.planning/phases/01-orientation-configuration-contract/01-CONTEXT.md` - locked orientation schema, range, and semantics carried into Phase 2.
- `.planning/phases/01-orientation-configuration-contract/01-01-SUMMARY.md` - established API/state contract behavior that control logic must consume unchanged.
- `.planning/phases/01-orientation-configuration-contract/01-02-SUMMARY.md` - persistence/helper patterns and known validation/runtime constraints from completed work.

### Control-path implementation anchors
- `src/heliostat.h` - heliostat controller run/reflect/setPosition path and current orientation state fields.
- `src/geometry.h` - vector, spherical/cartesian conversions, and rotation helpers available for transform implementation.
- `src/geometry.cpp` - current coordinate conversion behavior used by reflection logic.
- `src/main.cpp` - control loop timing/cadence boundaries and runtime constraints.
- `src/orientation_contract.h` - orientation contract constants and field semantics for consistency.

### Architectural guidance
- `.planning/codebase/ARCHITECTURE.md` - control-loop data flow and layering expectations.
- `.planning/codebase/CONCERNS.md` - technical risk areas relevant to math/order/sign regressions.

</canonical_refs>

<code_context>
## Existing Code Insights

### Reusable Assets
- `HeliostatController::reflect` and `reflectCurrentSource` in `src/heliostat.h` provide the direct insertion point for tilt-aware behavior.
- Rotation helpers (`rotX`, `rotY`, `rotZ`) and conversion utilities in `src/geometry.h` support frame-transform implementation without adding heavy dependencies.
- Orientation state fields (`tiltDeg`, `tiltAzimuthDeg`) are already available in `HeliostatController` from Phase 1.

### Established Patterns
- Control behavior currently computes reflection from source/target directions then directly calls `setPosition`; Phase 2 should extend this path rather than create parallel logic.
- Existing architecture keeps API/state transport in service/router layers and domain math in controller/model layers.
- External contracts are degree-based; internal conversions occur at geometry boundaries.

### Integration Points
- Primary integration point: `src/heliostat.h` (`reflect`, `reflectCurrentSource`, or adjacent helper path) for transform application before `setPosition`.
- Supporting integration point: `src/geometry.h` and `src/geometry.cpp` for reusable transform math primitives.
- Verification anchor: maintain control loop behavior in `src/main.cpp` without cadence regressions.

</code_context>

<specifics>
## Specific Ideas

Auto-selected decisions log (`--auto`):
- [auto] Transform placement -> apply orientation in control behavior path before actuator targeting.
- [auto] Direction semantics -> preserve downslope azimuth convention from Phase 1.
- [auto] Compatibility model -> enforce strict zero-tilt equivalence to legacy behavior.
- [auto] Runtime behavior -> orientation changes apply on subsequent cycles without restart.

</specifics>

<deferred>
## Deferred Ideas

- UI-facing calibration loop and apply/test/observe UX remain Phase 3 scope.
- Advanced calibration analytics (residual/drift metrics) remain deferred to later scope.

</deferred>

---

*Phase: 02-tilt-aware-control-behavior*
*Context gathered: 2026-03-17*
