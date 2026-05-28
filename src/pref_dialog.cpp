#include "pref_dialog.h"
#include "enc_managed_pi.h"

static const wxString kScheduleLabels[] = {
    "Every hour",
    "Every 6 hours",
    "Every 12 hours",
    "Daily",
    "Weekly"
};

wxBEGIN_EVENT_TABLE(PrefDialog, wxDialog)
    EVT_CHECKBOX(wxID_ANY, PrefDialog::OnAutoRefreshToggle)
    EVT_BUTTON(wxID_APPLY, PrefDialog::OnSyncNow)
wxEND_EVENT_TABLE()

PrefDialog::PrefDialog(wxWindow* parent, const PluginSettings& s, EncManagedPlugin* plugin)
    : wxDialog(parent, wxID_ANY, "EncManaged Preferences",
               wxDefaultPosition, wxDefaultSize,
               wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
    , m_plugin(plugin)
{
    auto* main = new wxBoxSizer(wxVERTICAL);
    auto* grid = new wxFlexGridSizer(2, 10, 20);
    grid->AddGrowableCol(1, 1);

    // Chart path
    grid->Add(new wxStaticText(this, wxID_ANY, "Chart path:"),
              0, wxALIGN_CENTER_VERTICAL);
    m_dirPicker = new wxDirPickerCtrl(this, wxID_ANY, s.chartPath,
                                      "Select chart directory",
                                      wxDefaultPosition, wxDefaultSize,
                                      wxDIRP_USE_TEXTCTRL | wxDIRP_DIR_MUST_EXIST);
    grid->Add(m_dirPicker, 1, wxEXPAND);

    // Catalog URL
    grid->Add(new wxStaticText(this, wxID_ANY, "Catalog URL:"),
              0, wxALIGN_CENTER_VERTICAL);
    m_catalogUrl = new wxTextCtrl(this, wxID_ANY, s.catalogURL);
    grid->Add(m_catalogUrl, 1, wxEXPAND);

    // Auto-refresh
    grid->Add(new wxStaticText(this, wxID_ANY, "Auto-refresh:"),
              0, wxALIGN_CENTER_VERTICAL);
    m_autoRefreshCb = new wxCheckBox(this, wxID_ANY, "");
    m_autoRefreshCb->SetValue(s.autoRefresh);
    grid->Add(m_autoRefreshCb, 0);

    // Schedule
    grid->Add(new wxStaticText(this, wxID_ANY, "Schedule:"),
              0, wxALIGN_CENTER_VERTICAL);
    wxArrayString labels;
    for (auto& l : kScheduleLabels) labels.Add(l);
    m_scheduleChoice = new wxChoice(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, labels);

    // Select matching schedule index
    int sel = 3;  // default: Daily
    for (int i = 0; i < kScheduleCount; ++i) {
        if (kScheduleValues[i] == s.scheduleHours) { sel = i; break; }
    }
    m_scheduleChoice->SetSelection(sel);
    m_scheduleChoice->Enable(s.autoRefresh);
    grid->Add(m_scheduleChoice, 0);

    // Workers
    grid->Add(new wxStaticText(this, wxID_ANY, "Workers:"),
              0, wxALIGN_CENTER_VERTICAL);
    m_workersSpin = new wxSpinCtrl(this, wxID_ANY, wxEmptyString,
                                   wxDefaultPosition, wxDefaultSize,
                                   wxSP_ARROW_KEYS, 1, 16, s.workers);
    grid->Add(m_workersSpin, 0);

    // Status
    grid->Add(new wxStaticText(this, wxID_ANY, "Last sync:"),
              0, wxALIGN_TOP);
    m_statusText = new wxTextCtrl(this, wxID_ANY, s.lastSyncStatus,
                                  wxDefaultPosition, wxDefaultSize,
                                  wxTE_READONLY | wxTE_MULTILINE);
    m_statusText->SetMinSize(wxSize(-1, 40));
    grid->Add(m_statusText, 1, wxEXPAND);

    main->Add(grid, 0, wxALL | wxEXPAND, 10);

    // Log
    main->Add(new wxStaticText(this, wxID_ANY, "Sync log:"), 0, wxLEFT | wxRIGHT, 10);
    m_logText = new wxTextCtrl(this, wxID_ANY, s.syncLog,
                               wxDefaultPosition, wxSize(-1, 150),
                               wxTE_READONLY | wxTE_MULTILINE | wxTE_DONTWRAP);
    main->Add(m_logText, 1, wxALL | wxEXPAND, 10);

    // Button row: [Sync Now] on left, [OK][Cancel] on right
    auto* btnSizer = new wxBoxSizer(wxHORIZONTAL);
    m_syncNowBtn = new wxButton(this, wxID_APPLY, "Sync Now");
    m_syncNowBtn->Enable(!s.syncRunning);
    btnSizer->Add(m_syncNowBtn, 0, wxRIGHT, 5);
    btnSizer->AddStretchSpacer();
    btnSizer->Add(new wxButton(this, wxID_OK,     "OK"),     0, wxRIGHT, 5);
    btnSizer->Add(new wxButton(this, wxID_CANCEL, "Cancel"), 0);

    main->Add(btnSizer, 0, wxALL | wxEXPAND, 10);

    SetSizerAndFit(main);
    SetMinSize(wxSize(480, -1));
}

PluginSettings PrefDialog::GetSettings() const {
    PluginSettings s;
    s.chartPath     = m_dirPicker->GetPath();
    s.catalogURL    = m_catalogUrl->GetValue();
    s.autoRefresh   = m_autoRefreshCb->GetValue();
    int idx         = m_scheduleChoice->GetSelection();
    s.scheduleHours = (idx >= 0 && idx < kScheduleCount)
                          ? kScheduleValues[idx] : 24;
    s.workers       = m_workersSpin->GetValue();
    return s;
}

void PrefDialog::OnAutoRefreshToggle(wxCommandEvent&) {
    m_scheduleChoice->Enable(m_autoRefreshCb->GetValue());
}

void PrefDialog::OnSyncNow(wxCommandEvent&) {
    // Save current settings to plugin before triggering sync
    if (m_plugin) {
        EndModal(wxID_APPLY);
    }
}
