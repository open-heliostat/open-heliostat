# Open Heliostat Tilt-Aware Tracking

## What This Is

Open Heliostat is ESP32 firmware plus a web UI for controlling and configuring a heliostat. The project already supports sun tracking, motor control, sequencing, and remote configuration for level-mounted installations. This milestone adds orientation-aware tracking so the device can operate accurately when the mount plane is tilted relative to ground.

## Core Value

A user can install the heliostat on a non-level surface and still get correct, stable solar tracking without manual compensation hacks.

## Requirements

### Validated

- ✓ Device-hosted web UI and REST/event APIs for control/configuration — existing
- ✓ Sun tracking and heliostat control loop on ESP32 hardware — existing
- ✓ Persistent configuration and feature services in firmware — existing

### Active

- [ ] Add mount-orientation model using two parameters: tilt magnitude and tilt direction
- [ ] Apply mount-orientation transform in tracking/control math so pointing remains correct when tilted (for example 45 degrees)
- [ ] Expose mount-orientation settings in UI/API with persistence and safe defaults
- [ ] Preserve existing behavior for level installations (default compatibility)

### Out of Scope

- Automatic orientation detection from sensors alone — deferred, requires additional calibration design
- Redesigning unrelated controller offsets and existing offset semantics — excluded for this feature

## Context

The codebase is a brownfield ESP32 project with modular service-oriented firmware (`src/*Service.*`) and a static SvelteKit UI in `interface/`. Current tracking assumptions are based on level installation and existing offset controls do not represent mount plane tilt directionality. The requested feature needs explicit orientation modeling because one scalar tilt value is insufficient; both tilt amount and azimuth direction are required.

## Constraints

- **Compatibility**: Default configuration must preserve current level-install behavior — avoid regressions for existing users
- **Hardware**: Must run within current ESP32 runtime/control loop budget — no heavy compute or blocking paths
- **Configuration**: Settings must be user-configurable via existing UI/API patterns and persisted in current config mechanisms

## Key Decisions

| Decision | Rationale | Outcome |
|----------|-----------|---------|
| Represent mount orientation as two angles (tilt magnitude + tilt direction) | Single tilt parameter cannot disambiguate orientation in 3D | — Pending |
| User enters orientation via new UI/API configuration fields | Matches existing operational model and keeps setup explicit | — Pending |
| Default orientation is level mount | Keeps backward compatibility for current installs | — Pending |

---
*Last updated: 2026-03-17 after initialization*
