# Phase 3: Setup UI and Verification Loop - Research

**Researched:** 2026-03-18
**Domain:** SvelteKit heliostat setup UX and REST-backed verification loop
**Confidence:** HIGH

<user_constraints>
## User Constraints (from CONTEXT.md)

### Locked Decisions
### Setup location and information architecture
- Add orientation setup directly in the existing heliostat page flow, as a dedicated section within `interface/src/routes/heliostat/Heliostat.svelte`.
- Keep orientation setup separate from source/target editing and separate from existing sun tracker location/time controls.
- Reuse existing settings-card structure and grid layout patterns already used in the page.

### Input controls and validation UX
- Show both orientation inputs: `tiltDeg` and `tiltAzimuthDeg` with explicit degree semantics and bounds.
- Reuse existing form components (`Slider`, `GridForm`, `Button`) for consistency with current UI behavior.
- Validation messaging should be inline and immediate in the setup section (do not introduce a new global error system in this phase).

### Apply/test/observe interaction loop
- Provide explicit apply action for orientation changes instead of silent background mutation.
- After apply, immediately re-read heliostat state and show updated observable tracking values in the same page context.
- Keep verification interaction lightweight and operator-focused: configure -> apply -> observe current tracker/target behavior.

### Refresh/reload persistence confirmation
- Orientation values must be loaded from `/rest/heliostat` on page refresh/reload and shown in the setup section.
- The user verification loop includes refresh confirmation as part of setup completion criteria.
- No additional persistence mechanism is added in this phase; use existing REST + fs-backed service behavior from prior phases.

### Claude's Discretion
- Exact visual wording for helper text and section labels.
- Whether to use one shared save/apply button or per-field immediate apply inside the orientation section.

### Deferred Ideas (OUT OF SCOPE)
- Multi-step guided wizard remains deferred (v2 scope).
- Advanced residual error metrics and profile management remain deferred (v2 scope).
</user_constraints>

<phase_requirements>
## Phase Requirements

| ID | Description | Research Support |
|----|-------------|-----------------|
| UI-01 | User can configure orientation in UI through a dedicated setup section with validation. | Add `mountOrientation` state typing in `Heliostat.svelte`, render dedicated section with `Slider` controls, enforce bounds aligned with `orientation_contract.h`, and show inline validation text. |
| UI-02 | User can run an apply-test-observe loop in setup to verify orientation changes. | Use explicit apply button posting `{ mountOrientation: { tiltDeg, tiltAzimuthDeg } }` to `/rest/heliostat`, then immediate re-fetch and in-page observation using existing Sun Tracker telemetry fields. |
</phase_requirements>

## Summary

Phase 3 should be implemented as an in-place extension of the existing heliostat route rather than a new route, modal flow, or wizard. The existing page already has the required primitives: card sections, form controls, a REST read/write loop, and a visible observation context (`sunTracker` data). The most reliable delivery path is to add a dedicated Mount Orientation setup card that uses the existing `Slider`, `GridForm`, and `Button` components with explicit degree-based labels and inline validation messages.

The backend contract is already stable and production-integrated from phases 1 and 2: `/rest/heliostat` accepts and returns `mountOrientation` (`tiltDeg`, `tiltAzimuthDeg`) and persists through fs-backed service flow. The UI should therefore avoid introducing additional transport abstractions or new persistence mechanisms. Implement configure -> apply -> immediate re-read -> observe in one page context, then verify refresh persistence by rehydrating from `/rest/heliostat` on mount.

Because nyquist validation is enabled and there is no current frontend test runner, planning should include a Wave 0 test bootstrap for route logic (Vitest + Testing Library) or explicitly mark the loop as manual-only for device-coupled verification while still automating lint/type/build gates.

**Primary recommendation:** Implement a dedicated orientation card in `interface/src/routes/heliostat/Heliostat.svelte` with one explicit Apply action, immediate read-back, and refresh verification using existing REST and card/layout patterns.

## Standard Stack

### Core
| Library | Version | Purpose | Why Standard |
|---------|---------|---------|--------------|
| `svelte` | 5.54.0 (published 2026-03-17) | Route/component state and UI rendering | Already used throughout interface; route file is Svelte-first and this phase is route-centric. |
| `@sveltejs/kit` | 2.55.0 (published 2026-03-12) | App routing/build conventions | Existing project foundation; phase should stay in existing route architecture. |
| `vite` | 8.0.0 (published 2026-03-12) | Dev/build toolchain for interface | Existing command workflow (`dev`, `build`) and static output pipeline already wired. |
| Existing UI components (`SettingsCard`, `GridForm`, `Slider`, `Button`) | repository local | Consistent setup UX implementation | Locked decision explicitly requires reuse and avoids style/behavior divergence. |

