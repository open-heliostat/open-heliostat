# Architecture Patterns

**Domain:** ESP32 heliostat mount-orientation integration
**Researched:** 2026-03-17
**Confidence:** HIGH (derived from current project code paths and service patterns)

## Recommended Architecture

Add mount orientation as a first-class configuration object inside heliostat state, but keep application of the transform inside control math (controller/domain layer), not in transport/service/router code.

Conceptually:

- `MountOrientation` lives in `HeliostatController` state and is persisted via existing `HeliostatService` + `FSPersistence`.
- `HeliostatController::reflectCurrentSource()` computes reflection in world coordinates, then maps it into mount coordinates through a deterministic orientation transform.
- Existing azimuth/elevation motor controller APIs remain unchanged (`setPosition(azimuth, elevation)` still receives final mount-frame angles).

This preserves service contracts and loop timing while adding a new math stage.

## Component Boundaries

| Component | Responsibility | Communicates With |
|-----------|---------------|-------------------|
| UI heliostat page (`interface/src/routes/heliostat/Heliostat.svelte`) | Capture/edit mount orientation fields and display current values | REST `/rest/heliostat`, websocket `heliostat-service` |
| REST/Event transport (`HttpStateRouterEndpoint`, `EventEndpoint`) | Accept JSON patches and publish state snapshots/events | `HeliostatService`, UI |
| Heliostat service/router (`src/HeliostatService.h`, `src/HeliostatService.cpp`) | Parse/serialize orientation fields, define save-map, trigger persistence on change | `HeliostatController`, `FSPersistence` |
| Persistence (`/config/heliostat.json`) | Store orientation config with backward-compatible defaults | `FSPersistence`, service load on boot |
| Control/domain (`src/heliostat.h`, `src/geometry.*`) | Apply world->mount and mount->world transforms, run reflection math, produce final motor targets | azimuth/elevation controllers |
| Motor/control loop (`src/main.cpp` control task, controller drivers) | Execute high-rate control updates only on final target angles | `HeliostatController::runLoop()` |

Boundary rule: orientation representation and persistence are config concerns; transform application is strictly control-math concern.

## Data Model

Use two explicit parameters plus optional version/enable fields:

- `mountOrientation.tiltDeg`: magnitude of mount tilt relative to level plane (default `0.0`)
- `mountOrientation.tiltAzimuthDeg`: compass direction of downhill/uphill axis (choose one convention and document; default `0.0`)
- Optional: `mountOrientation.enabled` (default `false` or inferred `tiltDeg == 0`)
- Optional: `mountOrientation.version` for future schema evolution

Recommended JSON under existing heliostat payload:

```json
{
  "mountOrientation": {
    "tiltDeg": 0.0,
    "tiltAzimuthDeg": 0.0,
    "enabled": false,
    "version": 1
  }
}
```

Keep this as a nested object to avoid namespace collisions with existing root-level heliostat keys.

## Data Flow

### Configuration Flow (write path)

1. UI posts partial update to `/rest/heliostat` with `mountOrientation` object.
2. `HeliostatControllerJsonRouter` validates field types/ranges and updates controller state.
3. `StatefulService` marks state changed; `FSPersistence` writes whitelisted fields to `/config/heliostat.json`.
4. `EventEndpoint` emits updated heliostat state over websocket.
5. UI receives event and reflects persisted values.

Direction: UI -> REST/router -> controller state -> persistence + websocket -> UI.

### Runtime Control Flow (read/apply path)

1. `HeliostatService.loop()` triggers `HeliostatController::run()` (existing cadence).
2. `run()` calls reflection path for current source/target in world frame.
3. New transform stage converts world target vector to mount frame using `mountOrientation`.
4. Existing `setPosition()` passes transformed azimuth/elevation to motor controllers.
5. High-rate `runLoop()` executes actuator control unchanged.

Direction: persisted config -> controller runtime state -> transform -> actuator targets.

### Boot Flow

1. `HeliostatService.begin()` loads `/config/heliostat.json`.
2. If orientation fields absent (legacy file), defaults remain level (`tiltDeg=0`).
3. Controller initializes with compatible behavior identical to current release.

Direction: FS config -> router defaults merge -> controller.

## Transform Placement and Contract

Apply orientation transform at exactly one location: immediately before final actuator angle output.

Suggested internal methods in `HeliostatController`:

- `vec3 applyMountOrientation(vec3 worldVector)`
- `SphericalCoordinate worldToMount(SphericalCoordinate world)`
- optionally `SphericalCoordinate mountToWorld(SphericalCoordinate mount)` for diagnostics/calibration

Pipeline:

1. Compute reflection in world space using existing `reflect(source, target)` logic.
2. Convert to cartesian.
3. Rotate by inverse mount orientation matrix (world->mount).
4. Convert back to spherical and normalize azimuth/elevation ranges.

