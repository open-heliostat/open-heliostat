# Phase 1: Orientation Configuration Contract - Research

**Researched:** 2026-03-17
**Domain:** ESP32 firmware configuration contract (REST/event/persistence) for mount orientation
**Confidence:** HIGH

<user_constraints>
## User Constraints (from CONTEXT.md)

### Locked Decisions
### Orientation schema shape
- Add a dedicated `mountOrientation` object in heliostat state (do not nest in `sunTracker`).
- Use explicit degree-based fields: `tiltDeg` and `tiltAzimuthDeg`.
- Do not add a separate `enabled` flag in v1; behavior is driven directly by numeric values.
- Define `tiltAzimuthDeg` as the compass azimuth of downslope direction.

### Validation behavior
- Validation handling for out-of-range values: clamp to nearest valid value.
- Accepted ranges: `tiltDeg` in `[-90, 90]`, `tiltAzimuthDeg` in `[0, 360]`.
- Public API and UI use degrees only.
- Error payload style: single generic error string when validation fails.

### Persistence and migration policy
- If orientation fields are missing in legacy config, auto-fill level-install defaults at runtime.
- Persist orientation immediately after a valid change.
- For combined updates: if one orientation field is invalid, reject the whole orientation update.
- Legacy clients that do not send orientation fields continue working unchanged with level behavior.

### API update granularity
- v1 write path: single object write via main heliostat payload (`/rest/heliostat`).
- v1 read path: include orientation data in main `/rest/heliostat` response.
- Support patch semantics for `mountOrientation` (missing fields keep prior values).
- Reuse existing `heliostat-service` event stream for update propagation.

### Claude's Discretion
- Exact naming of internal helper functions for parse/serialize defaults and clamping utilities.
- Exact wording/text of generic validation error messages.

### Deferred Ideas (OUT OF SCOPE)
- Guided multi-step setup wizard (tracked in v2 requirements).
- Live residual pointing error metric and profile management (tracked in v2 requirements).
</user_constraints>

<phase_requirements>
## Phase Requirements

| ID | Description | Research Support |
|----|-------------|-----------------|
| ORNT-01 | User can set mount orientation with two parameters: tilt magnitude and tilt direction. | Additive `mountOrientation` object in `HeliostatController` + JsonRouter parse/serialize on `/rest/heliostat` with degree fields `tiltDeg` and `tiltAzimuthDeg`. |
| ORNT-02 | System validates orientation values with explicit units, valid bounds, and clear error responses. | Router-level typed parsing, bounds clamp policy, and `StateUpdateResult::ERROR` path in HTTP endpoint for invalid combined updates. |
| CONF-01 | Orientation configuration persists across reboot in heliostat configuration storage. | Extend save-map and existing `FSPersistence` flow at `/config/heliostat.json`; apply runtime defaults when missing on read. |
| API-01 | REST API supports read/write of orientation fields using existing heliostat service patterns. | Reuse existing `HttpStateRouterEndpoint` + `EventEndpoint` + `JsonRouter` pattern in `HeliostatService`. |
</phase_requirements>

## Summary

Phase 1 should be implemented as a schema extension of the existing heliostat state contract, not as a new service. The current architecture already has the exact primitives needed: field-level JSON routing (`JsonRouter`), update classification (`StateUpdateResult`), synchronized REST/event propagation, and filtered filesystem persistence (`JsonSaveManager` + `FSPersistence`).

The safest and fastest path is to introduce `mountOrientation` directly in `HeliostatController`, wire parse/serialize behavior in `HeliostatControllerJsonRouter`, and include it in save filtering. Keep behavior additive and backward-compatible: legacy configs without orientation keys should continue operating as level mounts by applying defaults at load/read time.

**Primary recommendation:** Implement orientation as an additive `mountOrientation` contract inside the existing `/rest/heliostat` router/persistence pipeline; do not create a parallel endpoint or storage path.

## Standard Stack

### Core
| Library | Version | Purpose | Why Standard |
|---------|---------|---------|--------------|
| Arduino framework (PlatformIO) | `framework = arduino` | Firmware runtime on ESP32 | Existing project runtime contract and services are built on this stack. |
| espressif32 platform | `6.12.0` | Build/SDK toolchain | Already pinned in `platformio.ini`, minimizing migration risk. |
| ArduinoJson | `>=7.0.0` | JSON parsing/serialization for routers and persistence | Current codebase already uses `JsonDocument`/`JsonVariant` idioms aligned with ArduinoJson v7 docs. |
| ESP32-sveltekit framework primitives | repo-local | Stateful REST/event/persistence infrastructure | Existing `StatefulService`, `HttpStateRouterEndpoint`, and `FSPersistence` exactly match phase scope. |

