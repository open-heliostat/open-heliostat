# External Integrations

**Analysis Date:** 2026-03-17

## APIs & External Services

**Firmware Update / Release Distribution:**
- GitHub Releases API - Used by frontend to discover available firmware versions.
  - SDK/Client: Browser `fetch` in `interface/src/routes/system/update/GithubFirmwareManager.svelte` and `interface/src/lib/components/UpdateIndicator.svelte`
  - Auth: none for public release reads; request headers include GitHub API version.
  - Endpoints used:
    - `https://api.github.com/repos/${repo}/releases`
    - `https://api.github.com/repos/${repo}/releases/latest`
- OTA binary download by device - Frontend posts a release asset URL to backend endpoint `/rest/downloadUpdate`; firmware downloads over HTTPS.
  - Request path: `/rest/downloadUpdate` in `docs/restfulapi.md`
  - Frontend call sites: `interface/src/routes/system/update/GithubFirmwareManager.svelte`, `interface/src/lib/components/UpdateIndicator.svelte`

**Messaging / IoT Protocols:**
- MQTT broker integration - Device connects to external broker when enabled.
  - SDK/Client: `PsychicMqttClient` via `lib/framework/MqttSettingsService.h` and `lib/framework/MqttEndpoint.h`
  - Auth/config source: compile-time defaults in `factory_settings.ini` (`FACTORY_MQTT_URI`, `FACTORY_MQTT_USERNAME`, `FACTORY_MQTT_PASSWORD`) and runtime settings via `/rest/mqttSettings`.
  - Default URI: `mqtts://broker.hivemq.com:8883` in `factory_settings.ini`.
- NTP synchronization - Device time sync for clock and solar calculations.
  - SDK/Client: framework NTP services (`lib/framework/NTPStatus.h` plus service usage in firmware)
  - Auth/config source: `FACTORY_NTP_SERVER` and timezone flags in `factory_settings.ini`.
  - Default server: `time.google.com`.

**Certificate / Trust Store Sources:**
- Root CA bundle download during build:
  - `https://curl.se/ca/cacert.pem`
  - `https://raw.githubusercontent.com/adafruit/certificates/main/data/roots-filtered.pem`
  - `https://raw.githubusercontent.com/adafruit/certificates/main/data/roots-full.pem`
  - Implemented in `scripts/generate_cert_bundle.py` and controlled by `board_ssl_cert_source` in `platformio.ini`.

**Local-Network Service Discovery/Control:**
- mDNS service advertisement/discovery for HTTP/WS and remote lookup.
  - Provider: ESP mDNS in firmware
  - Files: `lib/framework/ESP32SvelteKit.cpp`, `src/remotes.h`, `src/espnow.cpp`
- ArtNet over UDP for lighting/control frames.
  - Files: `src/ArtNetService.cpp`, route page `interface/src/routes/artnet/+page.svelte`
- ESP-NOW peer messaging (ESP32-to-ESP32 local wireless).
  - Files: `src/espnow.cpp`, `src/ESPNowService.cpp`, UI `interface/src/routes/connections/espnow/+page.svelte`

## Data Storage

**Databases:**
- Not detected (no SQL/NoSQL engine or ORM in firmware/interface).

**File Storage:**
- Device local storage: LittleFS (`platformio.ini` `board_build.filesystem = littlefs`).
- Stateful JSON persistence via framework `FSPersistence` to `/config/*.json` paths (examples in `src/GPSService.h`, `lib/framework/SecuritySettingsService.h`, `src/RemoteService.h`).
- Frontend static assets:
  - Embedded C++ header `lib/framework/WWWData.h` (when `EMBED_WWW` flag active)
  - or LittleFS `data/www` built by `scripts/build_interface.py`.

**Caching:**
- No dedicated external cache service detected.
- Browser-side local storage used for user JWT/session object in `interface/src/lib/stores/user.ts`.

## Authentication & Identity

**Auth Provider:**
- Custom local authentication with JWT generation/verification on device.
  - Implementation: `lib/framework/SecuritySettingsService.h` and `lib/framework/ArduinoJsonJWT.cpp`
  - API endpoints documented in `docs/restfulapi.md`: `/rest/signIn`, `/rest/verifyAuthorization`, `/rest/generateToken`, `/rest/securitySettings`.