### Supporting
| Library | Version | Purpose | When to Use |
|---------|---------|---------|-------------|
| `daisyui` | 5.5.19 (published 2026-02-20) | Existing utility/component styling baseline | Use current alert/card/button utility classes for inline feedback states. |
| `svelte-check` | 4.2.2 (project-installed) | Type + Svelte diagnostics | Required quick gate per task commit for this UI-heavy phase. |
| `vitest` | 4.1.0 (published 2026-03-12) | UI logic unit/integration tests (recommended Wave 0) | Add if phase plans require automated route logic verification beyond lint/type/build. |
| `@testing-library/svelte` + `jsdom` | 5.3.1 / 29.0.0 | Component interaction tests in DOM-like env | Use for apply/read-back state transition tests without hardware coupling. |

### Alternatives Considered
| Instead of | Could Use | Tradeoff |
|------------|-----------|----------|
| Existing heliostat route extension | New setup route/wizard | Adds navigation/state complexity and violates locked in-flow decision for this phase. |
| Shared in-page inline validation | Global toast/error framework | Over-scopes phase and conflicts with locked decision to avoid global error system changes. |
| Existing REST helpers (`getJsonRest`, `postJsonRest`) | New fetch wrapper/store abstraction | Unnecessary refactor risk; this phase needs focused UX loop delivery. |

**Installation:**
```bash
# No new runtime dependency required for UI implementation itself.
# Optional Wave 0 validation bootstrap if automated route tests are planned:
cd interface && npm install -D vitest @testing-library/svelte jsdom
```

**Version verification:**
```bash
npm view svelte version
npm view @sveltejs/kit version
npm view vite version
npm view daisyui version
npm view vitest version
npm view @testing-library/svelte version
npm view jsdom version
```

## Architecture Patterns

### Recommended Project Structure
```text
interface/src/routes/heliostat/
└── Heliostat.svelte            # Add Mount Orientation setup section + apply/read-back workflow

interface/src/lib/stores/
└── rest.ts                     # Reuse existing REST helpers, no new transport layer

src/
├── HeliostatService.cpp        # Existing /rest/heliostat mountOrientation + sunTracker payload contract
└── orientation_contract.h      # Authoritative orientation bounds/field semantics
```

### Pattern 1: Local Draft State + Explicit Apply
**What:** Keep user-edited orientation values in route-local draft variables, validate inline, and only send to backend on explicit Apply.
**When to use:** For orientation settings where operator intent and immediate feedback matter.
**Example:**
```typescript
// Source: existing route/API patterns in interface/src/routes/heliostat/Heliostat.svelte
// and mountOrientation contract in src/HeliostatService.cpp
let orientationDraft = { tiltDeg: 0, tiltAzimuthDeg: 0 };
let orientationError = '';

function validateOrientation() {
  const { tiltDeg, tiltAzimuthDeg } = orientationDraft;
  if (tiltDeg < -90 || tiltDeg > 90) return 'Tilt must be between -90 and 90 degrees';
  if (tiltAzimuthDeg < 0 || tiltAzimuthDeg > 360) return 'Tilt azimuth must be between 0 and 360 degrees';
  return '';
}

async function applyOrientation() {
  orientationError = validateOrientation();
  if (orientationError) return;
  await postJsonRest('/rest/heliostat', { mountOrientation: orientationDraft });
  await getHeliostatControllerState(); // immediate read-back for observe step
}
```

### Pattern 2: Rehydrate from Backend as Source of Truth
**What:** On mount and after apply, derive displayed values from `/rest/heliostat` response, not local optimistic-only state.
**When to use:** Refresh/reload persistence verification and preventing stale UI.
**Example:**
```typescript
// Source: current getHeliostatControllerState() pattern in interface/src/routes/heliostat/Heliostat.svelte
onMount(async () => {
  await getHeliostatControllerState();
  orientationDraft = {
    tiltDeg: heliostatControllerState.mountOrientation?.tiltDeg ?? 0,
    tiltAzimuthDeg: heliostatControllerState.mountOrientation?.tiltAzimuthDeg ?? 0
  };
});
```