### Supporting
| Library | Version | Purpose | When to Use |
|---------|---------|---------|-------------|
| PsychicHttp / Event socket stack | repo-local | `/rest/heliostat` and `heliostat-service` transport | Use for all API updates/events; no new transport surface needed. |
| LittleFS via `FSPersistence` | repo-local | Persist `/config/heliostat.json` | Use for orientation durability and migration defaults. |

### Alternatives Considered
| Instead of | Could Use | Tradeoff |
|------------|-----------|----------|
| Existing `/rest/heliostat` contract | New `/rest/orientation` endpoint | Adds parallel state paths and migration complexity for no functional gain in v1. |
| Save-map filtered persistence | Manual file IO branch | Higher regression risk, bypasses existing schema filtering and update semantics. |
| Router-level field parsing | Ad-hoc JSON processing in handlers | Breaks project convention and makes patch semantics harder to enforce. |

**Installation:**
```bash
# Existing stack already present; no new package install required for Phase 1.
pio run -e esp32-s3-devkitc-1
```

**Version verification:**
- Verified from repository configuration on 2026-03-17:
  - `espressif32 @ 6.12.0` in `platformio.ini`
  - `ArduinoJson@>=7.0.0` in `platformio.ini`
- External verification references:
  - PlatformIO `lib_deps` package-spec behavior (official docs)
  - ArduinoJson v7 API/upgrade docs confirming current API semantics

## Architecture Patterns

### Recommended Project Structure
```text
src/
├── heliostat.h             # Add mountOrientation state fields and defaults
├── HeliostatService.h      # Extend save-map and update classification contract
└── HeliostatService.cpp    # Add parse/serialize + validation/clamp behavior
```

### Pattern 1: Router-Centric Stateful Contract
**What:** Keep request parsing, patch semantics, and serialization in `HeliostatControllerJsonRouter` field handlers.
**When to use:** Any additive field in `/rest/heliostat` state.
**Example:**
```cpp
// Source: src/HeliostatService.cpp + src/lib/JsonStateRouter.h
{"mountOrientation", [&](JsonVariant content, HeliostatController &controller) {
    if (!content.is<JsonObject>()) return false;
    JsonObject obj = content.as<JsonObject>();
    // parse only present keys to preserve patch semantics
    if (obj["tiltDeg"].is<double>()) {
        controller.tiltDeg = clampTiltDeg(obj["tiltDeg"].as<double>());
    }
    if (obj["tiltAzimuthDeg"].is<double>()) {
        controller.tiltAzimuthDeg = clampAzimuthDeg(obj["tiltAzimuthDeg"].as<double>());
    }
    return true;
}}
```

### Pattern 2: Save-Map Filtered Persistence
**What:** Add `mountOrientation` keys to save-map and let `JsonSaveManager`/`FSPersistence` manage persistence.
**When to use:** Any field that must persist in `/config/heliostat.json`.
**Example:**
```cpp
// Source: src/HeliostatService.h
root["mountOrientation"]["tiltDeg"] = true;
root["mountOrientation"]["tiltAzimuthDeg"] = true;
```

### Pattern 3: Backward-Compatible Runtime Defaults
**What:** If legacy config omits orientation keys, apply level defaults in state before serialize/read responses.
**When to use:** During FS load and normal read paths to avoid migration scripts.
**Example:**
```cpp
// Source pattern: lib/framework/FSPersistence.h applyDefaults fallback
if (!obj["tiltDeg"].is<double>()) controller.tiltDeg = 0.0;
if (!obj["tiltAzimuthDeg"].is<double>()) controller.tiltAzimuthDeg = 0.0;
```

### Anti-Patterns to Avoid
- **New orientation micro-service:** duplicates endpoint/event/persistence plumbing already standardized.
- **Silent mixed-validity partial commit:** if one orientation field is invalid in a combined write, accepting the other violates locked decision.
- **Storing orientation under `sunTracker`:** directly conflicts with locked schema boundary.

