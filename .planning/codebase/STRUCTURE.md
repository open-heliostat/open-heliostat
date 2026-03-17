# Codebase Structure

**Analysis Date:** 2026-03-17

## Directory Layout

```text
open-heliostat/
├── src/                    # Firmware application code (domain services, controllers, drivers, main entry)
├── lib/framework/          # Reusable ESP32-SvelteKit framework services and transport primitives
├── lib/PsychicHttp/        # HTTP/WebSocket server library vendored in repo
├── src/lib/                # Project-specific HTTP/JSON router helper headers
├── interface/              # SvelteKit frontend project (routes, stores, components, static assets)
├── scripts/                # PlatformIO build hooks and artifact pipeline scripts
├── docs/                   # MkDocs documentation source
├── tests/                  # Test root (currently no committed test files)
├── build/                  # Build outputs (elf, merged bins, release files)
├── ssl_certs/              # Input certificates for generated cert bundle
├── platformio.ini          # Firmware build matrix, scripts, feature flags, dependencies
└── mkdocs.yml              # Documentation site config
```

## Directory Purposes

**`src/`:**
- Purpose: Main firmware module for this project.
- Contains: `src/main.cpp`, `*Service.h/.cpp` state services, domain controllers (`src/heliostat.h`, `src/servocontroller.h`), hardware adapters (`src/adxl345.*`, `src/espnow.*`, `src/gpsneo.h`), constants (`src/pins.h`).
- Key files: `src/main.cpp`, `src/HeliostatService.h`, `src/MovementSequencerService.h`, `src/TargetSequencerService.h`, `src/GPSService.cpp`.

**`lib/framework/`:**
- Purpose: Shared backend framework implementation.
- Contains: Core orchestrator (`ESP32SvelteKit`), generic endpoint/persistence/services, security/network/system services.
- Key files: `lib/framework/ESP32SvelteKit.h`, `lib/framework/ESP32SvelteKit.cpp`, `lib/framework/StatefulService.h`, `lib/framework/EventEndpoint.h`, `lib/framework/FSPersistence.h`.

**`src/lib/`:**
- Purpose: Project-local service adapters on top of framework.
- Contains: Nested/state router endpoint wrappers and router declarations.
- Key files: `src/lib/HttpStateRouterEndpoint.h`, `src/lib/HttpNestedStateEndpoint.h`, `src/lib/JsonStateRouter.h`.

**`interface/`:**
- Purpose: Frontend SPA codebase consumed by firmware build pipeline.
- Contains: SvelteKit app config, route pages, reusable UI components, typed stores.
- Key files: `interface/package.json`, `interface/svelte.config.js`, `interface/vite.config.ts`, `interface/src/routes/+layout.svelte`, `interface/src/lib/stores/socket.ts`.

**`scripts/`:**
- Purpose: Build-time orchestration scripts called by PlatformIO.
- Contains: Interface build/embed flow, cert bundle generation, merge/rename/save artifact steps.
- Key files: `scripts/build_interface.py`, `scripts/generate_cert_bundle.py`, `scripts/merge_bin.py`, `scripts/rename_fw.py`, `scripts/save_elf.py`.

**`docs/`:**
- Purpose: User/developer docs authored with MkDocs.
- Contains: Build process, component architecture, REST API, stores, structure, getting-started docs.
- Key files: `docs/buildprocess.md`, `docs/restfulapi.md`, `docs/statefulservice.md`, `docs/structure.md`.

**`tests/`:**
- Purpose: Dedicated test location for firmware-side tests.
- Contains: No test files currently committed in this directory.
- Key files: Not applicable.

## Key File Locations

**Entry Points:**
- `src/main.cpp`: Firmware runtime composition and lifecycle entrypoint.
- `interface/src/routes/+layout.svelte`: UI root shell and websocket bootstrap.
- `interface/src/routes/+layout.ts`: UI global loader (`/rest/features`).
- `scripts/build_interface.py`: Frontend build/embed pre-script entry from PlatformIO.

