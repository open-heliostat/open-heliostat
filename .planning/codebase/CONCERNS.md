# Codebase Concerns

**Analysis Date:** 2026-03-17

## Tech Debt

**Cross-stack typing and maintainability debt:**
- Issue: Extensive use of dynamic typing (`any`) and broad JSON/object mutation in UI and service adapters increases runtime error surface and weakens refactor safety.
- Files: `interface/src/routes/connections/mqtt/MQTT.svelte`, `interface/src/routes/connections/ntp/NTP.svelte`, `interface/src/routes/wifi/sta/Wifi.svelte`, `interface/src/lib/components/DraggableList.svelte`, `src/lib/HttpNestedStateEndpoint.h`, `src/lib/HttpStateRouterEndpoint.h`
- Impact: Type regressions become production defects instead of compile-time failures; nested-path writes are easy to break silently.
- Fix approach: Introduce stricter TS typing for API payloads and replace ad-hoc nested JSON path writes with typed helper functions and schema validation.

**Monolithic UI components with mixed responsibilities:**
- Issue: Very large Svelte pages combine API orchestration, presentation, intervals, and state normalization in single files.
- Files: `interface/src/routes/wifi/sta/Wifi.svelte`, `interface/src/routes/ethernet/Ethernet.svelte`, `interface/src/routes/system/metrics/SystemMetrics.svelte`, `interface/src/routes/artnet/+page.svelte`
- Impact: Higher change risk, difficult code review, and repeated polling/event handling patterns.
- Fix approach: Split into container + presentational components and centralize polling/socket update logic in reusable stores.

**Build pipeline debt for interface artifacts:**
- Issue: Prebuild script runs dependency install during firmware builds (`npm install` / `pnpm install` / `yarn install`) instead of using a deterministic CI-prepared artifact.
- Files: `scripts/build_interface.py`
- Impact: Slower builds, network-dependent builds, increased CI flakiness, and lockfile drift risks.
- Fix approach: Build frontend in a dedicated step, fail if lockfile and dependencies are missing, and avoid installing dependencies during embedded-firmware build.

## Known Bugs

**Metrics pages schedule timer with wrong `setInterval` signature:**
- Symptoms: Update callback is scheduled without explicit interval argument; expression `updateData(), 2000`/`5000` runs inside callback body and does not configure timing.
- Files: `interface/src/routes/system/metrics/SystemMetrics.svelte`, `interface/src/routes/system/metrics/BatteryMetrics.svelte`
- Trigger: Opening metrics pages after mount.
- Workaround: None in code; temporary mitigation is to avoid long-duration metrics page sessions.

**Firmware update indicator leaks periodic checks across mounts:**
- Symptoms: Hourly GitHub polling interval is created but never cleared on unmount.
- Files: `interface/src/lib/components/UpdateIndicator.svelte`
- Trigger: Repeated navigation/remount of layout where component is mounted.
- Workaround: Reload page to reset browser process state.

**Potential logging format misuse in ESP-NOW send callback:**
- Symptoms: Packet loss log prints MAC buffer using `%s` with binary pointer.
- Files: `src/espnow.cpp`
- Trigger: Failed ESP-NOW send path in `sentCallback`.
- Workaround: None.

## Security Considerations

**Security features disabled by default in current build config:**
- Risk: Auth feature compile flag is set off by default in repository config.
- Files: `features.ini`
- Current mitigation: Optional feature flags exist.
- Recommendations: Set `-D FT_SECURITY=1` for production profiles and maintain separate dev/prod feature presets.

**Insecure default credentials in factory settings:**
- Risk: Default AP and user credentials are static and predictable (`admin/admin`, `guest/guest`, AP password).
- Files: `factory_settings.ini`
- Current mitigation: Values are configurable at build time.
- Recommendations: Enforce unique credentials at first boot, block empty/weak passwords, and fail production build when defaults are unchanged.

**OTA certificate verification bypass enabled in build flags:**
- Risk: Build flag enables skipping certificate verification during firmware download.
- Files: `platformio.ini`
- Current mitigation: Cert bundle support exists (`src/certs/x509_crt_bundle.bin`).
- Recommendations: Disable `-D DOWNLOAD_OTA_SKIP_CERT_VERIFY` for release environments.

**Token storage and transport risks documented by project itself:**
- Risk: JWT persisted in browser local storage and often used over non-TLS LAN traffic.
- Files: `interface/src/lib/stores/user.ts`, `docs/stores.md`, `docs/index.md`
- Current mitigation: Role model and bearer checks exist.
- Recommendations: Move to secure cookie/session pattern where possible, minimize token lifetime, and enforce HTTPS for deployments with security enabled.

## Performance Bottlenecks

**Aggressive polling and repeated fetch patterns across UI routes:**
- Problem: Many pages poll REST endpoints every 1-5 seconds, in parallel with websocket events.
- Files: `interface/src/routes/wifi/sta/Wifi.svelte`, `interface/src/routes/wifi/ap/Accesspoint.svelte`, `interface/src/routes/connections/mqtt/MQTT.svelte`, `interface/src/routes/connections/ntp/NTP.svelte`, `interface/src/lib/components/ESPNowConsoleRestComp.svelte`
- Cause: Route-local intervals instead of centralized freshness strategy.
- Improvement path: Prefer websocket-first updates with adaptive/fallback polling and global deduped request scheduler.

**Firmware dynamic allocation and `String` churn in hot communication paths:**
- Problem: Frequent dynamic string/vector operations in ESP-NOW callbacks and service update code.
- Files: `src/espnow.cpp`, `src/ESPNowService.cpp`, `src/lib/HttpStateRouterEndpoint.h`, `src/lib/HttpNestedStateEndpoint.h`
- Cause: High-frequency allocations in constrained memory environment.
- Improvement path: Replace dynamic `String` composition in callback paths with fixed-size buffers where practical and preallocate bounded containers.

