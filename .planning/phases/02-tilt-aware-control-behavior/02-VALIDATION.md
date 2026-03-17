---
phase: 2
slug: tilt-aware-control-behavior
status: draft
nyquist_compliant: false
wave_0_complete: false
created: 2026-03-17
---

# Phase 2 - Validation Strategy

> Per-phase validation contract for feedback sampling during execution.

---

## Test Infrastructure

| Property | Value |
|----------|-------|
| **Framework** | PlatformIO + Unity |
| **Config file** | platformio.ini |
| **Quick run command** | `pio test -e esp32-s3-devkitc-1` |
| **Full suite command** | `pio test -e esp32-s3-devkitc-1 && pio run -e esp32-s3-devkitc-1` |
| **Estimated runtime** | ~120 seconds |

---

## Sampling Rate

- **After every task commit:** Run `pio test -e esp32-s3-devkitc-1`
- **After every plan wave:** Run `pio test -e esp32-s3-devkitc-1 && pio run -e esp32-s3-devkitc-1`
- **Before `/gsd-verify-work`:** Full suite must be green
- **Max feedback latency:** 180 seconds

---

## Per-Task Verification Map

| Task ID | Plan | Wave | Requirement | Test Type | Automated Command | File Exists | Status |
|---------|------|------|-------------|-----------|-------------------|-------------|--------|
| 2-01-01 | 01 | 1 | CTRL-01 | unit | `pio test -e esp32-s3-devkitc-1 --filter test_tilt_control_transform` | ❌ W0 | ⬜ pending |
| 2-01-02 | 01 | 1 | CTRL-02 | unit | `pio test -e esp32-s3-devkitc-1 --filter test_tilt_zero_equivalence` | ❌ W0 | ⬜ pending |

*Status: ⬜ pending · ✅ green · ❌ red · ⚠ flaky*

---

## Wave 0 Requirements

- [ ] `test/test_tilt_control_transform.cpp` - CTRL-01 transform placement and sign/order checks
- [ ] `test/test_tilt_zero_equivalence.cpp` - CTRL-02 identity/equivalence checks at zero tilt
- [ ] shared spherical comparison helper for tolerance + azimuth wrap handling

---

## Manual-Only Verifications

| Behavior | Requirement | Why Manual | Test Instructions |
|----------|-------------|------------|-------------------|
| Runtime orientation update applies without restart | CTRL-01 | Requires live runtime state mutation sequence | Update orientation via REST while running, confirm subsequent control outputs change on next cycles |
| Level-install compatibility in live controller behavior | CTRL-02 | Hardware/control-loop parity check beyond unit harness | Set tilt to zero and compare actuator targets against pre-transform baseline scenarios |

---

## Validation Sign-Off

- [ ] All tasks have `<automated>` verify or Wave 0 dependencies
- [ ] Sampling continuity: no 3 consecutive tasks without automated verify
- [ ] Wave 0 covers all missing references
- [ ] No watch-mode flags
- [ ] Feedback latency < 180s
- [ ] `nyquist_compliant: true` set in frontmatter

**Approval:** pending