**Configuration:**
- `platformio.ini`: Build environments, feature flags, dependencies, scripts.
- `factory_settings.ini`: Board/project compile-time defaults.
- `features.ini`: Feature toggles injected as build flags.
- `interface/tsconfig.json`: Frontend TypeScript configuration.
- `interface/.eslintrc.cjs`, `interface/.prettierrc`: Frontend lint/format settings.
- `mkdocs.yml`: Documentation build/navigation config.

**Core Logic:**
- `src/HeliostatService.*`: Heliostat state API + persistence wiring.
- `src/MovementSequencerService.*`: Axis sequencer API + persistence wiring.
- `src/TargetSequencerService.*`: Target sequence API + persistence wiring.
- `src/ArtNetService.*`, `src/ESPNowService.*`, `src/GPSService.*`: Communication and telemetry services.
- `lib/framework/ESP32SvelteKit.*`: Framework startup, route/static serving, system services.

**Testing:**
- `tests/`: Intended test root; currently empty.
- `interface/src/routes/demo/`: UI demo route used as reference flow, not automated tests.

## Naming Conventions

**Files:**
- Firmware services: PascalCase suffix `Service` (example: `src/HeliostatService.cpp`).
- Firmware domain/driver helpers: mostly lowercase or lower-mixed headers (`src/heliostat.h`, `src/servocontroller.h`, `src/adxl345.h`).
- Frontend routes: SvelteKit file conventions (`+page.svelte`, `+page.ts`, `+layout.svelte`, `+layout.ts`).
- Frontend components: PascalCase Svelte components (`interface/src/lib/components/StepperControlComp.svelte`).
- Scripts: snake_case Python filenames (`scripts/build_interface.py`).

**Directories:**
- Frontend route directories mirror URL segments (`interface/src/routes/connections/mqtt`, `interface/src/routes/system/update`).
- Framework and project C++ are separated by top-level roots (`lib/framework` vs `src`).

## Where to Add New Code

**New Feature:**
- Primary code: add domain controller/driver under `src/` and expose via a `*Service` in `src/`.
- Service wiring: instantiate and call `begin()/loop()` in `src/main.cpp`.
- API transport: prefer `HttpStateRouterEndpoint` + `EventEndpoint` + `FSPersistence` pattern used in `src/MovementSequencerService.h` and `src/TargetSequencerService.h`.
- Frontend integration: add route page in `interface/src/routes/<feature>/+page.svelte` and shared UI widgets in `interface/src/lib/components/`.

**New Component/Module:**
- Implementation: `interface/src/lib/components/<ComponentName>.svelte`.
- Shared client state: `interface/src/lib/stores/<store>.ts`.
- Route-level data loading: colocate `+page.ts` with route.

**Utilities:**
- Shared firmware helper wrappers: `src/lib/`.
- Shared framework-level reusable services: `lib/framework/`.
- Build tooling and release utilities: `scripts/`.

## Special Directories

**`build/`:**
- Purpose: Generated artifacts (ELF, merged bins, release checksums).
- Generated: Yes.
- Committed: Yes (release outputs tracked in repo snapshot).

**`interface/build/`:**
- Purpose: Generated static frontend output from Vite/SvelteKit static adapter.
- Generated: Yes.
- Committed: Yes.

**`interface/.svelte-kit/`:**
- Purpose: SvelteKit generated intermediate files/types.
- Generated: Yes.
- Committed: No (typically local build cache).

**`.pio/`:**
- Purpose: PlatformIO toolchain/build cache and libdeps.
- Generated: Yes.
- Committed: No (build cache directory).

## Navigation and Placement Commands

```bash
find src -maxdepth 1 -type f | sort                      # list firmware modules
find interface/src/routes -maxdepth 3 -type f | sort     # map UI route files
rg -n "class .*Service|StatefulService" src lib/framework  # find service patterns
rg -n "fetch\('/rest|socket\.on\(" interface/src            # find UI API/event usage
pio run -e esp32-s3-devkitc-1                            # build firmware stack
cd interface && npm run build                            # build static frontend
```

---

*Structure analysis: 2026-03-17*
