# AGENTS.md

> Keep `CLAUDE.md` in sync with this file — it's a copy used by Claude Code.

## Build

```bash
# Windows (MinGW) — Debug
cmake -B build_mingw -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Debug
cmake --build build_mingw

# Windows — deploy Qt DLLs for distribution
cmake --build build_mingw --target deploy

# One-click release packaging (build + deploy + strip credentials + zip)
# Requires bash: Git Bash, WSL, or MSYS2
bash package.sh v1.0.3

# Linux (native)
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build

# Windows → Linux cross-compile (Zig) — NOTE: cmake/toolchains/linux-x64.cmake
# is NOT in the repo; you must supply it externally.
export ZIG_PATH=/path/to/zig/zig.exe
cmake -B build_linux -G "MinGW Makefiles" \
      -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/linux-x64.cmake \
      -DZIG_PATH="$ZIG_PATH" -DCMAKE_BUILD_TYPE=Release
cmake --build build_linux
```

- C++17, Qt 6 (Core, Widgets, Mqtt), CMake ≥ 3.10
- Binary: `DjiCloudApi.exe` (Windows), `main` (Linux cross-compile)
- `cmake/toolchains/linux-x64.cmake` is **not committed** — the Zig cross-compile path needs it externally
- CMake copies `config/topic_mappings.json` → build dir on **every** build
- CMake copies `src/resources/config.json` → build dir `config.json` **only on first build** (preserves credentials)
- `package.sh` replaces `config.json` with `deploy/config.example.json` to strip credentials before zipping
- Build output goes to `build_mingw/`; `deploy/` holds pre-built distribution artifacts (exe is committed, DLLs are gitignored)
- No lint, typecheck, or test commands exist in this project

## Config file resolution

At runtime (see `src/main.cpp`):
1. `<exe_dir>/config/config.json` (primary — `config/` subdir is auto-created on startup)
2. `./config.json` (CWD-relative fallback for backward compat)

The CMake template `src/resources/config.json` is copied to the build root. At runtime the app may move/create it inside `config/`. When editing config, check both locations.

## Architecture

Three-layer, single-threaded — Qt signals/slots for async I/O:

```
UI Layer      src/ui/     — MainWindow, DeviceTreeWidget, OsdPanel, TopicParsePanel,
                            RawJsonPanel, PublishPanel, ConfigDialog, TopicListWidget,
                            DockControlPanel, FlightControlPanel, MaintenancePanel
Core Layer    src/core/   — DeviceManager (central dispatcher), ConfigStore, TopicManager,
                            TopicMapping, DeviceInfo/OsdData structs,
                            DockCommand/DockCommandExecutor (command system)
MQTT Layer    src/mqtt/   — MqttClientManager (QMqttClient wrapper, exponential backoff reconnect)
```

**Data flow:** MQTT message → `MqttClientManager::messageReceived` → `DeviceManager::parseAndRoute()` → `TopicManager` matches topic to device SN → parses DJI-format JSON `{"tid":..., "data":...}` → caches OSD, emits `deviceOsdUpdated(sn, rawJson)` → UI panels reactively update.

**Thread model:** Everything runs on the Qt main thread. `QMqttClient` is async — no worker threads.

**UI language:** All labels, field names, enum values are Chinese (UTF-8). Compiler flags enforce UTF-8 encoding: `/utf-8` (MSVC), `-fexec-charset=UTF-8` (GCC/MinGW).

### Key classes

| Class | Role |
|---|---|
| `DeviceManager` | Central dispatcher. Owns ConfigStore, TopicManager, MqttClientManager. Device CRUD, message routing, OSD caching (field-level merge via `mMergedOsdData`), profile switching. |
| `ConfigStore` | JSON config persistence with multi-profile support. Each profile has independent MQTT config + devices + topics. **Auto-saves on every mutation** — do not batch writes without considering this. |
| `TopicManager` | Topic ↔ device SN bidirectional mapping. Manages enabled/disabled state per topic, emits `topicsChanged` to trigger MQTT re-subscribe. |
| `MqttClientManager` | `QMqttClient` wrapper. Exponential backoff reconnect (base 1s, max 30s). Dedup subscription management. Tracks pending publishes via messageId→topic map. |
| `TopicMapping` | Loads `config/topic_mappings.json` — maps MQTT topic patterns to Chinese field names, units, enum translations, and group layouts. Topic keys prefixed with `dock/` or `aircraft/` for device-type-specific mappings. |
| `DockCommandExecutor` | Serial command executor. Publishes to `services` topic → subscribes `services_reply` → matches by `tid` → 10s timeout. Only one pending command at a time. |
| `MainWindow` | Top-level window (1280×760). Toolbar with config/connect/disconnect. Horizontal splitter: left = device tree + topic list (vertical split), right = OSD + TopicParse + RawJson + Publish (tabbed/stacked). Dock/Flight/Maintenance control panels open as separate dialogs from the "功能中心" toolbar menu. |

### Device hierarchy

`DeviceInfo` has `parentSn` — dock devices are top-level, aircraft can be children of a dock (`isChild()`). Tree renders as two-level hierarchy. Child aircraft are auto-discovered from dock OSD messages (`DeviceManager::checkAndAddChildAircraft`).

### OSD field-level merge

DJI sends OSD fields split across multiple MQTT messages. `DeviceManager` uses `mMergedOsdData` to merge fields incrementally — the latest value for each field key wins. The UI sees the merged result.

### Offline detection & JSON history

- Offline timeout: **5 seconds** (`OFFLINE_TIMEOUT_MS=5000`) — no message received → device marked offline
- JSON history: **500 entries** per device per topic (`MAX_JSON_HISTORY=500`)

### Profile switching

`DeviceManager::switchToProfile()` disconnects → clears **all** runtime state (devices, OSD caches, JSON history, topics) → loads new profile → reconnects if was connected. ConfigStore auto-saves on profile switch.

### Topic subscription pattern

Topics stored with `{sn}` placeholder in config; `TopicManager` resolves at runtime. Each device has its own topic list. Topics individually enable/disable. Disabled topics tracked separately and automatically unsubscribed.

### DJI MQTT topic convention

Common prefixes (replace `{sn}` with device serial):
- `thing/product/{sn}/osd` — telemetry/OSD data
- `thing/product/{sn}/state` — device state
- `thing/product/{sn}/services` — command requests (publish)
- `thing/product/{sn}/services_reply` — command responses (subscribe)
- `sys/product/{sn}/status` — system status

## Configuration files

- **`config.json`** — Multi-profile MQTT config + devices. Auto-generated on first run from `src/resources/config.json` template. Contains credentials — **never commit** (gitignored).
- **`config/topic_mappings.json`** — JSON field → Chinese translations. Topic keys use `{sn}` placeholder and optional `dock/`/`aircraft/` prefix. Copied to build dir on every build.
- **`deploy/config.example.json`** — Template with placeholder values, used during packaging.
- **`deploy/DjiCloudApi.exe`** + **`deploy/*.dll`** — Pre-built distribution binaries. DLLs are gitignored; the exe is committed.

## Unusual patterns

- `RawJsonPanel` is **header-only** (`RawJsonPanel.h` ~276 lines, no `.cpp`). Packet capture writes to `<exe_dir>/captures/`.
- `FlowLayout.h` is a standalone Qt layout helper, no `.cpp`.
- `DockControlDialog` / `FlightControlDialog` / `MaintenanceDialog` are thin dialog wrappers that embed their respective `*Panel` widget.
- Maintenance panel features are mostly "coming soon" stubs.

## No tests

This project has no automated test suite. All verification is manual.