**Frontend Auth Handling:**
- JWT decoding and persistence in browser localStorage via `jwt-decode` in `interface/src/lib/stores/user.ts`.
- Bearer token sent to secured REST routes, including OTA download trigger (`Authorization: Bearer ...`) in `interface/src/lib/components/UpdateIndicator.svelte`.

## Monitoring & Observability

**Error Tracking:**
- No external error tracking SaaS detected.

**Logs:**
- Device serial logging (feature flags such as `SERIAL_INFO`, `CORE_DEBUG_LEVEL` in `platformio.ini`).
- Optional `teleplot` instrumentation exists in codebase (`src/teleplot.h`) and compile-time flag comments (`platformio.ini`).
- Build artifacts/log outputs stored under `logs/` and `build/`.

## CI/CD & Deployment

**Hosting:**
- Firmware artifacts are generated locally by PlatformIO and intended for OTA distribution (commonly GitHub releases per docs).
- Documentation site deployment uses GitHub Pages via MkDocs GitHub Action.

**CI Pipeline:**
- GitHub Actions workflow at `.github/workflows/ci.yaml`:
  - Python setup
  - `pip install mkdocs-material`
  - `mkdocs gh-deploy --force`
- No automated firmware build workflow detected in `.github/workflows/`.

## Environment Configuration

**Required env vars:**
- Traditional `.env` environment variables are not the primary config mechanism.
- Required configuration is compile-time via `-D` flags in `platformio.ini`, `factory_settings.ini`, and `features.ini`.
- Critical integration-related defines include:
  - `FACTORY_MQTT_URI`, `FACTORY_MQTT_USERNAME`, `FACTORY_MQTT_PASSWORD`, `FACTORY_MQTT_CLIENT_ID`
  - `FACTORY_NTP_SERVER`, `FACTORY_NTP_TIME_ZONE_LABEL`, `FACTORY_NTP_TIME_ZONE_FORMAT`
  - `FACTORY_JWT_SECRET`
  - `FT_MQTT`, `FT_NTP`, `FT_DOWNLOAD_FIRMWARE`, `FT_SECURITY`
  - `board_ssl_cert_source`

**Secrets location:**
- Build-time defaults and credentials are stored as compile definitions in `factory_settings.ini`.
- Runtime credential updates are persisted in LittleFS config files via `FSPersistence` services.

## Webhooks & Callbacks

**Incoming:**
- No webhook-style public callback endpoints detected.
- Incoming protocol callbacks are local/transport callbacks:
  - REST handlers (`/rest/*`) via `HttpEndpoint` and router endpoints (`lib/framework/HttpEndpoint.h`, `src/lib/HttpStateRouterEndpoint.h`)
  - WebSocket/event subscriptions via EventSocket (`interface/src/lib/stores/socket.ts`, framework event services)
  - MQTT message callbacks in `lib/framework/MqttEndpoint.h`
  - ArtNet UDP packet handlers in `src/ArtNetService.cpp`
  - ESP-NOW message callbacks in `src/espnow.cpp`

**Outgoing:**
- Frontend outgoing HTTPS calls to GitHub API for release metadata (`GithubFirmwareManager.svelte`, `UpdateIndicator.svelte`).
- Device outgoing TLS connections for MQTT/NTP/OTA download when enabled (`factory_settings.ini`, `docs/buildprocess.md`, `/rest/downloadUpdate` in `docs/restfulapi.md`).

## Commands for Integration Verification

```bash
# Inspect effective build flags for integrations
pio run -e esp32-s3-devkitc-1 -t envdump

# Build certificate bundle path used by firmware
pio run -e esp32-s3-devkitc-1

# Frontend OTA/GitHub integration components
rg -n "api.github.com|/rest/downloadUpdate" interface/src

# Firmware integration surfaces (MQTT/NTP/ESPNow/ArtNet)
rg -n "Mqtt|NTP|ESPNow|ArtNet|downloadUpdate|FSPersistence" src lib/framework
```

---

*Integration audit: 2026-03-17*