**Build-time overhead from embedded frontend transformation:**
- Problem: Frontend install/build/compress runs during firmware build and can dominate iteration time.
- Files: `scripts/build_interface.py`
- Cause: Coupled firmware and frontend build pipeline.
- Improvement path: Add incremental cache checks and prebuilt artifact mode in CI.

## Fragile Areas

**HTTP nested state routing and path resolution:**
- Files: `src/lib/HttpNestedStateEndpoint.h`, `src/lib/HttpStateRouterEndpoint.h`
- Why fragile: Small path parsing changes alter update/read behavior for all nested REST endpoints.
- Safe modification: Add endpoint-level tests or at least scripted contract checks for representative nested paths before refactors.
- Test coverage: No first-party automated tests detected for these routing primitives.

**Control-loop and service-loop concurrency boundaries:**
- Files: `src/main.cpp`, `src/abstractcontroller.h`, `src/servocontroller.h`, `src/closedloopcontroller.h`, `src/GPSService.cpp`
- Why fragile: Multiple tasks/loops run on different cores with shared controller state and mutex protection mixed with frequent updates.
- Safe modification: Keep lock boundaries explicit and benchmark loop timing after each logic change.
- Test coverage: No hardware-in-the-loop regression suite in repository tests.

**Large UI route modules with embedded network side effects:**
- Files: `interface/src/routes/wifi/sta/Wifi.svelte`, `interface/src/routes/ethernet/Ethernet.svelte`, `interface/src/routes/system/metrics/SystemMetrics.svelte`
- Why fragile: UI changes can inadvertently alter auth headers, polling cadence, and lifecycle cleanup.
- Safe modification: Extract request logic into typed stores and assert timer cleanup in component tests.
- Test coverage: No UI component/integration test suite configured in `interface/package.json` scripts.

## Scaling Limits

**HTTP endpoint count and memory footprint constraints:**
- Current capacity: Endpoint count must be manually sized (`ESP32SvelteKit esp32sveltekit(&server, 200)`), and docs note endpoint memory pressure on ESP-IDF HTTP server.
- Limit: Feature growth can exceed static endpoint provisioning and memory budget.
- Scaling path: Track endpoint count in generated WWW/REST assets and enforce capacity checks in CI before release.

**Embedded filesystem/UI asset size pressure:**
- Current capacity: Build docs indicate constrained partition sizing and LITTLEFS limits when embedding frontend.
- Limit: UI growth competes with firmware code and OTA partitions.
- Scaling path: Use larger partition schemes per board, reduce UI bundle size, and selectively disable nonessential features.

## Dependencies at Risk

**Unpinned VCS dependencies in firmware toolchain:**
- Risk: Several libraries are pulled directly from GitHub URLs without immutable commit pinning.
- Impact: Upstream changes can introduce non-deterministic build/runtime regressions.
- Migration plan: Pin to tags/commits in `platformio.ini`, periodically bump with compatibility testing.

**Release integrity uses MD5 checksum generation:**
- Risk: Integrity metadata generation script uses MD5 for firmware checksum output.
- Impact: Weak hash for security-sensitive integrity guarantees.
- Migration plan: Add SHA-256 (or stronger) alongside MD5 for compatibility migration.

## Missing Critical Features

**No first-party automated test suite for firmware or UI:**
- Problem: `tests/` is effectively empty while complexity is high across control loops, networking, and UI auth/state flows.
- Blocks: Safe refactoring, regression detection, and confidence in security/performance fixes.

**No enforced production security profile:**
- Problem: Security-sensitive build flags and defaults are configurable but not guarded by release checks.
- Blocks: Reliable secure-by-default releases across environments.

## Test Coverage Gaps

**State routing and auth-protected endpoint behavior:**
- What's not tested: Nested path read/write contracts, auth predicate behavior on sensitive endpoints, malformed JSON behavior.
- Files: `src/lib/HttpNestedStateEndpoint.h`, `src/lib/HttpStateRouterEndpoint.h`, `src/ESPNowService.cpp`, `src/ArtNetService.cpp`
- Risk: Breaking API semantics or authorization behavior without detection.
- Priority: High

**UI lifecycle cleanup and polling correctness:**
- What's not tested: Interval setup/teardown, remount behavior, rate of background polling, and reconnection edge cases.
- Files: `interface/src/routes/system/metrics/SystemMetrics.svelte`, `interface/src/routes/system/metrics/BatteryMetrics.svelte`, `interface/src/lib/components/UpdateIndicator.svelte`, `interface/src/lib/stores/socket.ts`
- Risk: Memory leaks, noisy network load, stale state updates.
- Priority: High

**Build scripts and artifact reproducibility:**
- What's not tested: Deterministic frontend embed output and release artifact integrity validation.
- Files: `scripts/build_interface.py`, `scripts/rename_fw.py`
- Risk: CI instability and accidental release variance.
- Priority: Medium

## Verification Commands

```bash
# Find explicit debt markers in project code
rg -n "TODO|FIXME|HACK|XXX" src interface/src scripts docs tests --glob '!**/build/**'

# Surface polling/timer-heavy UI files
rg -n "setInterval\(|setTimeout\(" interface/src --glob '*.{svelte,ts}'

# Check for absent first-party tests (exclude dependency caches)
find src interface/src scripts tests -type f \( -name '*.test.*' -o -name '*.spec.*' \)

# Build firmware for primary target
pio run -e esp32-s3-devkitc-1

# Run frontend static checks
npm --prefix interface run check && npm --prefix interface run lint
```

---

*Concerns audit: 2026-03-17*
