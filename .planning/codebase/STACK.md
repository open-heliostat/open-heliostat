# Technology Stack

**Analysis Date:** 2026-03-17

## Languages

**Primary:**
- C++ (Arduino framework on ESP32) - Firmware and device services under `src/*.cpp`, `src/*.h`, and framework code under `lib/framework/*.h`, `lib/framework/*.cpp`.
- TypeScript + Svelte - Frontend UI in `interface/src/**/*.ts` and `interface/src/**/*.svelte`.

**Secondary:**
- Python 3 - Build/release tooling in `scripts/*.py` and CI docs deployment in `.github/workflows/ci.yaml`.
- Markdown + MkDocs YAML - Documentation source under `docs/*.md` and site config in `mkdocs.yml`.

## Runtime

**Environment:**
- ESP32 microcontrollers via PlatformIO/Arduino (`platformio.ini` with `framework = arduino` and `platform = espressif32 @ 6.12.0`).
- Browser runtime for static frontend output built by Vite/SvelteKit (`interface/svelte.config.js`, `interface/vite.config.ts`).
- Node.js runtime required for frontend toolchain (declared in docs at `docs/gettingstarted.md`; version not pinned by `.nvmrc`).
- Python runtime required for build scripts and CI (`scripts/*.py`, `.github/workflows/ci.yaml` uses `python-version: 3.x`).

**Package Manager:**
- npm (detected by `interface/package-lock.json` and install command in `scripts/build_interface.py`).
- Lockfile: present (`interface/package-lock.json`).

## Frameworks

**Core:**
- Arduino framework for ESP32 firmware (`platformio.ini`).
- ESP32-SvelteKit backend framework primitives (StatefulService/HttpEndpoint/EventSocket/MqttEndpoint) in `lib/framework/`.
- SvelteKit (`@sveltejs/kit`) for UI app (`interface/package.json`, `interface/svelte.config.js`).

**Testing:**
- No active firmware or frontend test runner configuration detected in repository root or `interface/` (`tests/` exists but empty; no `jest.config.*`/`vitest.config.*`).

**Build/Dev:**
- PlatformIO build orchestration (`platformio.ini`) with extra scripts:
  - `scripts/build_interface.py`
  - `scripts/generate_cert_bundle.py`
  - `scripts/merge_bin.py`
  - `scripts/rename_fw.py`
  - `scripts/save_elf.py`
- Vite 5 (`vite`) and Svelte plugin pipeline in `interface/vite.config.ts`.
- Tailwind CSS 4 + DaisyUI in frontend (`interface/package.json`).
- MkDocs Material docs toolchain (`mkdocs.yml`, `.github/workflows/ci.yaml`).

## Key Dependencies

**Critical:**
- `@sveltejs/kit` `^2.22.3` - Frontend application framework (`interface/package.json`).
- `svelte` `^5.35.5` - Component runtime (`interface/package.json`).
- `vite` `^5.4.19` - Frontend build/dev server (`interface/package.json`).
- `tailwindcss` `^4.1.11` + `daisyui` `^5.0.46` - UI styling system (`interface/package.json`).
- `jwt-decode` `^4.0.0` - Client-side JWT parsing (`interface/src/lib/stores/user.ts`).
- `msgpack-lite` `^0.1.26` - Event socket payload encoding (`interface/src/lib/stores/socket.ts`).

**Infrastructure:**
- PlatformIO library dependencies in `platformio.ini`:
  - `ArduinoJson@>=7.0.0`
  - `elims/PsychicMqttClient@^0.2.4`
  - GitHub libs: `TMCStepper`, `FastAccelStepper`, `TinyGPSPlus`, `Time`, `SolarPosition`
- Python script dependencies inferred from imports:
  - `requests` and `cryptography` in `scripts/generate_cert_bundle.py`
  - Script attempts runtime install of `cryptography` if missing.

## Configuration

**Environment:**
- No `.env` file detected at repository root (checked by shell glob with nullglob); configuration is compile-time and file-based.
- Primary configuration surfaces:
  - `platformio.ini` (toolchain, board envs, build flags, scripts)
  - `factory_settings.ini` (default WiFi/AP/NTP/MQTT/JWT settings via `-D` build flags)
  - `features.ini` (feature toggles like `FT_MQTT`, `FT_NTP`, `FT_DOWNLOAD_FIRMWARE`)

**Build:**
- Firmware build: `platformio.ini` environments (`esp32-s3-devkitc-1`, `esp32-c3-devkitm-1`, `esp32dev`, `Kincony-B16M`, `esp32-wt32-eth01`).
- Frontend build command path: `scripts/build_interface.py` executes:
  - `npm install`
  - `npm run build`
- Frontend output mode:
  - Embedded into firmware as `lib/framework/WWWData.h` when `EMBED_WWW` flag is set.
  - Or copied/gzipped to `data/www` for LittleFS upload.
- Release artifact packaging:
  - merged binaries to `build/merged/*_webflash.bin` (`scripts/merge_bin.py`)
  - renamed release binaries and md5 to `build/release/` (`scripts/rename_fw.py`)

## Platform Requirements

**Development:**
- PlatformIO CLI/extension and ESP32 toolchain (`docs/gettingstarted.md`, `platformio.ini`).
- Node.js + npm for interface build (`docs/gettingstarted.md`, `scripts/build_interface.py`).
- Python 3 with `requests`/`cryptography` for build helper scripts (`scripts/generate_cert_bundle.py`).
- Optional docs toolchain: `mkdocs-material` (`mkdocs.yml`, `.github/workflows/ci.yaml`).

**Production:**
- ESP32 target boards flashed with PlatformIO-generated binaries (`platformio.ini` envs).
- Frontend served directly from device flash (embedded data or LittleFS).
- OTA update pipeline expects HTTPS-hosted firmware binaries and optional GitHub release workflow integration (`docs/restfulapi.md`, `interface/src/lib/components/UpdateIndicator.svelte`).

## Common Commands

```bash
# Firmware build/upload
pio run -e esp32-s3-devkitc-1
pio run -e esp32-s3-devkitc-1 -t upload

# Frontend local development/build
cd interface && npm install
cd interface && npm run dev
cd interface && npm run build

# Documentation
mkdocs serve
mkdocs gh-deploy --force
```

---

*Stack analysis: 2026-03-17*
