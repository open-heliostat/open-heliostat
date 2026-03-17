# Project Research Summary

**Project:** Open Heliostat
**Domain:** Embedded heliostat control firmware + web configuration UI (tilt-aware mount orientation)
**Researched:** 2026-03-17
**Confidence:** HIGH

## Executive Summary

This project is an embedded mechatronics product: ESP32 firmware computes sun/target geometry and drives actuators, while a Svelte UI configures and validates behavior. Expert implementations in this space keep configuration, transport, and control math separated, then apply coordinate-frame transforms exactly once near actuator command output. For this milestone, the recommended strategy is to extend the existing geometry and controller stack with an explicit mount-orientation model (`tiltDeg`, `tiltAzimuthDeg`) and a deterministic world-to-mount transform layer.

The strongest implementation path is incremental and backward-compatible: preserve current APIs and SolarPosition integration, keep persisted defaults at identity (`tiltDeg=0`), and run transform math in `float` in hot control paths on ESP32-S3 for predictable loop cost. The UI should expose clear orientation inputs with strict validation and immediate apply/test feedback, while persistence continues through existing `HeliostatService` and `/config/heliostat.json` patterns.

Primary risks are frame-convention drift, rotation-order/sign errors, and silent unit mismatches at API boundaries. Mitigation is straightforward if enforced early: a written frame contract, identity/inverse transform tests, degree-only public schema, and migration fixtures proving legacy behavior remains unchanged at zero tilt.

## Key Findings

### Recommended Stack

Research converges on keeping the existing stack and adding minimal, explicit geometry primitives rather than introducing heavy robotics libraries. The firmware should continue using the current `SolarPosition` pipeline and existing geometry/control modules, with mount-orientation transforms inserted into controller math. Per-loop math should favor `float` on ESP32-S3 due to hardware acceleration and lower runtime cost versus software-emulated `double`.

**Core technologies:**
- ESP32 firmware C++ (`geometry.*`, `heliostat` controller): control and transform math in deterministic embedded path — lowest migration risk and highest auditability.
- Existing `SolarPosition` dependency: world-frame sun position source — already integrated, avoids milestone churn.
- Svelte 5 + SvelteKit 2 UI: orientation configuration and validation surface — preserves current frontend architecture and deployment model.
- Existing REST/stateful persistence (`/rest/heliostat`, `HeliostatService`, `FSPersistence`): durable config flow — keeps compatibility with current clients and boot behavior.

### Expected Features

The MVP must deliver a complete two-parameter orientation model and integrate it into tracking behavior, not just UI-level offsets. Reliability requires strict validation, persistent config, and a quick apply-test-observe loop for commissioning.

**Must have (table stakes):**
- Two-parameter mount orientation (`tiltDeg` + `tiltAzimuthDeg`) with strict bounds and explicit units.
- Orientation-aware transform inside tracking/control math before actuator commands.
- Backward-compatible defaults that preserve current behavior for legacy installs.
- Persistent orientation config with robust invalid-input handling.
- Basic setup UI for editing, validating, applying, and observing outcomes.

**Should have (competitive):**
- Guided orientation/setup wizard.
- Live residual error metric for verification confidence.
- Profile save/load for repeat deployments.

**Defer (v2+):**
- Sensor-driven auto-orientation (phone/browser as source of truth).
- Drift analytics and long-horizon correction trends.
- Rich visualization frameworks or major solar-engine migration.

### Architecture Approach

Architecture guidance is consistent: keep mount orientation as configuration in controller/service state, but apply transforms only in domain control math immediately before final actuator angle output. Services and router layers should parse/serialize/persist orientation fields, while UI remains a canonical config editor. The control loop remains structurally unchanged except for a single world-to-mount conversion stage.

**Major components:**
1. UI heliostat configuration page: capture and validate mount-orientation inputs.
2. REST/event + `HeliostatService` router: update state, persist config, emit events.
3. Controller + geometry layer: compute reflection in world frame and transform to mount frame once.
4. Persistence (`/config/heliostat.json`): additive schema with identity defaults.
5. Motor/control loop: consume final transformed azimuth/elevation without API changes.

### Critical Pitfalls

1. **Frame convention drift** (azimuth zero/sign/frame mismatch) — prevent with a written frame contract and golden-vector tests.
2. **Wrong rotation composition/order/sign** — enforce named transforms (`world_to_mount`, `mount_to_world`) and inverse-property tests.
3. **Degree/radian boundary errors** — keep API fields in degrees and convert only at the math boundary.
4. **Backward-compatibility break at default config** — require zero-tilt identity behavior and migration fixtures for legacy configs.
5. **Double compensation with existing offsets** — define correction order and keep orientation separate from motor-offset semantics.

## Implications for Roadmap

Based on cross-document synthesis, suggested phase structure:

### Phase 1: Coordinate Contract and Geometry Foundation
**Rationale:** Every later step depends on consistent frame semantics and correct transform primitives.
**Delivers:** Frame/units contract, orientation data model, pure transform helpers, identity/inverse test suite.
**Addresses:** Table-stakes correctness and backward-compatible defaults.
**Avoids:** Frame drift, rotation-order bugs, unit mismatch.

