# Requirements: Open Heliostat Tilt-Aware Tracking

**Defined:** 2026-03-17
**Core Value:** A user can install the heliostat on a non-level surface and still get correct, stable solar tracking without manual compensation hacks.

## v1 Requirements

Requirements for initial release of mount-orientation support.

### Orientation Model

- [ ] **ORNT-01**: User can set mount orientation with two parameters: tilt magnitude and tilt direction.
- [ ] **ORNT-02**: System validates orientation values with explicit units, valid bounds, and clear error responses.

### Control and Geometry

- [ ] **CTRL-01**: Tracking logic applies mount-orientation transform before actuator target computation.
- [ ] **CTRL-02**: Zero-tilt configuration produces behavior equivalent to current level-install operation.

### Persistence and API

- [ ] **CONF-01**: Orientation configuration persists across reboot in heliostat configuration storage.
- [ ] **API-01**: REST API supports read/write of orientation fields using existing heliostat service patterns.

### Setup UX

- [ ] **UI-01**: User can configure orientation in UI through a dedicated setup section with validation.
- [ ] **UI-02**: User can run an apply-test-observe loop in setup to verify orientation changes.

## v2 Requirements

Deferred to a future release.

### Setup Productivity

- **UI-WIZ-01**: User can complete orientation setup using a guided multi-step commissioning wizard.
- **METR-01**: User can view live residual pointing error metrics to evaluate calibration quality.
- **PROF-01**: User can save/load orientation profiles for repeat deployments.

## Out of Scope

Explicit exclusions for this scope.

| Feature | Reason |
|---------|--------|
| Automatic orientation detection as source of truth | Sensor/browser ambiguity and reliability risk for initial release |
| Reworking existing offset semantics | Orientation geometry must remain separate from motor/controller offset behavior |

## Traceability

Which phases cover which requirements. Populated during roadmap creation.

| Requirement | Phase | Status |
|-------------|-------|--------|
| ORNT-01 | Phase 1 | Pending |
| ORNT-02 | Phase 1 | Pending |
| CTRL-01 | Phase 2 | Pending |
| CTRL-02 | Phase 2 | Pending |
| CONF-01 | Phase 1 | Pending |
| API-01 | Phase 1 | Pending |
| UI-01 | Phase 3 | Pending |
| UI-02 | Phase 3 | Pending |

**Coverage:**
- v1 requirements: 8 total
- Mapped to phases: 8
- Unmapped: 0

---
*Requirements defined: 2026-03-17*
*Last updated: 2026-03-17 after roadmap creation*