## Don't Hand-Roll

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| REST routing + auth path handling | Custom raw HTTP parser | `HttpStateRouterEndpoint` | Already handles nested subpaths, typed JSON update flow, and `StateUpdateResult` error mapping. |
| Event fanout synchronization | Custom websocket payload plumbing | `EventEndpoint` + existing `heliostat-service` stream | Maintains consistency with existing subscribers and origin-id propagation. |
| Config file filtering/dirty-detect | Custom recursive JSON diff/filter | `JsonSaveManager::needsToSave` and `filterFieldsRecursively` | Existing logic already battle-tested for selective persistence. |
| FS default initialization | Manual boot-time migration script | `FSPersistence::readFromFS()` default-then-write behavior | Handles missing/invalid file and persists defaults predictably. |

**Key insight:** This codebase already has a complete stateful contract framework; Phase 1 should only extend mappings and validation rules within that framework.

## Common Pitfalls

### Pitfall 1: Wrong map target (`sourcesMap` vs `targetsMap`)
**What goes wrong:** In existing code, `sourcesMap` routing currently updates `targetsMap`; copying this pattern blindly can propagate subtle contract bugs.
**Why it happens:** Legacy naming drift in router handlers.
**How to avoid:** Add orientation handler independently and verify route-to-state mapping by explicit tests/manual payload checks.
**Warning signs:** POST to one field changes unrelated serialized response fields.

### Pitfall 2: Losing patch semantics
**What goes wrong:** Requiring both orientation fields on every write breaks partial update behavior.
**Why it happens:** Treating object writes as replace, not patch.
**How to avoid:** Parse only present keys and keep previous values for absent keys.
**Warning signs:** Sending only `tiltDeg` resets `tiltAzimuthDeg` unexpectedly.

### Pitfall 3: Validation policy drift (clamp vs reject)
**What goes wrong:** Implementing hard rejection for out-of-range values where clamping was chosen.
**Why it happens:** Mixing generic validation patterns from other services.
**How to avoid:** Centralize clamp helpers and use them consistently in parser path.
**Warning signs:** API behavior changes across fields or callers.

### Pitfall 4: Breaking backward compatibility on legacy config
**What goes wrong:** Missing orientation keys produce undefined values or errors after reboot.
**Why it happens:** Assuming persisted schema is always latest.
**How to avoid:** Apply defaults when keys are absent and include defaults in serialized output.
**Warning signs:** Existing devices without orientation config fail to deserialize or expose null fields.

## Code Examples

Verified patterns from project code and official docs:

### Extend save-map for persisted keys
```cpp
// Source: src/HeliostatService.h (existing pattern)
static const void getSaveMap(JsonObject &root, HeliostatController &state)
{
    // existing keys...
    root["mountOrientation"]["tiltDeg"] = true;
    root["mountOrientation"]["tiltAzimuthDeg"] = true;
}
```

### Return HTTP 400 on router-level error
```cpp
// Source: src/lib/HttpStateRouterEndpoint.h
StateUpdateResult outcome = _statefulService->updateWithoutPropagation(payload, _stateUpdater, request->path());

if (outcome == StateUpdateResult::ERROR)
{
    return request->reply(400);
}
```

### ArduinoJson v7 typed checks for optional fields
```cpp
// Source: ArduinoJson v7 API docs + existing code style in src/HeliostatService.cpp
if (obj["tiltDeg"].is<double>()) {
    controller.tiltDeg = obj["tiltDeg"].as<double>();
}
```

## State of the Art

| Old Approach | Current Approach | When Changed | Impact |
|--------------|------------------|--------------|--------|
| Manual `StaticJsonDocument` sizing and capacity tuning | ArduinoJson v7 `JsonDocument` with elastic capacity | 2024 (ArduinoJson 7) | Simplifies router code, but requires memory awareness on embedded targets. |
| Standalone endpoint-specific state code | Unified stateful service pattern (`JsonRouter` + REST/event/FS) | Matured in ESP32-sveltekit template lineage | Faster additive feature delivery and consistent behavior across services. |

**Deprecated/outdated:**
- `containsKey()`-centric validation style in ArduinoJson: replaced by `obj["key"].is<T>()` style for typed existence checks.

## Open Questions

1. **Generic error payload shape for validation failures**
   - What we know: Decision requires a single generic error string on validation failure.
   - What's unclear: Whether this should be returned as plain HTTP 400 body or JSON object with `error` key (existing services vary).
   - Recommendation: Standardize on JSON `{ "error": "..." }` if current endpoint wrapper supports it; otherwise document plain 400 text and keep stable.