Single-point transform avoids double-application bugs across UI, services, and sequencers.

## Safe Migration Strategy (Backward Compatible)

### Phase A: Schema Additive, No Behavior Change

- Add `mountOrientation` fields to router read/write + save map.
- Default values produce identity transform (`tiltDeg=0`), so all existing installs remain unchanged.
- UI displays fields but does not force user input.

### Phase B: Controlled Runtime Activation

- Enable transform in controller path gated by `enabled` or `tiltDeg > epsilon`.
- Keep old behavior path for zero tilt exactly bit-for-bit where practical.
- Add logs/telemetry for active orientation to ease rollout debugging.

### Phase C: Optional Calibration/Validation Enhancements

- Add helper actions (test vector preview, dry-run reflected angles) without changing core contract.
- Defer auto-detection/calibration integration until separate phase.

Compatibility rules:

- Never require migration of old `/config/heliostat.json`.
- Never repurpose existing `offset` semantics in motor controllers for mount tilt.
- Never change existing REST keys used by current UI automations.

## Build-Order Implications

Recommended implementation order to reduce regression risk:

1. **Domain model first** (`HeliostatController` + geometry helpers)
   - Introduce orientation struct, defaults, and pure transform helpers with unit tests.
2. **Service/router second** (`HeliostatService` JSON map and save map)
   - Wire read/update/persist for new fields.
3. **UI third** (`Heliostat.svelte` + frontend types)
   - Add controls bound to new payload.
4. **Integration verification fourth**
   - Validate boot with legacy config, API read/write, and control behavior at `tilt=0` and non-zero tilt.

Why this order:

- Control math is the core dependency.
- Service and UI can safely layer on top once state contract is stable.
- Migration risks are caught before user-facing changes.

## Patterns to Follow

### Pattern 1: StatefulService + JsonRouter Extension

**What:** Extend existing router map rather than introducing a parallel endpoint.
**When:** Any persistent heliostat configuration field.
**Example:** Add `mountOrientation` parse/serialize and include in save map whitelist.

### Pattern 2: Identity-by-Default Transforms

**What:** New transform returns exact input when disabled/zero tilt.
**When:** Backward-compatible rollout in brownfield control systems.
**Example:**

```cpp
if (!mountOrientation.enabled || fabs(mountOrientation.tiltDeg) < 1e-6) {
    return world;
}
return worldToMount(world);
```

### Pattern 3: Single Source of Truth for Frame Conversion

**What:** Keep frame conversion only in controller math layer.
**When:** Multiple services/UI paths can touch target angles.
**Example:** Sequencers still write world targets; controller owns conversion before actuation.

## Anti-Patterns to Avoid

### Anti-Pattern 1: Applying Orientation in UI

**What:** UI pre-transforms target coordinates before POST.
**Why bad:** Duplicated logic, frame mismatch, hidden drift.
**Instead:** UI sends canonical config/state only; firmware applies transform.

### Anti-Pattern 2: Reusing Motor Offset Fields for Tilt

**What:** Encode mount tilt into existing axis offsets.
**Why bad:** Breaks semantics, calibration conflicts, opaque behavior.
**Instead:** Dedicated `mountOrientation` model with explicit meaning.

### Anti-Pattern 3: Multi-Stage Transform Application

**What:** Partial transform in sequencer and another in controller.
**Why bad:** Double rotation and difficult debugging.
**Instead:** Exactly one transform stage before actuator target assignment.

## Scalability and Runtime Considerations

| Concern | At Current Device Scale | Risk | Mitigation |
|---------|--------------------------|------|------------|
| Control-loop budget | 20 Hz supervisory path + high-rate `runLoop()` | Extra trig/rotations | Precompute sin/cos when config changes; avoid dynamic allocations |
| Persistence churn | Config writes on state changes | Flash wear if noisy writes | Persist only on explicit config updates, not every control cycle |
| API compatibility | Existing clients expect current shape | Parse failures on unknown fields | Make new fields optional and additive |

## Verification Criteria for Architecture Phase

- Legacy config file without `mountOrientation` boots and tracks as before.
- Posting `mountOrientation` persists and round-trips through GET/event payloads.
- With `tiltDeg=0`, actuator targets match pre-feature behavior.
- With non-zero tilt, transform is applied once and produces stable targets.
- No measurable control-loop regression beyond acceptable margin.

## Sources

- Project architecture and layering notes from `.planning/codebase/ARCHITECTURE.md`.
- Current service/router contracts in `src/HeliostatService.h` and `src/HeliostatService.cpp`.
- Current control model in `src/heliostat.h`.
- Existing geometry utilities in `src/geometry.h` and `src/geometry.cpp`.
- Runtime task wiring in `src/main.cpp`.
- Current UI contract in `interface/src/routes/heliostat/Heliostat.svelte`.
