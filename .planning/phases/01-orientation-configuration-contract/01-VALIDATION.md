---
phase: 1
slug: orientation-configuration-contract
status: draft
nyquist_compliant: false
wave_0_complete: false
created: 2026-03-17
---

# Phase 1 - Validation Strategy

> Per-phase validation contract for feedback sampling during execution.

---

## Test Infrastructure

| Property | Value |
|----------|-------|
| **Framework** | none currently configured (Wave 0 required) |
| **Config file** | none - Wave 0 adds test env |
| **Quick run command** | `pio run -e esp32-s3-devkitc-1` |
| **Full suite command** | `pio run -e esp32-s3-devkitc-1` |
| **Estimated runtime** | ~60 seconds |

---

## Sampling Rate

- **After every task commit:** Run `pio run -e esp32-s3-devkitc-1`
- **After every plan wave:** Run `pio run -e esp32-s3-devkitc-1`
- **Before `/gsd-verify-work`:** Firmware build green and contract checks complete
- **Max feedback latency:** 120 seconds

---

## Per-Task Verification Map

| Task ID | Plan | Wave | Requirement | Test Type | Automated Command | File Exists | Status |
|---------|------|------|-------------|-----------|-------------------|-------------|--------|
| 1-01-01 | 01 | 1 | ORNT-01 | manual-only API smoke (Wave 0 pending) | `pio run -e esp32-s3-devkitc-1` | ❌ W0 | ⬜ pending |
| 1-01-02 | 01 | 1 | ORNT-02 | manual-only contract checks (Wave 0 pending) | `pio run -e esp32-s3-devkitc-1` | ❌ W0 | ⬜ pending |
| 1-01-03 | 01 | 1 | CONF-01 | manual-only persistence/reboot check (Wave 0 pending) | `pio run -e esp32-s3-devkitc-1` | ❌ W0 | ⬜ pending |
| 1-01-04 | 01 | 1 | API-01 | manual-only REST/websocket check (Wave 0 pending) | `pio run -e esp32-s3-devkitc-1` | ❌ W0 | ⬜ pending |

*Status: ⬜ pending · ✅ green · ❌ red · ⚠ flaky*

---

## Wave 0 Requirements

- [ ] `test/test_orientation_contract.cpp` - ORNT-01 and ORNT-02 parse, clamp, and patch semantics
- [ ] `test/test_orientation_persistence.cpp` - CONF-01 save-map and legacy-default reload checks
- [ ] `test/test_orientation_api_contract.cpp` - API-01 request/response shape checks
- [ ] `platformio.ini` test environment for `pio test`

---

## Manual-Only Verifications

| Behavior | Requirement | Why Manual | Test Instructions |
|----------|-------------|------------|-------------------|
| Read/write orientation object via REST | ORNT-01 | No automated HTTP test harness yet | POST `mountOrientation` updates, then GET `/rest/heliostat` and verify fields persist in response |
| Range handling and error response | ORNT-02 | Contract currently validated by runtime behavior only | Send out-of-range values and invalid combined payload, verify clamp and generic error behavior |
| Persistence across reboot | CONF-01 | Requires target device reboot cycle | Set orientation, reboot device, GET state, verify values restored from `/config/heliostat.json` |
| Event propagation on update | API-01 | Websocket/event check not automated yet | Subscribe to heliostat event stream, update orientation via REST, verify update event payload includes orientation |

---

## Validation Sign-Off

- [ ] All tasks have automated verify or explicit Wave 0 dependency
- [ ] Sampling continuity maintained during execution
- [ ] Wave 0 covers all missing automated checks
- [ ] No watch-mode flags in verification commands
- [ ] Feedback latency < 120s for build checks
- [ ] `nyquist_compliant: true` set in frontmatter when Wave 0 completed

**Approval:** pending
