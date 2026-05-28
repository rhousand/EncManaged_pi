// Downloads 2 small real ENC cells to /tmp, verifies atomic swap + state update.
// Requires network access. Takes ~30s depending on connection speed.
#include "../src/catalog.h"
#include "../src/downloader.h"
#include "../src/state.h"
#include <iostream>
#include <cassert>
#include <filesystem>

namespace fs = std::filesystem;

int main() {
    std::string dir = "/tmp/enc_dl_test";
    fs::remove_all(dir);
    fs::create_directories(dir);

    // Fetch real catalog to get valid URLs
    std::vector<CellInfo> cells;
    std::string err;
    std::cout << "Fetching catalog...\n";
    bool ok = FetchCatalog("https://www.charts.noaa.gov/ENCs/ENCProdCat.xml", cells, err);
    if (!ok) { std::cerr << "Catalog fetch failed: " << err << "\n"; return 1; }

    // Pick 2 small active cells (US5xxx = large-scale, usually < 1MB)
    std::vector<DownloadJob> jobs;
    for (auto& c : cells) {
        if (c.status == "Active" && c.name.substr(0, 3) == "US5" && c.zipfileSize > 0) {
            jobs.push_back({c, dir});
            if (jobs.size() == 2) break;
        }
    }

    if (jobs.empty()) {
        std::cerr << "No suitable cells found\n";
        return 1;
    }

    std::cout << "Downloading " << jobs.size() << " cells:\n";
    for (auto& j : jobs) std::cout << "  " << j.cell.name << " (" << j.cell.zipfileSize << " bytes)\n";

    SyncState state = LoadState(dir);
    int failCount = 0;

    RunDownloads(jobs, 2, [&](const DownloadResult& r) {
        if (r.ok) {
            for (auto& j : jobs) {
                if (j.cell.name == r.cellName) {
                    SaveOne(dir, state, r.cellName, j.cell.edtn, j.cell.updn);
                    break;
                }
            }
            std::cout << "  OK    " << r.cellName << "\n";
        } else {
            ++failCount;
            std::cerr << "  FAIL  " << r.cellName << ": " << r.err << "\n";
        }
    });

    if (failCount > 0) { std::cerr << failCount << " downloads failed\n"; return 1; }

    // Verify directories exist and state was saved
    for (auto& j : jobs) {
        fs::path cellDir = fs::path(dir) / j.cell.name;
        assert(fs::exists(cellDir) && fs::is_directory(cellDir));
        assert(state.cells.count(j.cell.name));
        std::cout << "  " << j.cell.name << " verified on disk\n";
    }

    // Run again — should overwrite via atomic swap without error
    std::cout << "Re-downloading (atomic swap over existing)...\n";
    RunDownloads(jobs, 2, [&](const DownloadResult& r) {
        if (!r.ok) { std::cerr << "  FAIL  " << r.cellName << ": " << r.err << "\n"; ++failCount; }
        else std::cout << "  OK    " << r.cellName << "\n";
    });
    assert(failCount == 0);

    fs::remove_all(dir);
    std::cout << "All downloader tests PASS\n";
    return 0;
}
