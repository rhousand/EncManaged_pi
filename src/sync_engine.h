#pragma once
#include <string>
#include <atomic>
#include <thread>
#include <wx/event.h>

wxDECLARE_EVENT(EVT_SYNC_PROGRESS, wxCommandEvent);
wxDECLARE_EVENT(EVT_SYNC_COMPLETE, wxCommandEvent);

struct SyncConfig {
    std::string chartPath;
    std::string catalogURL;
    int         workers{4};
};

class SyncEngine {
public:
    explicit SyncEngine(wxEvtHandler* sink);
    ~SyncEngine();

    bool IsRunning() const { return m_running.load(); }

    // Starts background sync. Posts EVT_SYNC_PROGRESS and EVT_SYNC_COMPLETE to sink.
    // No-op if already running.
    void Start(const SyncConfig& cfg);

    // Request cancellation (best-effort; in-progress downloads finish).
    void RequestStop();

private:
    void RunSync(SyncConfig cfg);

    wxEvtHandler*     m_sink;
    std::thread       m_thread;
    std::atomic<bool> m_running{false};
    std::atomic<bool> m_stopRequested{false};
};
