#pragma once
#include "ocpn_plugin.h"
#include "sync_engine.h"
#include <wx/timer.h>

class PrefDialog;

// opencpn_plugin_118 does not inherit wxEvtHandler; we add it here so
// we can own the event table, wxTimer, and wxQueueEvent sink directly.
class EncManagedPlugin : public opencpn_plugin_118, public wxEvtHandler {
public:
    explicit EncManagedPlugin(void* ppimgr);
    ~EncManagedPlugin() override;

    int  Init() override;
    bool DeInit() override;

    int      GetAPIVersionMajor()    override { return 1; }
    int      GetAPIVersionMinor()    override { return 18; }
    int      GetPlugInVersionMajor() override { return 0; }
    int      GetPlugInVersionMinor() override { return 1; }

    wxString GetCommonName()       override { return "EncManaged"; }
    wxString GetShortDescription() override;
    wxString GetLongDescription()  override;
    wxBitmap* GetPlugInBitmap()    override;

    void ShowPreferencesDialog(wxWindow* parent) override;

    // Called by PrefDialog to trigger an immediate sync
    void TriggerSync() { StartSync(); }

private:
    void LoadSettings();
    void SaveSettings();
    void ArmTimer();
    void StartSync();

    void OnSyncProgress(wxCommandEvent& ev);
    void OnSyncComplete(wxCommandEvent& ev);
    void OnTimer(wxTimerEvent& ev);

    wxTimer     m_timer;
    SyncEngine* m_engine{nullptr};
    wxBitmap    m_bitmap;

    wxString m_chartPath;
    wxString m_catalogURL  {"https://www.charts.noaa.gov/ENCs/ENCProdCat.xml"};
    bool     m_autoRefresh {false};
    int      m_scheduleHours{24};
    int      m_workers      {4};
    long     m_lastSyncEpoch{0};
    wxString m_lastSyncStatus;
    wxString m_syncLog;

    wxDECLARE_EVENT_TABLE();
};

extern "C" {
    DECL_EXP opencpn_plugin* create_pi(void* ppimgr);
    DECL_EXP void destroy_pi(opencpn_plugin* p);
}
