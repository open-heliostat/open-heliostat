# Coding Conventions

**Analysis Date:** 2026-03-17

## Naming Patterns

**Files:**
- Firmware C++ uses mostly `PascalCase` for services and components: `src/HeliostatService.cpp`, `src/ESPNowService.cpp`, `src/StepperSettingsService.cpp`, with some lowercase utility headers such as `src/geometry.h`, `src/heliostat.h`, `src/pins.h`.
- Svelte route files follow SvelteKit conventions: `interface/src/routes/**/+page.svelte`, `interface/src/routes/**/+page.ts`, `interface/src/routes/+layout.svelte`.
- Frontend shared modules use `camelCase` filenames for stores/helpers (`interface/src/lib/stores/socket.ts`, `interface/src/lib/stores/rest.ts`) and `PascalCase` for Svelte components (`interface/src/lib/components/SettingsCard.svelte`, `interface/src/lib/components/ConfirmDialog.svelte`).
- Python build scripts are `snake_case.py` (`scripts/build_interface.py`, `scripts/prebuild_utils.py`, `scripts/generate_cert_bundle.py`).

**Functions:**
- C++ methods and free functions are `camelCase`: `runLoop()`, `getLocationFromGPS()`, `readDirectionsMap()`, `updateDirectionsMap()` in `src/heliostat.h` and `src/HeliostatService.cpp`.
- TS/Svelte functions are `camelCase`: `getWifiStatus()`, `postWiFiSettings()`, `handleEdit()` in `interface/src/routes/wifi/sta/Wifi.svelte`.
- Python functions are `snake_case`: `is_build_task()`, `build_webapp()`, `download_cacert_file()` in `scripts/prebuild_utils.py` and `scripts/build_interface.py`.

**Variables:**
- Private C++ members use leading underscore (for service internals): `_eventEndpoint`, `_fsPersistence`, `_router` in `src/HeliostatService.h` and `src/RemoteService.h`.
- Global/static firmware objects are lower camel or descriptive lowercase in `src/main.cpp`: `esp32sveltekit`, `heliostatController`, `controlTaskHandle`.
- TypeScript local variables use `camelCase`; Svelte v5 rune state is used (`$state`, `$derived`) in `interface/src/routes/wifi/sta/Wifi.svelte`.

**Types:**
- C++ classes/structs use `PascalCase`: `HeliostatController`, `RemoteService`, `SphericalCoordinate`.
- TS type aliases use `PascalCase` in `interface/src/lib/types/models.ts`: `WifiStatus`, `MQTTSettings`, `MultiStepperControl`.

## Code Style

**Formatting:**
- Frontend formatting is enforced with Prettier in `interface/.prettierrc`:
  - `useTabs: true`
  - `singleQuote: true`
  - `trailingComma: none`
  - `printWidth: 100`
  - Svelte parser override (`*.svelte`)
- No root `.editorconfig` was detected.
- Firmware C++ style is not enforced by clang-format config in repository; style is convention-based with brace-heavy Arduino/C++ patterns and explicit spacing in files like `src/main.cpp` and `src/HeliostatService.cpp`.

**Linting:**
- ESLint is configured for the interface in `interface/.eslintrc.cjs`:
  - parser: `@typescript-eslint/parser`
  - extends: `eslint:recommended`, `plugin:@typescript-eslint/recommended`, `prettier`
  - Svelte processing via `eslint-plugin-svelte` / `svelte3` override
- Interface lint command in `interface/package.json`: `npm run lint` runs both Prettier check and ESLint.

## Import Organization

**Order:**
1. External packages (`svelte`, `@sveltejs/*`, `msgpack-lite`, icon packages)
2. Internal aliased imports (`$lib/*`, `$app/*`)
3. Relative imports (`./Scan.svelte`, `../app.css`)

Observed in `interface/src/routes/+layout.svelte` and `interface/src/routes/wifi/sta/Wifi.svelte`.

**Path Aliases:**
- `$lib/*` and other SvelteKit aliases are used throughout route and component code (`interface/src/routes/+layout.svelte`).
- Custom alias `$src` is configured in `interface/svelte.config.js`.

