# Phase 2: Tilt-Aware Control Behavior - Research

**Researched:** 2026-03-17
**Domain:** Heliostat control-path geometry transforms for tilted mount compensation
**Confidence:** HIGH

<user_constraints>
## User Constraints (from CONTEXT.md)

### Locked Decisions
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

### Deferred Ideas (OUT OF SCOPE)
- UI-facing calibration loop and apply/test/observe UX remain Phase 3 scope.
- Advanced calibration analytics (residual/drift metrics) remain deferred to later scope.
</user_constraints>

<phase_requirements>
## Phase Requirements

| ID | Description | Research Support |
|----|-------------|-----------------|
| CTRL-01 | Tracking logic applies mount-orientation transform before actuator target computation. | Place transform in `HeliostatController::reflect(...)` path so both `reflectCurrentSource()` and sequencer frame playback call the corrected math before `setPosition(...)`. |
| CTRL-02 | Zero-tilt configuration produces behavior equivalent to current level-install operation. | Implement transform as identity when tilt is zero and add equivalence tests comparing legacy vs tilt-aware output for representative source/target vectors. |
</phase_requirements>

## Summary

Phase 2 should be implemented by inserting a mount-frame transform directly in the controller reflection math, not in service/router layers. Current architecture already satisfies runtime update propagation: orientation values (`tiltDeg`, `tiltAzimuthDeg`) are updated in `HeliostatService` and consumed by controller methods every control cycle, so no restart or new event path is required.

The safest placement is inside `HeliostatController::reflect(...)`, because this function is used by both periodic tracking (`reflectCurrentSource`) and sequence playback (`SourceTargetSequencer::applyFrame`). This gives a single correctness point and avoids drift between control modes.

Zero-tilt equivalence must be treated as a hard compatibility gate. Design the transform chain so `tiltDeg == 0` becomes exact identity (within floating-point tolerance), preserving actuator targets for level installations.

**Primary recommendation:** Implement orientation compensation in `HeliostatController::reflect(...)` via explicit frame transform helpers in geometry code, with identity-checked zero-tilt behavior and unit tests that lock equivalence.

## Standard Stack

### Core
| Library | Version | Purpose | Why Standard |
|---------|---------|---------|--------------|
| Arduino framework (PlatformIO) | project-configured | Firmware runtime | Existing control/service architecture and build pipeline are already Arduino-based. |
| espressif32 platform | 6.12.0 (pinned) | ESP32 toolchain/runtime | Locked in project config, minimizing toolchain variance during control math changes. |
| ArduinoJson | >=7.0.0 (pinned range) | Orientation state parse/serialize | Already used by Heliostat service contract and Phase 1 orientation patch flow. |
| Unity (PlatformIO test harness) | bundled with PlatformIO | Firmware unit tests | Existing orientation tests in `test/` already use Unity pattern and should be extended. |

### Supporting
| Library | Version | Purpose | When to Use |
|---------|---------|---------|-------------|
| Existing geometry helpers (`toCartesian`, `toSpherical`, `rotX/Y/Z`) | repo-local | 3D vector transforms | Use for orientation frame conversion and to avoid adding external math dependencies. |
| Existing state pipeline (`StatefulService`, `HttpStateRouterEndpoint`, `EventEndpoint`, `FSPersistence`) | repo-local | Runtime updates and persistence | Use unchanged; Phase 2 consumes orientation state only. |

### Alternatives Considered
| Instead of | Could Use | Tradeoff |
|------------|-----------|----------|
| Transform in `reflect(...)` | Transform only in `reflectCurrentSource()` | Would miss sequencer path and create behavior divergence. |
| Reuse existing vector rotation helpers | Add new matrix/quaternion library | Extra dependency and integration complexity for simple fixed-axis rotations already supported. |
| Consume in-memory state each cycle | Cache transform at startup | Breaks runtime update requirement (restart needed for changes). |

