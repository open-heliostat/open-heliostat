# Architecture

**Analysis Date:** 2026-03-17

## Pattern Overview

**Overall:** Modular service-oriented firmware with a co-located static SPA frontend and build-time asset packaging.

**Key Characteristics:**
- Composition-root initialization in `src/main.cpp` wires hardware controllers, framework services, and domain services.
- Shared state services expose synchronized transport channels: REST (`/rest/*`), event socket (`/ws/events`), and persistence (`/config/*.json` in LittleFS).
- Frontend is a static SvelteKit app in `interface/src/routes` and `interface/src/lib` that consumes REST plus websocket events.

## Layers

**Firmware Composition Layer:**
- Purpose: Create and connect all runtime objects, configure server/tasking, and invoke lifecycle hooks.
- Location: `src/main.cpp`
- Contains: Global service/controller instances, `setup()`, `loop()`, FreeRTOS control task.
- Depends on: `lib/framework/ESP32SvelteKit.h`, `lib/PsychicHttp`, and project services under `src/*Service.*`.
- Used by: Arduino runtime entrypoint.

**Framework Platform Layer:**
- Purpose: Provide infrastructure primitives (auth/security, WiFi/AP, MQTT/NTP, system services, event socket, static file serving).
- Location: `lib/framework/`
- Contains: `ESP32SvelteKit`, `StatefulService`, `EventEndpoint`, `HttpEndpoint`, `FSPersistence`, feature-gated services.
- Depends on: Arduino/ESP32 SDK, `lib/PsychicHttp`, and optional external libs.
- Used by: Domain services in `src/` and composition layer in `src/main.cpp`.

**Domain Service Layer:**
- Purpose: Encapsulate heliostat features (tracking, sequencing, motor control, accelerometer calibration, ArtNet, remote, ESP-NOW, GPS).
- Location: `src/*Service.h`, `src/*Service.cpp`
- Contains: Stateful wrappers with JSON routers and endpoint bindings, e.g. `src/HeliostatService.h`, `src/MovementSequencerService.h`, `src/TargetSequencerService.h`, `src/ESPNowService.h`, `src/GPSService.h`.
- Depends on: Framework endpoint/persistence abstractions and domain controllers/models.
- Used by: `src/main.cpp` lifecycle orchestration and frontend clients over `/rest/*` + `/ws/events`.

**Domain Model/Controller Layer:**
- Purpose: Hold control algorithms and hardware-bound behavior.
- Location: `src/heliostat.h`, `src/servocontroller.h`, `src/movementsequencer.h`, `src/TargetSequencer.h`, `src/SourceTargetSequencer.h`, `src/geometry.*`, `src/sun.*`.
- Contains: Controller structs/classes and logic consumed by service wrappers.
- Depends on: Low-level drivers and math/time utilities.
- Used by: Domain service layer and control task in `src/main.cpp`.

**Hardware Driver Layer:**
- Purpose: Interface with sensors, motor drivers, GPS, and communication peripherals.
- Location: `src/adxl345.*`, `src/encoder.h`, `src/tmcdriver.h`, `src/espnow.*`, `src/gpsneo.h`, `src/pins.h`.
- Contains: Driver abstractions, protocol setup/update calls.
- Depends on: Arduino/ESP-IDF APIs and vendor libs.
- Used by: Domain model/controller and service layers.

**Frontend UI Layer:**
- Purpose: Present configuration/status UI and invoke backend APIs.
- Location: `interface/src/routes/`, `interface/src/lib/components/`, `interface/src/lib/stores/`
- Contains: Route pages, reusable components, websocket/REST stores.
- Depends on: SvelteKit runtime and backend API contracts.
- Used by: Browser clients; output is packaged into firmware via build scripts.

**Build/Packaging Layer:**
- Purpose: Build frontend, convert assets for flash/PROGMEM, merge binaries, and emit release artifacts.
- Location: `scripts/build_interface.py`, `scripts/generate_cert_bundle.py`, `scripts/merge_bin.py`, `scripts/rename_fw.py`, `scripts/save_elf.py`, `platformio.ini`.
- Contains: PlatformIO hooks and artifact pipeline logic.
- Depends on: PlatformIO/SCons, npm tooling in `interface/package.json`.
- Used by: Build commands (`pio run`, upload workflows).

## Data Flow

**Configuration + Command Flow (UI -> Firmware -> Persistence):**

1. UI route/components call `fetch('/rest/...')` (examples in `interface/src/routes/system/status/SystemStatus.svelte`, `interface/src/routes/controllers/+page.svelte`, `interface/src/routes/heliostat/Heliostat.svelte`).
2. HTTP is handled by framework endpoints (`lib/framework/HttpEndpoint.h`) or project router endpoint (`src/lib/HttpStateRouterEndpoint.h`).
3. JSON routers parse into service state (`src/HeliostatService.h`, `src/MovementSequencerService.h`, `src/TargetSequencerService.h`, `src/ESPNowService.h`).
4. `StatefulService` update handlers fan out state changes; `FSPersistence` writes selected fields to `/config/*.json` in LittleFS.

**Real-time State/Event Flow (Firmware -> UI):**

1. Services publish via `EventEndpoint`/`EventSocket` (`lib/framework/EventEndpoint.h`, `lib/framework/EventSocket.cpp`).
2. Browser initializes websocket in `interface/src/routes/+layout.svelte` using `/ws/events`.
3. `interface/src/lib/stores/socket.ts` decodes MessagePack/JSON payloads and dispatches by event name.
4. Telemetry/feature stores update UI state (`interface/src/lib/stores/telemetry.ts`, `interface/src/lib/stores/analytics.ts`, `interface/src/lib/stores/battery.ts`).

