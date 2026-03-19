# Project Milestones: Open Heliostat Tilt-Aware Tracking

## v1.0 MVP (Shipped: 2026-03-19)

**Delivered:** End-to-end tilt-aware heliostat support from firmware contract through control behavior and setup UI verification loop.

**Phases completed:** 1-3 (6 plans total)

**Key accomplishments:**
- Added `mountOrientation` (`tiltDeg`, `tiltAzimuthDeg`) to the heliostat REST/persistence contract with bounded patch semantics.
- Integrated tilt-aware transforms into the reflector control path with zero-tilt identity compatibility.
- Delivered a dedicated Mount Orientation setup section in the heliostat UI with inline validation.
- Implemented explicit apply-readback-observe flow and refresh persistence confirmation in the setup UX.
- Added route-level frontend test harness (Vitest/jsdom) and expanded orientation regression coverage.

**Stats:**
- 3 phases, 6 plans
- 37 files changed, 3851 insertions(+), 63 deletions(-)
- Git range: `4e4ef63` -> `f4f20b9`

**Known gaps accepted:**
- No formal milestone audit file was present at completion (`.planning/v1.0-MILESTONE-AUDIT.md` missing).
- Some PlatformIO runtime test paths still depend on local Python/runtime constraints.

**What's next:** Start v1.1 to improve setup productivity (wizard/profile/metrics) and close test environment/runtime gaps.

---