### Anti-Patterns to Avoid
- **Silent auto-post on every slider tick:** Generates noisy writes and undermines explicit apply/test/observe workflow.
- **Duplicating orientation bounds in multiple places without single constants:** Causes UI/backend drift; if local constants are needed, keep one local source and tie values to contract docs.
- **Introducing new global state/error buses:** Unnecessary for phase scope and likely to delay delivery.

## Don't Hand-Roll

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| REST transport flow | New custom API client layer for this phase | Existing `getJsonRest` / `postJsonRest` + route-local handlers | Existing project pattern already matches required GET/POST loop. |
| UI structure | New card/form framework | Existing `SettingsCard`, `GridForm`, `Slider`, `Button` | Preserves consistency and reduces regression risk. |
| Persistence mechanism | Additional frontend persistence cache for orientation | Existing `/rest/heliostat` + firmware fs persistence | Contract already persists in firmware and reload path must verify that behavior. |
| Orientation parsing/validation semantics | Alternate field names or unit model | Existing `mountOrientation.tiltDeg` + `tiltAzimuthDeg` semantics | Field names and bounds are already established and tested in prior phases. |

**Key insight:** This phase is integration UX, not infrastructure expansion. Reusing existing route, components, and REST contract is both lower risk and aligned with locked decisions.

## Common Pitfalls

### Pitfall 1: UI type model omits `mountOrientation`
**What goes wrong:** Orientation controls cannot hydrate correctly or require unsafe casts.
**Why it happens:** `HeliostatControllerState` in `Heliostat.svelte` currently does not include `mountOrientation`.
**How to avoid:** Extend route-local type (or shared model type) first, then wire controls.
**Warning signs:** `undefined` reads, missing slider defaults after initial fetch.

### Pitfall 2: Apply action does not perform immediate re-fetch
**What goes wrong:** Operator cannot reliably observe confirmed state after applying changes.
**Why it happens:** POST result is assumed sufficient without a canonical GET round-trip.
**How to avoid:** Always call `getHeliostatControllerState()` after successful POST.
**Warning signs:** UI values differ from backend after apply or after transient request failure.

### Pitfall 3: Bounds mismatch between UI validation and backend contract
**What goes wrong:** User sees accepted values that backend clamps/rejects differently.
**Why it happens:** Hardcoded UI ranges drift from `orientation_contract.h` semantics.
**How to avoid:** Mirror contract ranges exactly: `tiltDeg [-90, 90]`, `tiltAzimuthDeg [0, 360]`.
**Warning signs:** Surprise value changes after apply/read-back.

### Pitfall 4: Observation step not tied to meaningful telemetry
**What goes wrong:** User applies values but cannot verify effect.
**Why it happens:** Loop ends at apply confirmation only.
**How to avoid:** Surface existing `sunTracker` azimuth/elevation/time status in the same context as setup completion.
**Warning signs:** Setup done status with no clear post-apply behavioral signal.

## Code Examples

Verified patterns from repository sources:

### Existing REST read/write loop on heliostat route
```typescript
// Source: interface/src/routes/heliostat/Heliostat.svelte
async function getHeliostatControllerState() {
  return getJsonRest(restPath, heliostatControllerState).then((data) => {
    heliostatControllerState = data;
  });
}

async function postHeliostatControllerState() {
  return postJsonRest(restPath, heliostatControllerState).then((data) => (heliostatControllerState = data));
}
```

### Backend contract for mount orientation serialization
```cpp
// Source: src/HeliostatService.cpp + src/orientation_contract.h
{"mountOrientation", [&](JsonVariant content, HeliostatController &controller) {
    return OrientationContract::applyMountOrientationPatch(content, controller.tiltDeg, controller.tiltAzimuthDeg);
}},

{"mountOrientation", [&](HeliostatController &controller, JsonVariant content) {
    JsonObject obj = content.to<JsonObject>();
    OrientationContract::writeMountOrientation(obj, controller.tiltDeg, controller.tiltAzimuthDeg);
}},
```

## State of the Art

| Old Approach | Current Approach | When Changed | Impact |
|--------------|------------------|--------------|--------|
| Level-install assumptions only (no mount orientation setup UX) | Orientation-aware firmware contract + control behavior available, UI setup pending in phase 3 | Phase 1-2 completed (2026-03-17 to 2026-03-18) | Phase 3 can focus on operator workflow instead of core math/API invention. |
| Implicit field editing patterns in mixed heliostat controls | Explicit setup loop required for orientation (`configure -> apply -> observe -> refresh verify`) | Locked in 03-CONTEXT (2026-03-18) | Reduces operator ambiguity and improves commissioning reliability. |