**Control Loop Flow (Deterministic Motor Runtime):**

1. `src/main.cpp` creates a dedicated FreeRTOS `controlTask` pinned to core 1.
2. `controlTask` invokes `heliostatController.runLoop()` at 20 Hz with latency warnings.
3. Main Arduino `loop()` executes service `loop()` handlers (sequencers, heliostat, accel calibration, ArtNet, remote, ESP-NOW).
4. GPS state updates run in its own task path (`src/GPSService.cpp`) to avoid blocking control timing.

**Frontend Delivery Flow (Build -> Device -> Browser):**

1. PlatformIO pre-script `scripts/build_interface.py` runs package manager install/build for `interface/`.
2. Assets are embedded into `lib/framework/WWWData.h` when `-D EMBED_WWW`, otherwise copied+gzipped under `data/www`.
3. `ESP32SvelteKit::begin()` serves static assets from PROGMEM (`WWWData::registerRoutes`) or LittleFS fallback (`lib/framework/ESP32SvelteKit.cpp`).
4. Browser loads SPA and uses `/rest/features` + route-level API fetches to hydrate views.

**State Management:**
- Firmware uses `StatefulService<T>` with explicit read/update callbacks and update handler registration (`lib/framework/StatefulService.h`).
- Frontend uses Svelte stores (`interface/src/lib/stores/*`) and route loaders (`interface/src/routes/+layout.ts`) for global feature flags.

## Key Abstractions

**Stateful Service + Endpoint Trio:**
- Purpose: Standardize synchronized state over HTTP/event socket/filesystem.
- Examples: `lib/framework/StatefulService.h`, `lib/framework/EventEndpoint.h`, `src/lib/HttpStateRouterEndpoint.h`, `lib/framework/FSPersistence.h`.
- Pattern: Define `read/update` + optional `readForSave`; call `begin()` to register endpoints and load persisted state.

**JSON Router Abstraction:**
- Purpose: Declarative state parse/serialize and selective save maps.
- Examples: `src/HeliostatService.h` (`HeliostatControllerJsonRouter`), `src/MovementSequencerService.h`, `src/TargetSequencerService.h`, `src/ESPNowService.h`, `src/lib/JsonStateRouter.h`.
- Pattern: Router owns field-level mapping and save filtering; service remains transport/persistence orchestrator.

**Framework Kernel (`ESP32SvelteKit`):**
- Purpose: Aggregate core framework services and boot sequence.
- Examples: `lib/framework/ESP32SvelteKit.h`, `lib/framework/ESP32SvelteKit.cpp`.
- Pattern: Constructor composes feature services; `begin()` initializes storage/network/server/endpoints and static UI hosting.

## Entry Points

**Firmware Runtime Entry:**
- Location: `src/main.cpp`
- Triggers: Device boot under Arduino runtime.
- Responsibilities: Configure HTTP server parameters, call `esp32sveltekit.begin()`, start all project services, create `controlTask`, run periodic service loops.

**Framework Boot Entry:**
- Location: `lib/framework/ESP32SvelteKit.cpp` (`ESP32SvelteKit::begin()`)
- Triggers: Called from `src/main.cpp` setup.
- Responsibilities: Mount FS, initialize WiFi/Ethernet, register static routes, start MDNS and framework services, spawn internal framework loop task.

**Web UI Entry:**
- Location: `interface/src/routes/+layout.svelte`, `interface/src/routes/+page.svelte`, `interface/src/app.html`
- Triggers: Browser request to `/` served by device HTTP server.
- Responsibilities: Load feature flags (`+layout.ts`), initialize websocket/auth context, render route tree and page components.

**Build Entry:**
- Location: `platformio.ini` (extra scripts) and `interface/package.json` scripts.
- Triggers: `pio run`, `pio run -t upload`, `npm run build` within interface pipeline.
- Responsibilities: Build and package frontend, cert bundle generation, merged binary generation, firmware renaming and artifact capture.

## Error Handling

**Strategy:** Layer-local handling with fail-soft logging and status propagation.

**Patterns:**
- Frontend REST wrappers use `try/catch` and console logging (`interface/src/lib/stores/rest.ts`).
- Websocket layer reconnects and emits `close/error/unresponsive` events (`interface/src/lib/stores/socket.ts`).
- Firmware logs timing/perf warnings in loops (`src/main.cpp`) and validates JSON/persistence through router update results.

## Cross-Cutting Concerns

**Logging:** Serial and ESP log macros across firmware (`src/main.cpp`, `lib/framework/*`).
**Validation:** Router-based parse checks and save-map filtering (`src/lib/JsonStateRouter.h`, service-specific router classes in `src/*Service.h`).
**Authentication:** Security manager and auth predicates in framework (`lib/framework/SecuritySettingsService.h`, `lib/framework/AuthenticationService.h`, websocket auth in `lib/framework/EventSocket.h`).

## Architecture Commands

Use these commands when navigating or extending architecture:

```bash
pio run -e esp32-s3-devkitc-1          # build firmware + run pre/post scripts
pio run -t upload                      # upload firmware
pio run -t uploadfs                    # upload LittleFS image when not embedding WWW
cd interface && npm run dev            # run UI in local dev mode
cd interface && npm run build          # build static UI assets
rg -n "StatefulService|HttpStateRouterEndpoint|EventEndpoint" src lib/framework src/lib
```

---

*Architecture analysis: 2026-03-17*
