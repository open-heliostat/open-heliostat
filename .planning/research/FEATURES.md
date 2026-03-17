# Feature Landscape

**Domain:** Orientation-aware heliostat alignment and calibration
**Researched:** 2026-03-17

## Table Stakes

Features users expect for a reliable mount-orientation setup. Missing these makes tilted installs feel broken.

| Feature | Why Expected | Complexity | Notes |
|---------|--------------|------------|-------|
| Two-parameter mount orientation model (`tiltDegrees` + `tiltAzimuthDegrees`) | A single tilt value cannot uniquely define mount plane orientation in 3D; users need both amount and direction | Medium | Must default to level (`0`, direction ignored) for backward compatibility; validate ranges and units clearly |
| Orientation-aware target transform in tracking loop | If install is tilted, sun/target pointing must still be physically correct | High | Transform must be applied before motor command output, not as post-hoc UI offset |
| Backward-compatible safe defaults | Existing level installs must keep existing behavior without reconfiguration | Low | Default orientation state should produce mathematically identical outputs to current behavior |
| Persistent orientation config in existing service patterns | Orientation setup must survive reboot and power loss | Medium | Follow current `/rest/heliostat` + FS persistence pattern used by `HeliostatService` |
| Basic orientation setup UI with validation | Users need one obvious place to configure orientation, with clear labels and bounds | Medium | Add to heliostat/sun-tracker context, not hidden in low-level motor calibration |
| Apply-test-observe calibration loop | Users need a quick way to validate pointing before and after changing orientation | Medium | Show predicted sun az/el and current target error indicators during setup |
| Fault-tolerant input handling | Invalid values should not silently degrade tracking | Low | Reject NaN/out-of-range values; preserve last known-good config |

## Differentiators

Capabilities that increase trust, speed of setup, and field usability beyond baseline.

| Feature | Value Proposition | Complexity | Notes |
|---------|-------------------|------------|-------|
| Guided orientation wizard (3-5 steps) | Reduces install mistakes by sequencing: set location/time -> set mount orientation -> verify on reference target -> save | Medium | Best implemented as stateful UI flow that writes same REST fields as manual form |
| Live residual error metric after transform | Makes calibration quality measurable (for example, angular error to selected reference) | Medium | Reuse existing controller telemetry patterns; expose simple pass/fail thresholds |
| Presets and import/export of site profiles | Speeds repeat deployments and replacement controller commissioning | Medium | Persist named profiles in config store; include rollback to previous profile |
| Assisted orientation estimation using phone/inclinometer input (optional assist, not source of truth) | Faster initial setup in field with coarse auto-fill | Medium | Keep explicit user confirmation before commit; not autonomous mode |
| Verification mode with timestamped before/after snapshots | Helps installers document improvement and troubleshoot drift over time | Medium | Can be saved as lightweight JSON report in filesystem |
| Drift detection warning (orientation vs observed correction trend) | Detects mechanical changes after storms/maintenance | High | Requires trend analysis across sessions and a clear non-spam alert strategy |

## Anti-Features

Features to explicitly avoid because they create regressions, ambiguity, or unsafe behavior.

| Anti-Feature | Why Avoid | What to Do Instead |
|--------------|-----------|-------------------|
| "Tilt-only" orientation setting (no direction) | Underdetermined model causes wrong pointing except in special cases | Require both tilt magnitude and tilt direction |
| Treating mount orientation as encoder offset | Conflates installation geometry with actuator calibration; hard to debug and non-portable | Keep geometry transform and per-axis encoder calibration as separate layers |
| Silent fallback to level mount on invalid orientation | Hides field misconfiguration and creates intermittent behavior | Fail fast with explicit validation errors and last-known-good retention |
| Fully automatic orientation detection from phone/browser sensors | Browser sensor permissions/noise/frame ambiguity make this unreliable for production control | Offer sensor-assisted prefill plus explicit confirmation and manual edit |
| Blocking setup flow that disables all motion for long periods | Poor UX in outdoor commissioning and encourages bypassing calibration | Use non-blocking apply/preview steps with immediate control recovery |
| Overloading existing source/target naming UX for orientation controls | Increases cognitive load and accidental edits to operational targets | Keep orientation in dedicated "Mount Orientation" section tied to sun-tracker/setup context |

## Feature Dependencies

```text
Mount orientation schema + validation
  -> Transform integration in tracking/control math
    -> Live residual/error visualization
      -> Guided wizard verification step

Persistence + REST exposure
  -> UI setup form
    -> Profile save/load

Time/location correctness (existing sun tracker)
  -> Orientation verification quality
```

## Dependency Notes

| Feature | Depends On |
|---------|------------|
| Orientation transform in control loop | Mount orientation schema, existing heliostat run loop, coordinate math path |
| Guided setup wizard | Stable REST endpoints, validation responses, sun-tracker readiness (`isTimeSet`, lat/lon) |
| Residual error metric | Access to commanded vs observed direction or equivalent telemetry |
| Profile import/export | Persistence format versioning and migration guardrails |
| Drift detection | Historical calibration snapshots and threshold policy |

## MVP Recommendation

Prioritize:
1. Two-parameter orientation model with strict validation and safe defaults.
2. Orientation transform integrated in tracking/control math with backward-compatibility tests.
3. Minimal setup UI (manual form + apply/test feedback) in heliostat configuration flow.

Defer:
- Sensor-assisted orientation prefill: useful but not required for first reliable release.
- Drift analytics: high value later, but adds telemetry/history complexity.
- Full multi-step wizard polish: can follow after core correctness is proven.

## Sources

- Project context and requirements: `.planning/PROJECT.md`
- Existing architecture/service constraints: `.planning/codebase/STRUCTURE.md`, `.planning/codebase/CONCERNS.md`
- Current heliostat API and persistence patterns: `src/HeliostatService.h`, `src/HeliostatService.cpp`
- Current heliostat setup UX and calibration surfaces: `interface/src/routes/heliostat/Heliostat.svelte`, `interface/src/lib/components/AccelCalibComp.svelte`
- Tracker orientation parameterization reference (axis tilt/azimuth, slope handling): https://pvlib-python.readthedocs.io/en/stable/reference/generated/pvlib.tracking.singleaxis.html (MEDIUM confidence for direct heliostat transfer)
- NREL slope-aware orientation reference linked from pvlib: https://www.nrel.gov/docs/fy20osti/76626.pdf (MEDIUM confidence relevance; PV trackers, not heliostat mirrors)
