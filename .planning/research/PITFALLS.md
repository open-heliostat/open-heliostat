# Domain Pitfalls: Tilt-Aware Heliostat Tracking on ESP32

**Domain:** Embedded heliostat tracking with mount orientation transforms
**Researched:** 2026-03-17
**Scope:** Adding mount tilt magnitude + tilt direction to an existing level-assumed control stack

## Suggested Phase Map (for downstream planning)

| Phase | Topic | Goal |
|------|-------|------|
| P1 | Coordinate model contract | Define frame conventions, angle units, and transform order |
| P2 | Tracking math integration | Apply mount transform in source/target/reflection pipeline |
| P3 | Persistence + API | Add config fields with backward-compatible defaults |
| P4 | UI + validation | Expose settings safely and prevent invalid inputs |
| P5 | Verification + field calibration | Bench and outdoor validation across orientations |

## Critical Pitfalls

### Pitfall 1: Frame Convention Drift (North/East/Up vs device axes)
**What goes wrong:**
Mount tilt direction is interpreted in one frame while solar azimuth/elevation is computed in another, producing a consistent but wrong pointing offset that changes by time of day.

**Why it happens:**
- Existing code uses spherical/cartesian conversions in `geometry.*` and reflection in `heliostat.h`, but frame semantics are implicit.
- Solar calculators in the ecosystem use different longitude/time conventions (legacy NOAA page explicitly documents non-standard west-positive inputs), which can leak into implementation assumptions.

**Warning signs:**
- At solar noon, pointing error appears mostly in one axis and flips sign morning vs afternoon.
- A known level mount (tilt=0) no longer behaves exactly like pre-feature firmware.
- Team discussions include phrases like "it depends which azimuth zero we mean" without a written contract.

**Prevention:**
- Write a single frame contract before code: world frame, device frame, azimuth zero reference, positive rotation directions, elevation bounds, degree/radian policy.
- Add invariant tests: tilt=0 and direction arbitrary must equal old math within tolerance.
- Store transform metadata with examples in docs and comments near conversion helpers.

**Detection:**
- Golden-vector tests for 8 azimuth points and 3 elevations before and after transform.
- Replay captured solar positions and compare expected pointing deltas for synthetic known tilts.

**Phase mapping:** P1 (primary), P2, P5

---

### Pitfall 2: Wrong Rotation Composition (order/sign bug)
**What goes wrong:**
Applying tilt-direction as sequential rotations in the wrong order or with wrong sign creates non-physical behavior (especially near horizon/zenith), despite code compiling and appearing plausible.

**Why it happens:**
- 3D rotations are non-commutative.
- Existing helpers (`rotX/rotY/rotZ`) are mutable and easy to compose incorrectly.
- Developers mix "rotate world into mount" vs "rotate mount into world" semantics.

**Warning signs:**
- Error is small at one part of sky and large elsewhere.
- Opposite tilt direction does not mirror behavior as expected.
- Regression appears only for non-zero tilt direction, not pure magnitude.

**Prevention:**
- Use explicit transform pipeline naming: `world_to_mount`, `mount_to_world`.
- Keep a single canonical implementation path; avoid duplicate formula variants in controller vs service layers.
- Validate with identity and inverse checks ($R^{-1}(R(v)) \approx v$).

**Detection:**
- Unit tests for rotation composition and inverse.
- Property tests on random vectors to ensure norm preservation and inverse consistency.

**Phase mapping:** P1 (primary), P2, P5

---

### Pitfall 3: Silent Degree/Radian Mix in New Settings
**What goes wrong:**
UI/API accepts degrees while internal math expects radians (or vice versa), leading to severe pointing error that looks like bad calibration.

**Why it happens:**
- Existing code mixes degree-facing APIs with radian internals (`degToRad`, `radToDeg`, `ObjectDirection`).
- JSON payloads are typeless at boundaries.

**Warning signs:**
- Small input changes produce massive actuator jumps.
- 45-degree input behaves like ~0.78 or ~2578 depending on path.
- No explicit unit in API fields.

**Prevention:**
- Keep public config/API strictly in degrees; convert once at math boundary.
- Name fields with unit suffix where practical (`tiltDeg`, `tiltDirectionDeg`).
- Add schema-level range checks at API boundary.

**Detection:**
- Endpoint contract tests with representative values (0, 5, 45, 89).
- UI e2e check asserting roundtrip value persistence unchanged.