### Phase 2: Controller Integration and Runtime Safety
**Rationale:** Core product value is correct mount-aware tracking in live control.
**Delivers:** World-frame reflection then single world-to-mount transform before `setPosition()`, zero-tilt identity gate, loop-time instrumentation.
**Uses:** Existing firmware stack + SolarPosition + float-optimized math path.
**Implements:** Controller/domain transform placement pattern.
**Avoids:** Double compensation, singularity instability, loop starvation.

### Phase 3: Persistence and API Contract
**Rationale:** Commissioning reliability requires durable and explicit configuration semantics.
**Delivers:** Additive `mountOrientation` JSON schema, router parse/serialize updates, save-map integration, migration fixtures.
**Addresses:** Persistent configuration and fault-tolerant input handling.
**Avoids:** Backward-compat regressions and silent invalid fallback.

### Phase 4: UI Setup and Validation UX
**Rationale:** Field success depends on users entering the right numbers with clear semantics.
**Delivers:** Orientation inputs with bounds/tooltips, apply-test-observe flow, websocket roundtrip confirmation.
**Addresses:** Basic setup UX plus calibration feedback loop.
**Avoids:** Direction confusion and unsafe command generation from bad input.

### Phase 5: System Verification and Field Calibration Hardening
**Rationale:** Outdoor geometric systems fail at edge cases; broad matrix verification is mandatory before release.
**Delivers:** Test matrix across tilt/direction/time/latitude, legacy-upgrade smoke tests, timezone/DST cross-checks against trusted reference.
**Addresses:** Reliability and release confidence.
**Avoids:** Under-tested scenarios, time-convention false positives, install-specific overfitting.

### Phase Ordering Rationale

- Geometry contract and transform primitives are hard dependencies for safe integration.
- Runtime math integration must precede API/UI so behavior is stable before exposing controls.
- Persistence/API then UI follows existing architecture layering and minimizes regression surface.
- Verification comes last but is planned early due high-risk edge cases in coordinate/time handling.

### Research Flags

Phases likely needing deeper research during planning:
- **Phase 2:** Numeric edge-case strategy (horizon singularities, clamping policy) and control-loop budget measurements on target hardware.
- **Phase 5:** External reference alignment procedure (SPA/NOAA convention mapping) and field validation protocol.

Phases with standard patterns (skip research-phase):
- **Phase 3:** Additive JSON schema + existing service persistence flow is already established in this codebase.
- **Phase 4:** Svelte form validation and REST roundtrip patterns are straightforward extensions of existing UI architecture.

## Confidence Assessment

| Area | Confidence | Notes |
|------|------------|-------|
| Stack | HIGH | Strong alignment with existing code and official ESP-IDF performance guidance for float/double behavior. |
| Features | HIGH | Clear table-stakes vs differentiators; directly grounded in project requirements and current UX surfaces. |
| Architecture | HIGH | Derived from current component boundaries and service/controller flow in repository. |
| Pitfalls | MEDIUM-HIGH | Risks are concrete and well supported, but some field-condition/time-convention effects require empirical validation. |

**Overall confidence:** HIGH

### Gaps to Address

- Confirm exact tilt-direction semantic (uphill/downhill reference) in UI and docs with one canonical diagram before implementation lock.
- Define acceptance thresholds for pointing error and loop-latency regression in field verification plan.
- Validate timezone/DST handling path with fixed reference timestamps to prevent false attribution to orientation math.
- Decide whether `mountOrientation.enabled` is explicit or inferred from `tiltDeg` and keep that consistent across API/UI/firmware.

## Sources

### Primary (HIGH confidence)
- Project research files: `.planning/research/STACK.md`, `.planning/research/FEATURES.md`, `.planning/research/ARCHITECTURE.md`, `.planning/research/PITFALLS.md`
- Existing project architecture and implementation references cited in research: `src/HeliostatService.h`, `src/HeliostatService.cpp`, `src/heliostat.h`, `src/geometry.h`, `src/geometry.cpp`, `src/main.cpp`, `interface/src/routes/heliostat/Heliostat.svelte`
- ESP-IDF documentation (performance, RTOS/FPU behavior, watchdogs):
  - https://docs.espressif.com/projects/esp-idf/en/stable/esp32s3/api-guides/performance/speed.html
  - https://docs.espressif.com/projects/esp-idf/en/stable/esp32s3/api-reference/system/freertos_idf.html
  - https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/system/wdts.html

### Secondary (MEDIUM confidence)
- SolarPosition library documentation/repository: https://github.com/KenWillmott/SolarPosition
- Svelte/SvelteKit official docs:
  - https://svelte.dev/docs/svelte/overview
  - https://svelte.dev/docs/kit/introduction
- NREL SPA reference: https://midcdmz.nrel.gov/spa/

### Tertiary (LOW-MEDIUM confidence)
- pvlib single-axis tracking reference (analogy source): https://pvlib-python.readthedocs.io/en/stable/reference/generated/pvlib.tracking.singleaxis.html
- NREL slope-aware tracker paper context: https://www.nrel.gov/docs/fy20osti/76626.pdf
- NOAA legacy solar calculator convention notes: https://gml.noaa.gov/grad/solcalc/azel.html

---
*Research completed: 2026-03-17*
*Ready for roadmap: yes*
