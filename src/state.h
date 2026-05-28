#pragma once
#include <string>
#include <unordered_map>
#include <mutex>
#include <memory>
#include <ctime>

struct CellEntry {
    int    edtn{0};
    int    updn{0};
    time_t downloadedAt{0};
};

// mu is heap-allocated so SyncState is movable (std::mutex is non-movable).
struct SyncState {
    time_t catalogFetchedAt{0};
    std::unordered_map<std::string, CellEntry> cells;
    mutable std::unique_ptr<std::mutex> mu{std::make_unique<std::mutex>()};
};

// Load state from <dir>/.sync-state.json. Returns empty state if file absent.
SyncState LoadState(const std::string& dir);

// Save full state atomically (tmp + rename).
bool SaveState(const std::string& dir, SyncState& state);

// Update one cell and save atomically. Thread-safe.
bool SaveOne(const std::string& dir, SyncState& state,
             const std::string& cellName, int edtn, int updn);

// Write <dir>/chartdldr_pi.dat for OpenCPN's Chart Downloader plugin.
bool WriteChtdldr(const std::string& dir, const SyncState& state);