**Phase mapping:** P1, P3 (primary), P4, P5

---

### Pitfall 4: Backward Compatibility Break at Default Configuration
**What goes wrong:**
Existing installations shift behavior after firmware update because default orientation is not treated as exact level identity in all code paths.

**Why it happens:**
- New config fields are added without deterministic migration.
- Save/load maps are manually curated in service routers and easy to partially update.

**Warning signs:**
- Users with old config files report changed tracking immediately after update.
- First boot after upgrade shows NaN or extreme defaults for new fields.
- Behavior differs before/after reboot due to persistence mismatch.

**Prevention:**
- Migration rule: missing fields resolve to `tilt=0`, `direction=0` and produce byte-for-byte equivalent targets vs old code path.
- Update save map and serializer together, then test cold boot + warm boot + upgrade path.
- Version config schema for heliostat settings.

**Detection:**
- Fixture tests for old config payloads.
- Hardware smoke test on a known level rig before release.

**Phase mapping:** P3 (primary), P5

---

### Pitfall 5: Double Compensation with Existing Offsets/Controller Tuning
**What goes wrong:**
Mount orientation correction is added on top of existing manual offsets or axis inversion settings, effectively compensating twice.

**Why it happens:**
- Brownfield system already has controller offsets and stepper inversion semantics.
- Feature scope excludes redesigning offset semantics, so integration boundaries are subtle.

**Warning signs:**
- Previously tuned installations require major retuning after enabling orientation.
- Compensated errors become larger when existing offsets are non-zero.
- Users "fix" behavior only by setting unlikely orientation values.

**Prevention:**
- Define exact order: mechanical calibration offsets vs orientation transform vs closed-loop control.
- Add migration guidance: when to zero legacy offsets before enabling orientation.
- Block simultaneous conflicting modes if necessary.

**Detection:**
- Matrix test across combinations of offset/inversion/orientation.
- Compare error surfaces with and without legacy offsets.

**Phase mapping:** P2 (primary), P4, P5

---

### Pitfall 6: Singularities and Numeric Instability (near horizon, normalization, acos domain)
**What goes wrong:**
Tracking math emits NaN or unstable azimuth near degenerate geometries, causing actuator spikes or oscillations.

**Why it happens:**
- Reflection and angle math use normalization and inverse trig (`acos`) where floating-point drift can exceed [-1, 1].
- Near-zero vector magnitudes and below-horizon cases are not always guarded.

**Warning signs:**
- Intermittent jumps to extreme azimuth/elevation.
- Logs show occasional NaN/inf.
- Instability clusters around sunrise/sunset or low sun angles.

**Prevention:**
- Clamp dot products before `acos`.
- Guard zero-length vectors and define behavior when sun elevation is below threshold.
- Add physically meaningful min/max elevation and slew limits.

**Detection:**
- Fuzz tests around boundary angles.
- Runtime assertions/telemetry counters for invalid math events.

**Phase mapping:** P2 (primary), P5

---

### Pitfall 7: Time/Timezone Misinterpretation Blamed as Orientation Error
**What goes wrong:**
Wrong UTC/local/DST handling shifts solar position, then orientation feature is incorrectly blamed.

**Why it happens:**
- Solar position depends critically on timestamp and coordinates.
- The stack has both system time and TimeLib paths; manual time inputs can be ambiguous.
- External calculators vary in sign conventions for longitude/time zone.

**Warning signs:**
- Constant angular error roughly tracking solar hour angle.
- Good pointing only when manual local-time offsets are hacked.
- Different results between two calculators without clear convention mapping.

**Prevention:**
- Standardize internal solar calculation inputs on UTC epoch + standard lat/long convention.
- Log UTC ISO, local ISO, timezone offset, and computed sun az/el in one debug packet.
- Define and document accepted conventions in API.

**Detection:**
- Cross-check against one trusted reference source for fixed timestamps.
- Regression tests across DST transitions and timezone changes.

**Phase mapping:** P2, P3, P5 (primary)

---

### Pitfall 8: Control-Loop Starvation from Heavy Math/Logging in Real-Time Path
**What goes wrong:**
Added transform computations and debug logs in hot loops increase loop latency, causing jitter, missed motor updates, or watchdog events.

**Why it happens:**
- ESP32 has tight timing budgets and watchdog constraints.
- Frequent trig and JSON/log formatting in loop/task contexts can monopolize CPU if not bounded.

