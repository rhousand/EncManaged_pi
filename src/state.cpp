#include "state.h"
#include "nlohmann/json.hpp"
#include <fstream>
#include <cctype>

using json = nlohmann::json;

static std::string StatePath(const std::string& dir) {
    return dir + "/.sync-state.json";
}

SyncState LoadState(const std::string& dir) {
    SyncState s;
    std::ifstream f(StatePath(dir));
    if (!f.is_open()) return s;

    json j;
    try { f >> j; } catch (...) { return s; }

    s.catalogFetchedAt = j.value("catalog_fetched_at", (long long)0);
    // Assign to named var — range-for over .items() on a temporary is UB
    auto cells_j = j.value("cells", json::object());
    for (auto& [name, entry] : cells_j.items()) {
        CellEntry ce;
        ce.edtn         = entry.value("edtn", 0);
        ce.updn         = entry.value("updn", 0);
        ce.downloadedAt = entry.value("downloaded_at", (long long)0);
        s.cells[name]   = ce;
    }
    return s;
}

static bool WriteStateFile(const std::string& dir, SyncState& state) {
    json j;
    j["catalog_fetched_at"] = state.catalogFetchedAt;
    j["cells"] = json::object();
    for (auto& [name, ce] : state.cells) {
        j["cells"][name] = {
            {"edtn",          ce.edtn},
            {"updn",          ce.updn},
            {"downloaded_at", (long long)ce.downloadedAt}
        };
    }

    std::string tmp = StatePath(dir) + ".tmp";
    {
        std::ofstream f(tmp);
        if (!f.is_open()) return false;
        f << j.dump(2);
    }
    return std::rename(tmp.c_str(), StatePath(dir).c_str()) == 0;
}

bool SaveState(const std::string& dir, SyncState& state) {
    std::lock_guard<std::mutex> lock(*state.mu);
    return WriteStateFile(dir, state);
}

bool SaveOne(const std::string& dir, SyncState& state,
             const std::string& cellName, int edtn, int updn)
{
    std::lock_guard<std::mutex> lock(*state.mu);
    state.cells[cellName] = CellEntry{edtn, updn, std::time(nullptr)};
    return WriteStateFile(dir, state);
}

bool WriteChtdldr(const std::string& dir, const SyncState& state) {
    std::string tmp = dir + "/chartdldr_pi.dat.tmp";
    {
        std::ofstream f(tmp);
        if (!f.is_open()) return false;
        f << "encprodcat " << state.catalogFetchedAt << "\n";
        for (auto& [name, ce] : state.cells) {
            std::string lower = name;
            for (char& c : lower) c = (char)std::tolower((unsigned char)c);
            f << lower << " " << ce.downloadedAt << "\n";
        }
    }
    return std::rename(tmp.c_str(), (dir + "/chartdldr_pi.dat").c_str()) == 0;
}