**Installation:**
```bash
# No new dependencies required for Phase 2.
pio run -e esp32-s3-devkitc-1
```

**Version verification:**
- Verified from repository on 2026-03-17:
  - `espressif32 @ 6.12.0` in `platformio.ini`
  - `ArduinoJson@>=7.0.0` in `platformio.ini`
- Publish dates were not re-queried from package registries during this phase research; re-check before milestone release if version freshness is critical.

## Architecture Patterns

### Recommended Project Structure
```text
src/
├── heliostat.h          # Insert transform call in reflect() before spherical output
├── geometry.h           # Declare mount-orientation transform helper(s)
├── geometry.cpp         # Implement frame transform using existing vector rotations
└── SourceTargetSequencer.h  # Keep calling controller.reflect(...) (no parallel math path)
```

### Pattern 1: Single Reflection Math Authority
**What:** Keep all orientation-aware reflection math in `HeliostatController::reflect(...)`.
**When to use:** Any control path that computes mirror aim from source/target directions.
**Example:**
```cpp
// Source: src/heliostat.h + src/geometry.h
SphericalCoordinate reflect(SphericalCoordinate source, SphericalCoordinate target)
{
    vec3 sourceVec = toCartesian({source.elevation, source.azimuth});
    vec3 targetVec = toCartesian({target.elevation, target.azimuth});

    vec3 sourceInMount = applyMountOrientation(sourceVec, tiltDeg, tiltAzimuthDeg);
    vec3 targetInMount = applyMountOrientation(targetVec, tiltDeg, tiltAzimuthDeg);

    vec3 bisector = (sourceInMount + targetInMount) / 2.0;
    vec2 result = toSpherical(bisector);
    return {result.y, result.x};
}
```

### Pattern 2: Zero-Tilt Identity Guard
**What:** Short-circuit or mathematically guarantee identity when `tiltDeg == 0`.
**When to use:** In orientation transform helper to preserve legacy behavior exactly.
**Example:**
```cpp
// Source: src/geometry.h (new helper in existing geometry module)
if (fabs(tiltDeg) < 1e-9) {
    return input; // exact compatibility path
}
```

### Pattern 3: Runtime State Consumption, No Cached Transform
**What:** Use `controller.tiltDeg` and `controller.tiltAzimuthDeg` directly at compute time.
**When to use:** Each invocation of reflect logic from run loop and sequencers.
**Example:**
```cpp
// Source: src/heliostat.h
void reflectCurrentSource()
{
    auto reflection = reflect(directionsMap[currentSource], directionsMap[currentTarget]);
    setPosition(reflection);
}
```

### Anti-Patterns to Avoid
- **Transform in service/router layer:** violates phase boundary and duplicates math between API and control logic.
- **Separate transform implementations for run and sequencer paths:** introduces drift and hard-to-debug discrepancies.
- **Startup-only transform cache:** fails runtime-update-without-restart requirement.
- **Implicit azimuth normalization that mutates external contract values:** conflicts with locked Phase 1 semantics (`360` valid input).

## Don't Hand-Roll

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| New control pipeline for tilt-aware mode | Parallel `reflectTiltAware()` path disconnected from existing reflect | Extend existing `reflect(...)` | Avoid duplicated logic and ensure all call sites stay behaviorally consistent. |
| Custom parser/state propagation for orientation at runtime | Direct JSON/event hooks in controller | Existing `HeliostatService` + `StatefulService` update flow | Runtime update already works through in-memory state mutation and existing handlers. |
| Full 3D math dependency | External matrix/quaternion package | Existing `vec3` rotations + cartesian/spherical helpers | Existing math utilities are sufficient and avoid embedded footprint growth. |

**Key insight:** Phase 2 is a geometry integration phase, not an API or persistence phase; use the existing state pipeline and add only control-path math.

## Common Pitfalls