**Warning signs:**
- New intermittent control stutter when orientation is enabled.
- Watchdog warnings/panics under sustained operation.
- Tracking lag grows when telemetry/logging is turned on.

**Prevention:**
- Precompute reusable trig terms when settings change, not every cycle.
- Keep hot-loop logging off by default; sample telemetry at lower rates.
- Benchmark loop execution time before/after feature.

**Detection:**
- Add loop timing histograms and max-latency counters.
- Stress run with watchdog enabled and orientation active for multi-hour windows.

**Phase mapping:** P2 (primary), P5

## Moderate Pitfalls

### Pitfall 9: Ambiguous UI Semantics for Tilt Direction
**What goes wrong:**
Users enter downhill direction while firmware expects uphill normal azimuth (or vice versa), leading to exactly opposite correction.

**Warning signs:**
- Rotating direction by 180 degrees "fixes" behavior.
- Support tickets mention confusion about compass reference and magnetics.

**Prevention:**
- UI copy must explicitly define direction semantic with diagram.
- Add helper calibration wizard using two observed sun points if possible.

**Phase mapping:** P4 (primary), P5

### Pitfall 10: Unbounded User Input and Unsafe Mechanical Commands
**What goes wrong:**
Invalid orientation values (e.g., >90 tilt) propagate into impossible targets and can drive motors into hard stops.

**Warning signs:**
- Motor stalls at feature enable.
- Spurious emergency stop events after saving config.

**Prevention:**
- Enforce strict ranges and sanitize NaN/inf at API boundary.
- Apply final command clamping against mechanical limits after all transforms.

**Phase mapping:** P3, P4 (primary), P5

## Minor Pitfalls

### Pitfall 11: Incomplete Test Matrix (Only testing noon or one orientation)
**What goes wrong:**
Feature appears correct in a narrow scenario but fails in morning/evening or opposite hemisphere assumptions.

**Prevention:**
- Test matrix should include: tilt {0, 5, 15, 30, 45}, direction {0, 90, 180, 270}, times {morning, noon, afternoon}, and at least two latitudes.

**Phase mapping:** P5 (primary)

### Pitfall 12: Overfitting to One Field Installation
**What goes wrong:**
Compensations tuned to one rig (backlash/flex) get encoded as universal math.

**Prevention:**
- Keep mount-orientation model purely geometric; put hardware-specific compensation in existing controller calibration layers.

**Phase mapping:** P2, P5

## Phase-Specific Warning Matrix

| Phase Topic | Likely Pitfall | Fast Mitigation |
|-------------|---------------|-----------------|
| P1 Coordinate model contract | Frame convention drift, rotation-order confusion | Write frame contract doc + identity/inverse tests before coding |
| P2 Tracking math integration | Double compensation, singularities, loop starvation | One canonical transform pipeline, clamp math domains, benchmark loop time |
| P3 Persistence + API | Unit mismatch, backward compatibility regressions | Degree-only API contract, schema validation, migration fixtures |
| P4 UI + validation | Direction semantic confusion, unsafe inputs | Explicit diagram/tooltips, hard validation and bounds |
| P5 Verification + field calibration | Time convention mixups, under-tested scenarios | Reference-vector regression suite + broad field matrix |

## Sources

- ESP-IDF Watchdogs (official): https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/system/wdts.html (used for loop starvation/watchdog risk framing)
- NREL Solar Position Algorithm page: https://midcdmz.nrel.gov/spa/ (used for solar position dependency and validation context)
- NOAA Solar Position Calculator (legacy conventions warning): https://gml.noaa.gov/grad/solcalc/azel.html (used to flag non-standard longitude/time-zone sign confusion)
- Project code inspection: `src/geometry.h`, `src/geometry.cpp`, `src/heliostat.h`, `src/HeliostatService.cpp`, `.planning/PROJECT.md`, `.planning/codebase/CONCERNS.md`, `.planning/codebase/TESTING.md`

## Confidence Notes

- **HIGH:** Runtime/control-loop and persistence pitfalls tied directly to current code structure.
- **MEDIUM:** Solar convention mismatch risks (authoritative sources confirm convention variability, but exact third-party tools used by operators may vary).
- **MEDIUM:** Recommended phase structure (no existing roadmap phases present yet, so phase labels are proposed for planning alignment).