**Deprecated/outdated:**
- Assuming one scalar tilt is enough: replaced by two-parameter orientation model (`tiltDeg`, `tiltAzimuthDeg`).

## Open Questions

1. **Should apply be one shared section button or per-field apply?**
   - What we know: Both options are explicitly left to Claude's discretion.
   - What's unclear: Which choice best matches operator workflow in this deployment.
   - Recommendation: Use one shared Apply for atomic orientation intent and cleaner testability.

2. **Where should orientation type live long-term (route-local vs shared model)?**
   - What we know: Current route defines local `HeliostatControllerState` and currently lacks `mountOrientation`.
   - What's unclear: Whether this model is consumed elsewhere soon.
   - Recommendation: Start route-local for minimal scope; promote to shared model if reused by another route/component.

## Validation Architecture

### Test Framework
| Property | Value |
|----------|-------|
| Framework | Existing: `svelte-check` 4.2.2 + ESLint/Prettier (quality gates). Recommended Wave 0: `vitest` 4.1.0 + `@testing-library/svelte` 5.3.1 + `jsdom` 29.0.0 |
| Config file | none - see Wave 0 |
| Quick run command | `cd interface && npm run check` |
| Full suite command | `cd interface && npm run lint && npm run check && npm run build` |

### Phase Requirements -> Test Map
| Req ID | Behavior | Test Type | Automated Command | File Exists? |
|--------|----------|-----------|-------------------|-------------|
| UI-01 | Dedicated orientation section with inline validation and bounded inputs | integration (route logic) | `cd interface && npm run check && npm run lint` (baseline) + `cd interface && npx vitest run interface/src/routes/heliostat/heliostat-orientation.test.ts` (Wave 0) | ❌ Wave 0 |
| UI-02 | Apply then observe updated state and refresh persistence visibility | integration + manual hardware-coupled verification | `cd interface && npm run build` (baseline) + `cd interface && npx vitest run interface/src/routes/heliostat/heliostat-apply-loop.test.ts` (Wave 0 for logic) | ❌ Wave 0 |

### Sampling Rate
- **Per task commit:** `cd interface && npm run check`
- **Per wave merge:** `cd interface && npm run lint && npm run check && npm run build`
- **Phase gate:** full interface quality suite green, then operator UAT: apply/test/observe + browser refresh confirmation against device `/rest/heliostat`

### Wave 0 Gaps
- [ ] `interface/vitest.config.ts` - establish test runner configuration for Svelte route tests
- [ ] `interface/src/routes/heliostat/heliostat-orientation.test.ts` - UI-01 validation and render contract
- [ ] `interface/src/routes/heliostat/heliostat-apply-loop.test.ts` - UI-02 apply/read-back route logic
- [ ] `interface/src/test/setup.ts` - shared jsdom + fetch mocking utilities
- [ ] Framework install: `cd interface && npm install -D vitest @testing-library/svelte jsdom`

## Sources

### Primary (HIGH confidence)
- Repository source: `interface/src/routes/heliostat/Heliostat.svelte` - existing heliostat page layout, controls, and REST interaction patterns.
- Repository source: `interface/src/lib/stores/rest.ts` - REST helper behavior and error handling style.
- Repository source: `src/HeliostatService.cpp` - `/rest/heliostat` read/write contract including `mountOrientation` and observable `sunTracker` fields.
- Repository source: `src/orientation_contract.h` - authoritative orientation bounds and field names.
- Repository source: `.planning/phases/03-setup-ui-and-verification-loop/03-CONTEXT.md` - locked scope decisions and deferred items.
- npm registry metadata (queried 2026-03-18): latest versions/publish dates for `svelte`, `@sveltejs/kit`, `vite`, `daisyui`, `vitest`, `@testing-library/svelte`, `jsdom`.

### Secondary (MEDIUM confidence)
- Repository planning references: `.planning/codebase/ARCHITECTURE.md`, `.planning/codebase/CONVENTIONS.md`, `.planning/codebase/TESTING.md`.

### Tertiary (LOW confidence)
- None.

## Metadata

**Confidence breakdown:**
- Standard stack: HIGH - derived from current repository dependencies and verified npm latest versions.
- Architecture: HIGH - based on direct route/service source and phase context constraints.
- Pitfalls: HIGH - inferred from current type/flow gaps visible in route and service code.

**Research date:** 2026-03-18
**Valid until:** 2026-04-17
