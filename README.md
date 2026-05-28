# EncManaged_pi

OpenCPN plugin that automatically syncs NOAA ENC (Electronic Navigational Charts) from the [NOAA ENC Product Catalog](https://www.charts.noaa.gov/ENCs/ENCProdCat.xml).

Self-contained — no external tools required. All sync logic is implemented in C++ inside the plugin.

## Features

- Scheduled background sync (hourly / every 6h / every 12h / daily / weekly)
- Manual "Sync Now" from preferences
- Atomic chart updates — safe to interrupt; existing charts never removed until replacement is ready
- Resume-safe — state saved per chart; re-run picks up where it left off
- Parallel downloads (configurable worker count)
- Writes `chartdldr_pi.dat` for OpenCPN's Chart Downloader plugin compatibility

## Requirements

- OpenCPN 5.2+ (plugin API 1.18)
- CMake 3.18+
- libcurl
- wxWidgets (headers only — OpenCPN provides the library at runtime)
- C++ compiler with C++17 support

All third-party source dependencies (tinyxml2, nlohmann/json, OpenCPN plugin header) are vendored in the repository.

## Build & Install

### Option A — Nix (recommended)

Nix provides a fully reproducible dev shell with all dependencies pinned.

```bash
# Install Nix with flakes enabled, then:
direnv allow        # auto-activates dev shell via .envrc
# or manually: nix develop

cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
cmake --install build
```

### Option B — Homebrew (macOS)

```bash
brew install cmake pkg-config curl wxwidgets

cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
cmake --install build
```

### Option C — apt (Debian / Ubuntu)

```bash
sudo apt install cmake pkg-config libcurl4-openssl-dev libwxgtk3.2-dev build-essential

cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
sudo cmake --install build
```

> For older Ubuntu (< 23.04) replace `libwxgtk3.2-dev` with `libwxgtk3.0-gtk3-dev`.

---

`cmake --install` copies the plugin library and registers it with OpenCPN by writing three files to OpenCPN's plugin config directory:

| File | Purpose |
|------|---------|
| `plugins/install_data/imports/encmanaged.xml` | Marks plugin as imported so OpenCPN shows it in the Plugins list |
| `plugins/install_data/encmanaged.files` | File manifest (used by OpenCPN for uninstall tracking) |
| `plugins/install_data/encmanaged.version` | Installed version; prevents a spurious "update available" button |

Without these files, OpenCPN silently drops the plugin from its internal array when the Plugins tab is opened and it never appears in the list.

## Setup

1. Restart OpenCPN after install
2. Open **Options → Plugins** — EncManaged appears in the list
3. Check **Enabled** and click **Apply** (OpenCPN may ask to restart)
4. Click the wrench icon to open preferences
5. Set chart directory (default: `~/Documents/Charts/MANAGED_ENC/US`)
6. Enable auto-refresh and choose sync interval
7. Click **Sync Now** for an immediate sync

Add the chart directory to OpenCPN: **Options → Charts → Add directory**.

After each sync, scan for new charts via **Tools → Charts → Scan for new/updated charts**.

## File layout

```
<chart_path>/
├── .sync-state.json      # per-chart edition/update tracking
├── chartdldr_pi.dat      # OpenCPN Chart Downloader plugin state
└── {CELLNAME}/
    ├── {CELLNAME}.000    # S-57 chart data
    └── {CELLNAME}A.TXT   # chart info
```

## Platform support

| Platform | Status | Notes |
|----------|--------|-------|
| macOS | Primary | Tested on arm64 + x86_64 |
| Linux | Supported | wxGTK required |
| Windows | Not scoped | OpenCPN/wxWidgets Windows build not tested |