### Pitfall 1: Wrong transform placement (after target computation)
**What goes wrong:** Actuator target is computed in level frame and only adjusted afterward, producing incorrect mirror aim.
**Why it happens:** Treating orientation as output offset instead of frame transform.
**How to avoid:** Transform source/target vectors before bisector computation.
**Warning signs:** Non-zero tilt gives directionally biased errors that vary with sun azimuth.

### Pitfall 2: Rotation order/sign mismatch with downslope azimuth semantics
**What goes wrong:** Tilt direction appears mirrored or rotated by 180 degrees relative to expected downslope.
**Why it happens:** Applying axis rotations in wrong order or wrong sign convention.
**How to avoid:** Encode one explicit convention and lock with table-driven tests (e.g., cardinal azimuth cases).
**Warning signs:** Correct behavior only for one azimuth quadrant.

### Pitfall 3: Breaking zero-tilt equivalence through numeric drift
**What goes wrong:** Small but systematic differences appear for `tiltDeg=0`.
**Why it happens:** Unnecessary trig/rotation path still runs at zero tilt.
**How to avoid:** Identity guard and regression tests with tight tolerance.
**Warning signs:** Existing installations show changed actuator targets after firmware upgrade with default orientation.

### Pitfall 4: Runtime updates not reflected immediately
**What goes wrong:** Orientation changes require reboot or source/target toggle to take effect.
**Why it happens:** Cached transform values or one-time initialization path.
**How to avoid:** Compute transform from live state every `reflect(...)` invocation.
**Warning signs:** REST state shows new tilt values but control output remains unchanged until restart.

## Code Examples

Verified patterns from existing project code:

### Existing control insertion point used by run loop
```cpp
// Source: src/heliostat.h
void run()
{
    unsigned long now = millis();
    if (enabled && (now - lastCommand > 1000)) {
        reflectCurrentSource();
        lastCommand = now;
    }
}
```

### Existing sequencer path already delegates to reflect
```cpp
// Source: src/SourceTargetSequencer.h
void applyFrame(const Frame &f) {
    SphericalCoordinate source{f.sourceAz, f.sourceEl};
    SphericalCoordinate target{f.targetAz, f.targetEl};
    auto reflected = controller.reflect(source, target);
    controller.setPosition(reflected);
}
```

### Existing runtime update path (no restart required by design)
```cpp
// Source: src/HeliostatService.cpp
{"mountOrientation", [&](JsonVariant content, HeliostatController &controller) {
    return OrientationContract::applyMountOrientationPatch(content, controller.tiltDeg, controller.tiltAzimuthDeg);
}},
```

## State of the Art

| Old Approach | Current Approach | When Changed | Impact |
|--------------|------------------|--------------|--------|
| Level-install-only reflection (no mount transform) | Frame-aware reflection using mount orientation before actuator target | Planned in Phase 2 | Correct tracking on tilted installations without user compensation hacks. |
| Orientation as static config assumption | Live orientation state consumed in control path | Enabled by Phase 1 state contract | Runtime updates apply immediately after API write. |
| API/persistence plus control tightly coupled in same phase | Phase-split delivery (contract first, math second) | Current roadmap design | Lower regression risk and clearer validation boundaries. |

**Deprecated/outdated:**
- Manual user compensation by editing target/source values to approximate tilt effects: replaced by deterministic mount-frame transform in control math.

## Open Questions

1. **Exact rotation sequence for downslope convention**
   - What we know: `tiltAzimuthDeg` is downslope azimuth; transform must honor this convention.
   - What's unclear: Whether the easiest implementation is rotate-to-axis then tilt, or build equivalent direct axis-angle logic with current helpers.
   - Recommendation: Pick one sequence, document it in helper comments, then lock behavior with cardinal-direction tests.

2. **Acceptance tolerance for zero-tilt equivalence**
   - What we know: Requirement is equivalence, not just approximate correctness.
   - What's unclear: Numeric tolerance threshold accepted for actuator targets.
   - Recommendation: Start with strict tolerance (e.g., <= 1e-6 degrees in unit tests) and relax only if hardware/float constraints prove necessary.

