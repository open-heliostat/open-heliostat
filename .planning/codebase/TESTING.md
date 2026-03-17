# Testing Patterns

**Analysis Date:** 2026-03-17

## Test Framework

**Runner:**
- Dedicated automated test framework for firmware or interface is **not detected** in repository root or `interface/` (no Jest/Vitest/Playwright/Cypress config files found).
- `tests/` directory exists but is currently empty (`tests/`).
- CI workflow in `.github/workflows/ci.yaml` deploys documentation only; it does not execute firmware/interface tests.

**Assertion Library:**
- Not detected (no active unit/integration test harness configured).

**Run Commands:**
```bash
cd interface && npm run check   # Type/svelte validation (not a test runner)
cd interface && npm run lint    # Formatting + lint validation
cd interface && npm run build   # Production build verification
pio run                         # Firmware compile validation
pio test                        # PlatformIO test entrypoint (currently no test cases under tests/)
mkdocs build                    # Docs build validation
```

## Test File Organization

**Location:**
- Intended firmware test location is `tests/` (PlatformIO standard), but no test files are present.
- No co-located frontend `*.test.*` / `*.spec.*` files were detected under `interface/src/`.

**Naming:**
- Not established in current codebase (no discovered test files to infer naming convention).

**Structure:**
```
tests/                         # Present, currently empty
interface/src/**               # No *.test.ts / *.spec.ts / *.test.svelte detected
.github/workflows/ci.yaml      # Docs deploy only
```

## Test Structure

**Suite Organization:**
```typescript
// Not detected in repository: no describe()/it()/test() suites present.
```

**Patterns:**
- Setup pattern: Not detected.
- Teardown pattern: Not detected.
- Assertion pattern: Not detected.

## Mocking

**Framework:**
- Not detected.

**Patterns:**
```typescript
// Not detected: no mock framework usage found in active test files.
```

**What to Mock:**
- Prescriptive recommendation based on architecture:
  - Mock browser `fetch` and WebSocket payloads for frontend store logic in `interface/src/lib/stores/rest.ts` and `interface/src/lib/stores/socket.ts`.
  - Mock peripheral/transport boundaries for firmware service logic around JSON routers in `src/HeliostatService.cpp` and `src/ESPNowService.cpp`.

**What NOT to Mock:**
- Do not mock pure data transformations and geometry math in `src/geometry.h`; keep deterministic calculations as direct unit assertions.
- Do not mock type-only contracts in `interface/src/lib/types/models.ts`; validate runtime adapters instead.

## Fixtures and Factories

**Test Data:**
```typescript
// No fixture/factory modules currently exist.
// Existing reusable shape definitions live in:
// interface/src/lib/types/models.ts
```

**Location:**
- Not detected (no test fixture directories or seed builders).

## Coverage

**Requirements:**
- No coverage threshold or enforcement detected.

**View Coverage:**
```bash
# Not available with current repository configuration.
# Add a test runner (e.g., Vitest/Jest) before coverage reporting can be generated.
```

## Test Types

**Unit Tests:**
- Not implemented in repository at present.

**Integration Tests:**
- Not implemented in repository at present.

**E2E Tests:**
- Not used (no Playwright/Cypress or equivalent setup detected).

## Current Quality Validation Pattern (Non-test)

- Frontend quality gates are command-based and manually runnable from `interface/package.json`:
  - `check` (Svelte + TypeScript validation)
  - `lint` (Prettier + ESLint)
  - `build` (Vite compile)
- Firmware quality gate is primarily compile/build via PlatformIO in `platformio.ini` and auxiliary scripts in `scripts/`.
- Documentation gate exists in CI: `.github/workflows/ci.yaml` runs `mkdocs gh-deploy --force`.

## Common Patterns

**Async Testing:**
```typescript
// No explicit async test pattern present.
// Closest production async pattern is async fetch + status checks:
// interface/src/lib/stores/rest.ts
```

**Error Testing:**
```typescript
// No explicit error-test pattern present.
// Production error handling uses try/catch + console.error:
// interface/src/lib/stores/rest.ts
// interface/src/routes/wifi/sta/Wifi.svelte
```

## Recommended Baseline for New Tests (Aligned to Existing Code)

- Firmware tests: place first test modules under `tests/` and invoke with `pio test`.
- Frontend unit tests: colocate as `*.test.ts` next to stores/components, starting with:
  - `interface/src/lib/stores/rest.ts`
  - `interface/src/lib/stores/socket.ts`
  - `interface/src/lib/components/toasts/notifications.ts`
- Integration tests: add route-level tests for key flows in:
  - `interface/src/routes/+layout.svelte`
  - `interface/src/routes/wifi/sta/Wifi.svelte`
- Keep command checks in CI alongside tests so existing lint/type/build behavior remains enforced.

---

*Testing analysis: 2026-03-17*
