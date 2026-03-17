# Roadmap: Open Heliostat Tilt-Aware Tracking

## Overview

This roadmap delivers tilt-aware heliostat tracking by first establishing a complete orientation configuration contract, then integrating orientation into control math with backward-compatible behavior, and finally providing a setup UX that supports practical field verification.

## Phases

**Phase Numbering:**
- Integer phases (1, 2, 3): Planned milestone work
- Decimal phases (2.1, 2.2): Urgent insertions (marked with INSERTED)

Decimal phases appear between their surrounding integers in numeric order.

- [ ] **Phase 1: Orientation Configuration Contract** - Define, validate, persist, and expose mount orientation settings through firmware APIs.
- [ ] **Phase 2: Tilt-Aware Control Behavior** - Apply mount-orientation transforms in tracking while preserving level-install compatibility.
- [ ] **Phase 3: Setup UI and Verification Loop** - Deliver a dedicated setup workflow for configure/apply/test/observe orientation changes.

## Phase Details

### Phase 1: Orientation Configuration Contract
**Goal**: Users can set mount orientation reliably through API-backed configuration with persistence and safe defaults.
**Depends on**: Nothing (first phase)
**Requirements**: ORNT-01, ORNT-02, CONF-01, API-01
**Success Criteria** (what must be TRUE):
  1. User can read and write orientation using tilt magnitude and tilt direction through the existing REST heliostat patterns.
  2. Invalid orientation inputs return clear validation errors with explicit bounds/units.
  3. Orientation settings persist across reboot and reload correctly from heliostat configuration storage.
  4. Existing devices without orientation settings default to level-mount behavior without manual migration.
**Plans**: TBD

### Phase 2: Tilt-Aware Control Behavior
**Goal**: Tracking outputs remain correct for tilted mounts while preserving current behavior at zero tilt.
**Depends on**: Phase 1
**Requirements**: CTRL-01, CTRL-02
**Success Criteria** (what must be TRUE):
  1. With non-zero orientation configured, tracking applies mount-orientation transform before actuator target computation.
  2. With zero tilt configured, output behavior is equivalent to current level-install operation.
  3. Updating orientation settings changes subsequent tracking targets without requiring firmware restart.
**Plans**: TBD

### Phase 3: Setup UI and Verification Loop
**Goal**: Users can configure orientation confidently and verify effect through an explicit setup workflow.
**Depends on**: Phase 2
**Requirements**: UI-01, UI-02
**Success Criteria** (what must be TRUE):
  1. User can configure tilt magnitude and tilt direction in a dedicated setup section with inline validation.
  2. User can apply orientation changes and immediately observe updated behavior for verification.
  3. User can complete an apply-test-observe loop and confirm values remain available after refresh/reload.
**Plans**: TBD

## Progress

**Execution Order:**
Phases execute in numeric order: 1 -> 2 -> 3

| Phase | Plans Complete | Status | Completed |
|-------|----------------|--------|-----------|
| 1. Orientation Configuration Contract | 0/TBD | Not started | - |
| 2. Tilt-Aware Control Behavior | 0/TBD | Not started | - |
| 3. Setup UI and Verification Loop | 0/TBD | Not started | - |