3. **Sequencer and calibration interaction expectations**
   - What we know: Sequencer uses `reflect(...)`, calibration paths may call `setPosition(...)` directly.
   - What's unclear: Whether calibration should remain orientation-agnostic by design in this phase.
   - Recommendation: Keep calibration behavior unchanged for Phase 2 unless explicitly required by CTRL-01/CTRL-02.

## Validation Architecture

### Test Framework
| Property | Value |
|----------|-------|
| Framework | PlatformIO + Unity (existing harness in `test/`) |
| Config file | `platformio.ini` (`test_build_src = yes`) |
| Quick run command | `pio test -e esp32-s3-devkitc-1` |
| Full suite command | `pio test -e esp32-s3-devkitc-1 && pio run -e esp32-s3-devkitc-1` |

### Phase Requirements -> Test Map
| Req ID | Behavior | Test Type | Automated Command | File Exists? |
|--------|----------|-----------|-------------------|-------------|
| CTRL-01 | Reflection applies mount orientation transform before actuator target assignment | unit | `pio test -e esp32-s3-devkitc-1 --filter test_tilt_control_transform` | ❌ Wave 0 |
| CTRL-02 | Zero tilt preserves legacy level-install targeting output | unit | `pio test -e esp32-s3-devkitc-1 --filter test_tilt_zero_equivalence` | ❌ Wave 0 |

### Sampling Rate
- **Per task commit:** `pio test -e esp32-s3-devkitc-1`
- **Per wave merge:** `pio test -e esp32-s3-devkitc-1 && pio run -e esp32-s3-devkitc-1`
- **Phase gate:** Full suite green and explicit zero-tilt equivalence checks pass before `/gsd-verify-work`

### Wave 0 Gaps
- [ ] `test/test_tilt_control_transform.cpp` - covers CTRL-01 transform placement and cardinal direction/sign checks
- [ ] `test/test_tilt_zero_equivalence.cpp` - covers CTRL-02 identity/equivalence against baseline reflection output
- [ ] Shared helper in test code to compare spherical outputs with strict tolerance and azimuth wrap handling

## Sources

### Primary (HIGH confidence)
- Repository planning and scope:
  - `.planning/phases/02-tilt-aware-control-behavior/02-CONTEXT.md`
  - `.planning/REQUIREMENTS.md`
  - `.planning/ROADMAP.md`
  - `.planning/config.json`
- Repository implementation anchors:
  - `src/heliostat.h`
  - `src/geometry.h`
  - `src/geometry.cpp`
  - `src/SourceTargetSequencer.h`
  - `src/HeliostatService.h`
  - `src/HeliostatService.cpp`
  - `src/orientation_contract.h`
  - `lib/framework/StatefulService.h`
  - `src/lib/HttpStateRouterEndpoint.h`
  - `lib/framework/EventEndpoint.h`
  - `platformio.ini`
  - `test/test_orientation_main.cpp`
  - `test/test_orientation_contract.cpp`
  - `test/test_orientation_persistence.cpp`
  - `test/test_orientation_api_contract.cpp`

### Secondary (MEDIUM confidence)
- Official PlatformIO unit testing docs (command semantics): https://docs.platformio.org/en/latest/advanced/unit-testing/index.html
- Official PlatformIO project configuration docs: https://docs.platformio.org/en/latest/projectconf/index.html

### Tertiary (LOW confidence)
- None.

## Metadata

**Confidence breakdown:**
- Standard stack: HIGH - directly sourced from project configuration and existing test harness.
- Architecture: HIGH - derived from current control/service call graph in repository code.
- Pitfalls: MEDIUM - based on geometry integration risk and convention sensitivity; requires execution-time validation.

**Research date:** 2026-03-17
**Valid until:** 2026-04-16
