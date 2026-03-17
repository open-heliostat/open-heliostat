---
phase: 3
slug: setup-ui-and-verification-loop
status: draft
nyquist_compliant: false
wave_0_complete: false
created: 2026-03-18
---

# Phase 3 - Validation Strategy

> Per-phase validation contract for feedback sampling during execution.

---

## Test Infrastructure

| Property | Value |
|----------|-------|
| **Framework** | Existing: svelte-check + ESLint/Prettier. Wave 0 target: Vitest + Svelte Testing Library + jsdom |
| **Config file** | none - Wave 0 adds vitest config |
| **Quick run command** | `cd interface && npm run check` |
| **Full suite command** | `cd interface && npm run lint && npm run check && npm run build` |
| **Estimated runtime** | ~90 seconds |

---

## Sampling Rate

- **After every task commit:** Run `cd interface && npm run check`
- **After every plan wave:** Run `cd interface && npm run lint && npm run check && npm run build`
- **Before `/gsd-verify-work`:** Full suite green + manual device apply/test/observe verification
- **Max feedback latency:** 120 seconds

---

## Per-Task Verification Map

| Task ID | Plan | Wave | Requirement | Test Type | Automated Command | File Exists | Status |
|---------|------|------|-------------|-----------|-------------------|-------------|--------|
| 3-01-01 | 01 | 1 | UI-01 | route integration | `cd interface && npm run check` | ✅ | ⬜ pending |
| 3-01-02 | 01 | 1 | UI-01 | contract validation | `cd interface && npm run lint && npm run check` | ✅ | ⬜ pending |
| 3-02-01 | 02 | 2 | UI-02 | apply loop integration | `cd interface && npm run check && npm run build` | ✅ | ⬜ pending |
| 3-02-02 | 02 | 2 | UI-02 | persistence/refresh flow | `cd interface && npm run lint && npm run check && npm run build` | ✅ | ⬜ pending |

*Status: ⬜ pending · ✅ green · ❌ red · ⚠ flaky*

---

## Wave 0 Requirements

- [ ] `interface/vitest.config.ts` - configure Vitest for route-level UI tests
- [ ] `interface/src/test/setup.ts` - shared jsdom/fetch setup for route tests
- [ ] `interface/src/routes/heliostat/heliostat-orientation.test.ts` - UI-01 setup and validation behavior
- [ ] `interface/src/routes/heliostat/heliostat-apply-loop.test.ts` - UI-02 apply/observe/refresh logic
- [ ] `cd interface && npm install -D vitest @testing-library/svelte jsdom`

---

## Manual-Only Verifications

| Behavior | Requirement | Why Manual | Test Instructions |
|----------|-------------|------------|-------------------|
| Apply and observe updated behavior on hardware | UI-02 | Hardware state and real sun-tracker behavior are device-coupled | Configure orientation in UI, apply, observe updated state values and pointing behavior |
| Refresh/reload persistence confirmation | UI-02 | End-to-end browser + firmware persistence check | Refresh browser after apply and verify orientation values are retained and rendered |

---

## Validation Sign-Off

- [ ] All tasks include automated verify steps or Wave 0 dependency
- [ ] Sampling continuity maintained across both waves
- [ ] Wave 0 plan covers missing route-level tests
- [ ] No watch-mode verification commands
- [ ] Feedback latency < 120s on default checks
- [ ] `nyquist_compliant: true` set when Wave 0 complete

**Approval:** pending