## Error Handling

**Patterns:**
- Frontend REST calls typically use `try/catch` and log failures via `console.error`, then return existing/default state (`interface/src/lib/stores/rest.ts`, `interface/src/routes/wifi/sta/Wifi.svelte`).
- Frontend response validation commonly branches on `response.status == 200` before applying updates (`interface/src/lib/stores/rest.ts`, `interface/src/routes/wifi/sta/Wifi.svelte`).
- Firmware JSON update handlers rely on `JsonVariant` type guards and boolean success returns:
  - `if (value.is<JsonObject>()) { ...; return true; } return false;` in `src/ESPNowService.cpp`.
- Stateful firmware updates classify mutations via `StateUpdateResult::CHANGED` vs `UNCHANGED` in router update functions (`src/HeliostatService.h`, `src/RemoteService.h`).

## Logging

**Framework:** `Serial` and ESP logging ecosystem (build-flag controlled).

**Patterns:**
- Diagnostic `Serial.printf(...)`/`Serial.println(...)` in runtime loop and profiling sections (`src/main.cpp`).
- Build documentation and config encourage toggling with build flags in `platformio.ini`:
  - `-D SERIAL_INFO`
  - `-D CORE_DEBUG_LEVEL=4`
- Python scripts emit status with `print(...)` and helper wrappers (`status()`, `critical()`) in `scripts/generate_cert_bundle.py`.

## Comments

**When to Comment:**
- Comments are used to explain runtime constraints, tuning choices, and build-time options:
  - task frequency/latency thresholds in `src/main.cpp`
  - platform/build options in `platformio.ini`
  - script behavior in `scripts/build_interface.py`
- Many optional code paths are retained as commented toggles for experimentation (`src/main.cpp`, `src/StepperSettingsService.cpp`).

**JSDoc/TSDoc:**
- Not a dominant pattern in TS/Svelte source; comments are inline/block prose rather than JSDoc tags.
- Python scripts include module/function docstrings in `scripts/prebuild_utils.py`.

## Function Design

**Size:**
- Firmware service/router functions are medium to large and often combine parsing and orchestration (`src/HeliostatService.cpp`, `src/ESPNowService.cpp`).
- Frontend route components keep networking, validation, and UI handlers in one Svelte file per page (`interface/src/routes/wifi/sta/Wifi.svelte`).

**Parameters:**
- Firmware uses references/pointers for stateful service integration (`HeliostatController&`, `PsychicHttpServer*`) in `src/HeliostatService.h`.
- Frontend favors typed payloads in handlers and stores (`WifiReconnectEvent`, `Analytics`) in `interface/src/routes/wifi/sta/Wifi.svelte` and `interface/src/routes/+layout.svelte`.

**Return Values:**
- Router parse/update flows use booleans or `StateUpdateResult` to signal mutation success in C++ (`src/HeliostatService.h`, `src/RemoteService.h`).
- Async frontend helpers return updated data object even on failure fallback (`interface/src/lib/stores/rest.ts`).

## Module Design

**Exports:**
- Frontend TS modules typically use named exports (`export const socket`, `export async function getJsonRest`) in `interface/src/lib/stores/socket.ts` and `interface/src/lib/stores/rest.ts`.
- Model definitions are centralized as exported type aliases in `interface/src/lib/types/models.ts`.

**Barrel Files:**
- No barrel-index aggregation pattern was detected (`index.ts` export hubs are not present in interface `lib` folders).

## Prescriptive Guidance for New Code

- Match frontend formatting by running from `interface/`: `npm run format` and `npm run lint`.
- Keep new service/state classes in firmware with existing naming and update contracts used in `src/*Service.h` and `src/*Service.cpp`.
- For REST/event state updates in firmware, continue using `JsonRouter` + `StateUpdateResult` patterns as in `src/HeliostatService.h`.
- For Svelte routes, place route-specific fetch/validation handlers inside the route component and use shared typed models from `interface/src/lib/types/models.ts`.
- For build scripts, follow `snake_case` naming and guard non-build tasks via `is_build_task(...)` from `scripts/prebuild_utils.py`.

---

*Convention analysis: 2026-03-17*
