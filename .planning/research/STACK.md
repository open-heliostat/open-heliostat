# Technology Stack: Mount-Orientation Tracking (Tilt Magnitude + Tilt Direction)

**Project:** Open Heliostat (existing ESP32 firmware + Svelte UI)
**Scope:** Stack dimension only for mount-orientation support (not full system redesign)
**Researched:** 2026-03-17
**Overall confidence:** MEDIUM-HIGH

## Recommended Stack (Prescriptive)

### 1) Firmware Math Core (C++ / existing codebase)

| Choice | Version/Type | Use | Why this fits ESP32 + current code |
|---|---|---|---|
| Keep existing `geometry.h/.cpp` and extend it | Existing in repo | Add small, explicit 3D transform utilities (3x3 matrix + vector ops) | Zero new heavy dependency, deterministic, easy to audit in embedded control path |
| Use `float` in control-path transform math | ESP-IDF guidance for ESP32-S3 | Per-loop vector transforms and matrix multiplies | ESP-IDF docs explicitly note `double` is software-emulated and much slower; `float` is hardware-accelerated on S3 |
| Keep config/API values as `double` at boundary if needed, cast to `float` in hot path | Existing style already mixes doubles | Maintain API precision while keeping runtime fast | Minimal behavioral churn and avoids slow math where it matters |

**Do this:** add a minimal `MountFrame` utility in firmware with:
- `vec3 normalFromTilt(float tiltDeg, float directionDeg)`
- `mat3 worldToMount(float tiltDeg, float directionDeg)`
- `vec3 transformWorldToMount(const mat3&, const vec3&)`
- `vec3 transformMountToWorld(const mat3&, const vec3&)`

### 2) Solar Position Source

| Choice | Version/Type | Use | Why |
|---|---|---|---|
| Keep `SolarPosition` library currently in use | Existing PlatformIO dependency (`SolarPosition`) | Sun azimuth/elevation in world frame | Already integrated and working in current architecture; no migration risk during tilt feature |
| Keep UTC-time discipline (`time(nullptr)` provider) | Existing implementation in `sun.cpp` | Stable sun vector calculation | Matches `SolarPosition` requirement that calculations are based on UTC |

**Do this:** treat `SolarPosition` output as world-frame sun direction, then transform into mount frame before actuator command conversion.

### 3) Orientation Algorithm (Core Recommendation)

Use a **frame-transform-first** approach, not ad-hoc angle offsets.

#### Canonical frame definitions
- World frame: existing level reference used by current azimuth/elevation semantics.
- Mount frame: frame whose +Z is mount normal (derived from tilt magnitude + tilt direction).
- Mirror normal command is solved in mount frame, then mapped to actuator azimuth/elevation.

#### Recommended math pipeline
1. Convert source and target spherical coordinates to world vectors:
   - `s_world = toCartesian(sourceAzEl)`
   - `t_world = toCartesian(targetAzEl)`
2. Compute desired mirror normal in world frame:
   - `n_world = normalize(s_world + t_world)`
3. Build mount transform from tilt params and transform:
   - `n_mount = R_world_to_mount * n_world`
4. Convert `n_mount` to mount-frame azimuth/elevation and feed existing controller targets.

This keeps reflector physics correct while isolating tilt handling in one transform layer.

### 4) UI/TypeScript Layer

| Choice | Version/Type | Use | Why |
|---|---|---|---|
| Keep current Svelte 5 + SvelteKit 2 stack | From existing `interface/package.json` | Add two config inputs and validation/help text | No framework churn; this is configuration UI, not rendering-heavy 3D |
| No runtime 3D math library in browser | Avoid adding gl-matrix unless later needed for visualization | Settings entry + lightweight preview only | Prevents dependency bloat and keeps embedded web UI small/simple |

**Do this:** add UI fields for:
- `tiltMagnitudeDeg` (0..89.9)
- `tiltDirectionDeg` (0..360)

Persist under existing `/rest/heliostat` + `/config/heliostat.json` model.

### 5) Validation/Tooling

| Choice | Version/Type | Use | Why |
|---|---|---|---|
| Add deterministic transform test vectors (firmware-side) | C++ unit-style checks or host-build checks | Validate frame math invariants | Catches sign/axis mistakes early (most common tilt bugs) |
| Add golden-case fixtures | Simple JSON/CSV vectors generated once | Regression checks across refactors | Low effort, high confidence for geometry correctness |

**Minimum test set:**
- Zero tilt is identity transform.
- 45 degree tilt toward cardinal directions produces expected rotated up-vector.
- Roundtrip: `mountToWorld(worldToMount(v)) ~= v` within epsilon.
- Reflection symmetry still holds for known sun/target pairs.

## What Not To Add

1. Do not add full robotics/SLAM libraries (Eigen-heavy stacks, ROS-style kinematics toolchains) for this feature.
2. Do not switch solar-position engine during this milestone (no SPA migration now).
3. Do not add quaternion-only API as the persisted user model; keep user-facing tilt magnitude + direction.
4. Do not implement sensor-fusion auto-calibration in this phase (explicitly out of scope in project requirements).
5. Do not introduce browser-side 3D frameworks (Three.js) unless a later milestone requires rich visualization.

## Practical 2026 Decision Summary

- **Best practical stack:** extend existing geometry module with a tiny, explicit 3x3 transform layer, keep existing `SolarPosition`, keep existing REST/stateful-service persistence pattern, add only two UI fields.
- **Key performance rule on ESP32:** run per-loop transform math in `float`, not `double`, for predictable control-loop cost.
- **Key correctness rule:** always solve mirror normal in world frame first, then transform into mount frame for actuator commands.

## Confidence by Area

| Area | Confidence | Notes |
|---|---|---|
| ESP32 numeric/perf guidance | HIGH | Backed by ESP-IDF docs stating `double` is software-emulated and slower; `float` hardware behavior documented |
| Solar library choice (keep existing) | MEDIUM-HIGH | Strong fit for incremental feature; library itself is older and not frequently updated |
| Transform algorithm choice | HIGH | Standard rigid-body frame transform pattern; directly compatible with current vector/bisector logic |
| UI/dependency minimization | MEDIUM-HIGH | Strongly aligned with existing architecture and embedded constraints |

## Sources

- ESP-IDF (ESP32-S3) speed optimization notes (float vs double guidance):
  - https://docs.espressif.com/projects/esp-idf/en/stable/esp32s3/api-guides/performance/speed.html
- ESP-IDF FreeRTOS (IDF) notes on FPU behavior and `double` software implementation:
  - https://docs.espressif.com/projects/esp-idf/en/stable/esp32s3/api-reference/system/freertos_idf.html
- SolarPosition library documentation/repo (UTC-time expectations, API shape):
  - https://github.com/KenWillmott/SolarPosition
- ArduinoJson v7 documentation and release notes (current ecosystem status for existing stack):
  - https://arduinojson.org/v7/
  - https://arduinojson.org/news/2024/01/03/arduinojson-7/
- NREL SPA reference (benchmark/accuracy context, if future migration considered):
  - https://midcdmz.nrel.gov/spa/
- Svelte/SvelteKit official docs (current UI stack context):
  - https://svelte.dev/docs/svelte/overview
  - https://svelte.dev/docs/kit/introduction
