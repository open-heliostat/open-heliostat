# Open Heliostat Tilt-Aware Tracking

## What This Is

Open Heliostat is ESP32 firmware plus a web UI for controlling and configuring a heliostat. v1.0 now supports orientation-aware operation for non-level installations, including firmware contract support, tilt-aware control behavior, and a dedicated setup workflow for configure/apply/verify. The product remains focused on practical commissioning and reliable tracking in real-world mounting conditions.

## Core Value

A user can install the heliostat on a non-level surface and still get correct, stable solar tracking without manual compensation hacks.

## Current State

- **Shipped version:** v1.0 MVP (2026-03-19)
- **Status:** Milestone complete and archived in `.planning/milestones/`
- **Scope delivered:** Orientation contract, control integration, and setup UI verification loop

## Next Milestone Goals

- Improve setup productivity with a guided orientation workflow.
- Add stronger in-product verification signals (residual/error visibility).
- Strengthen test/runtime environment consistency for repeatable CI and local verification.

## Requirements

### Validated

- ✓ Device-hosted web UI and REST/event APIs for control/configuration — existing
- ✓ Sun tracking and heliostat control loop on ESP32 hardware — existing
- ✓ Persistent configuration and feature services in firmware — existing
- ✓ Mount orientation model with tilt magnitude and tilt direction — v1.0
- ✓ Tilt-aware tracking/control behavior with zero-tilt compatibility — v1.0
- ✓ Orientation setup UI with apply/readback verification loop — v1.0

### Active

- [ ] Guided orientation setup wizard for faster commissioning
- [ ] Residual/error indicators to quantify setup quality during verification
- [ ] Profile save/load flow for repeated deployment scenarios
- [ ] Stabilize automated runtime test execution across local and CI environments

### Out of Scope

- Automatic orientation detection from sensors alone — deferred, requires additional calibration design
- Redesigning unrelated controller offsets and existing offset semantics — excluded for this feature

## Context

The codebase is a brownfield ESP32 project with modular service-oriented firmware (`src/*Service.*`) and a static SvelteKit UI in `interface/`. v1.0 established a two-parameter orientation contract, integrated transform-aware control logic, and delivered UI setup/apply verification flows. Remaining work centers on setup productivity, richer operator feedback, and hardening validation workflows.

## Constraints

- **Compatibility**: Default configuration must preserve current level-install behavior — avoid regressions for existing users
- **Hardware**: Must run within current ESP32 runtime/control loop budget — no heavy compute or blocking paths
- **Configuration**: Settings must be user-configurable via existing UI/API patterns and persisted in current config mechanisms

## Key Decisions

| Decision | Rationale | Outcome |
|----------|-----------|---------|
| Represent mount orientation as two angles (tilt magnitude + tilt direction) | Single tilt parameter cannot disambiguate orientation in 3D | ✓ Good |
| User enters orientation via new UI/API configuration fields | Matches existing operational model and keeps setup explicit | ✓ Good |
| Default orientation is level mount | Keeps backward compatibility for current installs | ✓ Good |
| Apply/readback UI loop instead of implicit auto-save edits | Commissioning clarity and verifiable operator intent | ✓ Good |

---
*Last updated: 2026-03-19 after v1.0 milestone*
