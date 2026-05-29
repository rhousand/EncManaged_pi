#include "enc_managed_pi.h"
#include "pref_dialog.h"
#include <wx/fileconf.h>
#include <wx/msgdlg.h>
#include <filesystem>
#include <ctime>

#define CONFIG_PREFIX "/PlugIns/EncManaged"

namespace fs = std::filesystem;

wxBEGIN_EVENT_TABLE(EncManagedPlugin, wxEvtHandler)
    EVT_TIMER(wxID_ANY,                           EncManagedPlugin::OnTimer)
    EVT_COMMAND(wxID_ANY, EVT_SYNC_PROGRESS,      EncManagedPlugin::OnSyncProgress)
    EVT_COMMAND(wxID_ANY, EVT_SYNC_COMPLETE,      EncManagedPlugin::OnSyncComplete)
wxEND_EVENT_TABLE()

EncManagedPlugin::EncManagedPlugin(void* ppimgr)
    : opencpn_plugin_118(ppimgr), m_timer(this) {}

EncManagedPlugin::~EncManagedPlugin() {
    m_timer.Stop();
    delete m_engine;
    m_engine = nullptr;
}

int EncManagedPlugin::Init() {
    LoadSettings();
    m_engine = new SyncEngine(this);
    if (m_autoRefresh) ArmTimer();
    return WANTS_PREFERENCES;
}

bool EncManagedPlugin::DeInit() {
    m_timer.Stop();
    if (m_engine) {
        m_engine->RequestStop();
        delete m_engine;
        m_engine = nullptr;
    }
    return true;
}

wxString EncManagedPlugin::GetShortDescription() {
    return "Automated NOAA ENC chart sync";
}

wxString EncManagedPlugin::GetLongDescription() {
    return "EncManaged syncs NOAA Electronic Navigational Charts automatically.\n"
           "Configure chart path, schedule, and worker count in the preferences dialog.";
}

wxBitmap* EncManagedPlugin::GetPlugInBitmap() {
    return &m_bitmap;
}

void EncManagedPlugin::LoadSettings() {
    wxFileConfig* cfg = GetOCPNConfigObject();
    if (!cfg) return;

    cfg->SetPath(CONFIG_PREFIX);
    cfg->Read("ChartPath",      &m_chartPath,      wxGetHomeDir() + "/Documents/Charts/MANAGED_ENC/US");
    cfg->Read("CatalogURL",     &m_catalogURL,     "https://www.charts.noaa.gov/ENCs/ENCProdCat.xml");
    cfg->Read("AutoRefresh",    &m_autoRefresh,    false);
    cfg->Read("ScheduleHours",  &m_scheduleHours,  24);
    cfg->Read("Workers",        &m_workers,        4);
    cfg->Read("LastSyncEpoch",  &m_lastSyncEpoch,  0L);
    cfg->Read("LastSyncStatus", &m_lastSyncStatus, "");
    cfg->SetPath("/");
}

void EncManagedPlugin::SaveSettings() {
    wxFileConfig* cfg = GetOCPNConfigObject();
    if (!cfg) return;

    cfg->SetPath(CONFIG_PREFIX);
    cfg->Write("ChartPath",      m_chartPath);
    cfg->Write("CatalogURL",     m_catalogURL);
    cfg->Write("AutoRefresh",    m_autoRefresh);
    cfg->Write("ScheduleHours",  m_scheduleHours);
    cfg->Write("Workers",        m_workers);
    cfg->Write("LastSyncEpoch",  m_lastSyncEpoch);
    cfg->Write("LastSyncStatus", m_lastSyncStatus);
    cfg->SetPath("/");
    cfg->Flush();
}

void EncManagedPlugin::ArmTimer() {
    m_timer.Stop();
    if (!m_autoRefresh) return;

    long intervalSec = (long)m_scheduleHours * 3600L;
    long now         = (long)std::time(nullptr);
    long delayMs;

    if (m_lastSyncEpoch > 0) {
        long elapsed   = now - m_lastSyncEpoch;
        long remaining = intervalSec - elapsed;
        delayMs = (remaining > 0) ? remaining * 1000L : 1000L;
    } else {
        delayMs = intervalSec * 1000L;
    }
    m_timer.StartOnce((int)delayMs);
}

void EncManagedPlugin::OnTimer(wxTimerEvent&) {
    StartSync();
}

void EncManagedPlugin::StartSync() {
    if (!m_engine || m_engine->IsRunning()) return;

    SyncConfig cfg;
    cfg.chartPath  = m_chartPath.ToStdString();
    cfg.catalogURL = m_catalogURL.ToStdString();
    cfg.workers    = m_workers;

    fs::create_directories(cfg.chartPath);
    m_syncLog.Clear();
    m_engine->Start(cfg);
}

void EncManagedPlugin::OnSyncProgress(wxCommandEvent& ev) {
    m_syncLog += ev.GetString() + "\n";
}

void EncManagedPlugin::OnSyncComplete(wxCommandEvent& ev) {
    bool ok = (ev.GetInt() == 1);
    m_lastSyncStatus = ev.GetString();
    if (ok) m_lastSyncEpoch = (long)std::time(nullptr);
    SaveSettings();
    ArmTimer();

    if (ok) {
        wxWindow* parent = GetOCPNCanvasWindow();
        OCPNMessageBox_PlugIn(parent,
            "EncManaged: Charts updated.\n"
            "Use Tools → Charts → Scan for new/updated charts to load them.",
            "EncManaged Sync Complete",
            wxOK | wxICON_INFORMATION);
    }
}

void EncManagedPlugin::ShowPreferencesDialog(wxWindow* parent) {
    PluginSettings s;
    s.chartPath      = m_chartPath;
    s.catalogURL     = m_catalogURL;
    s.autoRefresh    = m_autoRefresh;
    s.scheduleHours  = m_scheduleHours;
    s.workers        = m_workers;
    if (m_lastSyncEpoch > 0) {
        std::time_t t = (std::time_t)m_lastSyncEpoch;
        char buf[32];
        std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M", std::localtime(&t));
        wxString detail = m_lastSyncStatus.IsEmpty() ? "(no sync yet)" : m_lastSyncStatus;
        s.lastSyncStatus = wxString(buf) + "\n" + detail;
    } else {
        s.lastSyncStatus = m_lastSyncStatus.IsEmpty() ? "(no sync yet)" : m_lastSyncStatus;
    }
    s.syncLog        = m_syncLog;
    s.syncRunning    = m_engine && m_engine->IsRunning();

    PrefDialog dlg(parent, s, this);
    int ret = dlg.ShowModal();

    if (ret == wxID_OK || ret == wxID_APPLY) {
        PluginSettings out = dlg.GetSettings();
        m_chartPath      = out.chartPath;
        m_catalogURL     = out.catalogURL;
        m_autoRefresh    = out.autoRefresh;
        m_scheduleHours  = out.scheduleHours;
        m_workers        = out.workers;
        SaveSettings();
        ArmTimer();
        if (ret == wxID_APPLY) StartSync();
    }
}

opencpn_plugin* create_pi(void* ppimgr) { return new EncManagedPlugin(ppimgr); }
void destroy_pi(opencpn_plugin* p)       { delete p; }
