#pragma once
#include <wx/dialog.h>
#include <wx/filepicker.h>
#include <wx/checkbox.h>
#include <wx/choice.h>
#include <wx/spinctrl.h>
#include <wx/textctrl.h>
#include <wx/stattext.h>
#include <wx/button.h>
#include <wx/sizer.h>

class EncManagedPlugin;

struct PluginSettings {
    wxString chartPath;
    wxString catalogURL;
    bool     autoRefresh{false};
    int      scheduleHours{24};
    int      workers{4};
    wxString lastSyncStatus;
    wxString syncLog;
    bool     syncRunning{false};
};

class PrefDialog : public wxDialog {
public:
    PrefDialog(wxWindow* parent, const PluginSettings& s, EncManagedPlugin* plugin);
    PluginSettings GetSettings() const;

private:
    void OnAutoRefreshToggle(wxCommandEvent&);
    void OnSyncNow(wxCommandEvent&);

    wxDirPickerCtrl* m_dirPicker;
    wxTextCtrl*      m_catalogUrl;
    wxCheckBox*      m_autoRefreshCb;
    wxChoice*        m_scheduleChoice;
    wxSpinCtrl*      m_workersSpin;
    wxTextCtrl*      m_statusText;
    wxTextCtrl*      m_logText;
    wxButton*        m_syncNowBtn;

    EncManagedPlugin* m_plugin;

    static constexpr int kScheduleValues[] = {1, 6, 12, 24, 168};
    static constexpr int kScheduleCount    = 5;

    wxDECLARE_EVENT_TABLE();
};