2. **Normalization behavior for boundary azimuth values**
   - What we know: Accepted range is `[0, 360]` and out-of-range values should clamp.
   - What's unclear: Whether exactly `360` should be preserved or normalized to `0` for downstream math consistency.
   - Recommendation: Keep storage exactly as input-clamped (allow `360`) in Phase 1 contract; normalize only in Phase 2 math layer if needed.

## Validation Architecture

### Test Framework
| Property | Value |
|----------|-------|
| Framework | None currently configured for firmware unit/integration tests |
| Config file | none - see Wave 0 |
| Quick run command | `pio run -e esp32-s3-devkitc-1` |
| Full suite command | `pio run -e esp32-s3-devkitc-1` |

### Phase Requirements -> Test Map
| Req ID | Behavior | Test Type | Automated Command | File Exists? |
|--------|----------|-----------|-------------------|-------------|
| ORNT-01 | Read/write `mountOrientation.tiltDeg` + `mountOrientation.tiltAzimuthDeg` via `/rest/heliostat` with patch semantics | manual-only (API smoke) | `pio run -e esp32-s3-devkitc-1` | ❌ Wave 0 |
| ORNT-02 | Bounds and validation behavior (clamp, generic error on invalid combined update) | manual-only (contract checks) | `pio run -e esp32-s3-devkitc-1` | ❌ Wave 0 |
| CONF-01 | Persist orientation to `/config/heliostat.json` and reload across reboot | manual-only (device reboot verification) | `pio run -e esp32-s3-devkitc-1` | ❌ Wave 0 |
| API-01 | `/rest/heliostat` response includes orientation and update propagates via `heliostat-service` event stream | manual-only (REST + websocket check) | `pio run -e esp32-s3-devkitc-1` | ❌ Wave 0 |

### Sampling Rate
- **Per task commit:** `pio run -e esp32-s3-devkitc-1`
- **Per wave merge:** `pio run -e esp32-s3-devkitc-1`
- **Phase gate:** Firmware build green plus scripted/manual API contract checklist before `/gsd-verify-work`

### Wave 0 Gaps
- [ ] `test/test_orientation_contract.cpp` - ORNT-01/ORNT-02 parse, clamp, patch semantics via router-level tests
- [ ] `test/test_orientation_persistence.cpp` - CONF-01 save-map and reload/default migration checks
- [ ] `test/test_orientation_api_contract.cpp` - API-01 request/response shape checks (mocked endpoint or host-side integration)
- [ ] `platformio.ini` test env section - define `pio test` target for host/native or embedded test harness

## Sources

### Primary (HIGH confidence)
- Repository code:
  - `src/HeliostatService.h`
  - `src/HeliostatService.cpp`
  - `src/heliostat.h`
  - `src/lib/JsonStateRouter.h`
  - `src/lib/HttpStateRouterEndpoint.h`
  - `lib/framework/FSPersistence.h`
  - `lib/framework/StatefulService.h`
  - `platformio.ini`
- Planning constraints and requirements:
  - `.planning/phases/01-orientation-configuration-contract/01-CONTEXT.md`
  - `.planning/REQUIREMENTS.md`
  - `.planning/ROADMAP.md`
  - `.planning/config.json`

### Secondary (MEDIUM confidence)
- PlatformIO docs (`lib_deps` semantics): https://docs.platformio.org/en/latest/projectconf/sections/env/options/library/lib_deps.html
- ArduinoJson v7 docs (API and migration):
  - https://arduinojson.org/v7/api/jsondocument/
  - https://arduinojson.org/v7/api/jsonvariant/
  - https://arduinojson.org/v7/how-to/upgrade-from-v6/

### Tertiary (LOW confidence)
- ESP32-sveltekit upstream README overview: https://github.com/theelims/ESP32-sveltekit

## Metadata

**Confidence breakdown:**
- Standard stack: HIGH - directly grounded in repository pins and existing implementation.
- Architecture: HIGH - based on existing `HeliostatService` and framework abstractions already in production paths.
- Pitfalls: MEDIUM - strongly suggested by current code patterns, but full impact still needs execution-time validation.

**Research date:** 2026-03-17
**Valid until:** 2026-04-16
