#include "sync_engine.h"
#include "catalog.h"
#include "state.h"
#include "downloader.h"
#include <filesystem>
#include <atomic>
#include <wx/wx.h>

namespace fs = std::filesystem;

wxDEFINE_EVENT(EVT_SYNC_PROGRESS, wxCommandEvent);
wxDEFINE_EVENT(EVT_SYNC_COMPLETE, wxCommandEvent);

SyncEngine::SyncEngine(wxEvtHandler* sink) : m_sink(sink) {}

SyncEngine::~SyncEngine() {
    RequestStop();
    if (m_thread.joinable()) m_thread.join();
}

void SyncEngine::Start(const SyncConfig& cfg) {
    if (m_running.exchange(true)) return;
    m_stopRequested = false;
    // Join previous thread if any before replacing
    if (m_thread.joinable()) m_thread.join();
    m_thread = std::thread(&SyncEngine::RunSync, this, cfg);
    // Do NOT detach — destructor joins to prevent use-after-free
}

void SyncEngine::RequestStop() {
    m_stopRequested = true;
}

static void PostProgress(wxEvtHandler* sink, const wxString& msg) {
    auto* ev = new wxCommandEvent(EVT_SYNC_PROGRESS);
    ev->SetString(msg);
    wxQueueEvent(sink, ev);
}

static void PostComplete(wxEvtHandler* sink, bool ok, const wxString& status) {
    auto* ev = new wxCommandEvent(EVT_SYNC_COMPLETE);
    ev->SetString(status);
    ev->SetInt(ok ? 1 : 0);
    wxQueueEvent(sink, ev);
}

void SyncEngine::RunSync(SyncConfig cfg) {
    // Clean up leftover temp/backup dirs from prior interrupted sync
    std::error_code ec;
    for (auto& entry : fs::directory_iterator(cfg.chartPath, ec)) {
        std::string name = entry.path().filename().string();
        bool isTemp   = (name.size() >= 5 && name.substr(0, 5) == ".tmp-");
        bool isBackup = (name.size() >= 4 && name.substr(name.size() - 4) == ".old");
        if (isTemp || isBackup) fs::remove_all(entry.path());
    }

    PostProgress(m_sink, "Fetching NOAA catalog...");
    std::vector<CellInfo> cells;
    std::string fetchErr;
    if (!FetchCatalog(cfg.catalogURL, cells, fetchErr)) {
        PostComplete(m_sink, false, "Catalog fetch failed: " + fetchErr);
        m_running = false;
        return;
    }
    PostProgress(m_sink, wxString::Format("Catalog: %zu cells", cells.size()));

    SyncState state = LoadState(cfg.chartPath);
    state.catalogFetchedAt = std::time(nullptr);

    std::vector<DownloadJob> toDownload;
    std::vector<std::string> toDelete;

    for (auto& cell : cells) {
        if (m_stopRequested) break;
        fs::path localDir = fs::path(cfg.chartPath) / cell.name;
        bool exists = fs::exists(localDir);

        if (cell.status == "Cancelled") {
            if (exists) toDelete.push_back(cell.name);
            continue;
        }
        auto it = state.cells.find(cell.name);
        if (!exists ||
            it == state.cells.end() ||
            it->second.edtn != cell.edtn ||
            it->second.updn != cell.updn)
        {
            toDownload.push_back({cell, cfg.chartPath});
        }
    }

    int unchanged = (int)cells.size() - (int)toDownload.size() - (int)toDelete.size();
    PostProgress(m_sink, wxString::Format(
        "Plan: %zu download, %zu delete, %d unchanged",
        toDownload.size(), toDelete.size(), unchanged));

    // failCount modified from multiple worker threads — must be atomic
    std::atomic<int> failCount{0};

    if (!toDownload.empty()) {
        PostProgress(m_sink, wxString::Format(
            "Downloading %zu charts (%d workers)...", toDownload.size(), cfg.workers));

        // Build cell lookup for edtn/updn in callback
        std::unordered_map<std::string, const CellInfo*> cellMap;
        for (auto& cell : cells) cellMap[cell.name] = &cell;

        RunDownloads(toDownload, cfg.workers, [&](const DownloadResult& r) {
            if (r.ok) {
                auto it = cellMap.find(r.cellName);
                if (it != cellMap.end()) {
                    SaveOne(cfg.chartPath, state, r.cellName,
                            it->second->edtn, it->second->updn);
                }
                PostProgress(m_sink, "  OK    " + r.cellName);
            } else {
                ++failCount;
                PostProgress(m_sink, "  FAIL  " + r.cellName + ": " + r.err);
            }
        });
    }

    int fc = failCount.load();

    if (!toDelete.empty()) {
        if (fc > 0) {
            PostProgress(m_sink, wxString::Format(
                "Skipping %zu deletions — %d download(s) failed",
                toDelete.size(), fc));
        } else {
            for (auto& name : toDelete) {
                fs::remove_all(fs::path(cfg.chartPath) / name, ec);
                {
                    std::lock_guard<std::mutex> lock(*state.mu);
                    state.cells.erase(name);
                }
                PostProgress(m_sink, "  DELETE " + name);
            }
        }
    }

    WriteChtdldr(cfg.chartPath, state);

    int downloaded = (int)toDownload.size() - fc;
    wxString status = fc > 0
        ? wxString::Format("%d chart(s) failed — re-run to retry", fc)
        : wxString::Format("OK — %d updated, %zu deleted, %d unchanged",
                           downloaded, toDelete.size(), unchanged);

    PostComplete(m_sink, fc == 0, status);
    m_running = false;
}
