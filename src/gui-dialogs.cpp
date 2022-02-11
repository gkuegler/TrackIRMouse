/**
 * GUI components for pop-up dialogues.
 * - Settings dialog pop-up
 *
 * --License Boilerplate Placeholder--
 */

#include "gui-dialogs.hpp"

#include <wx/font.h>

#include "config.hpp"
#include "gui-control-id.hpp"

// clang-format off
BEGIN_EVENT_TABLE(cSettingsGeneralPanel, wxPanel)
    EVT_CHECKBOX(myID_WATCHDOG_ENABLED, cSettingsGeneralPanel::OnEnabledWatchdog)
    EVT_CHECKBOX(myID_TRACK_ON_START, cSettingsGeneralPanel::OnTrackOnStart)
    EVT_CHECKBOX(myID_QUIT_ON_LOSS_OF_TRACK_IR, cSettingsGeneralPanel::OnQuitOnLossOfTrackIr)
END_EVENT_TABLE()

BEGIN_EVENT_TABLE(cSettingsAdvancedlPanel, wxPanel)
    EVT_TEXT(myID_TRACK_IR_DLL_PATH, cSettingsAdvancedlPanel::OnTrackIrDllPath)
END_EVENT_TABLE()
// clang-format on

cSettingsPopup::cSettingsPopup(wxWindow* parent, SData* pUserData)
    : wxPropertySheetDialog(parent, wxID_ANY, "Settings", wxPoint(200, 200),
                            wxSize(300, 300), wxDEFAULT_DIALOG_STYLE, "") {
  CreateButtons(wxOK | wxCANCEL);
  CreateBookCtrl();

  m_pnlGen = new cSettingsGeneralPanel(GetBookCtrl(), pUserData);
  m_pnlAdv = new cSettingsAdvancedlPanel(GetBookCtrl(), pUserData);
  GetBookCtrl()->AddPage(m_pnlGen, "General");
  GetBookCtrl()->AddPage(m_pnlAdv, "Advanced");

  LayoutDialog();
}

cSettingsGeneralPanel::cSettingsGeneralPanel(wxWindow* parent, SData* pUserData)
    : wxPanel(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize,
              wxTAB_TRAVERSAL, "") {
  m_pUserData = pUserData;
  m_cbxEnableWatchdog = new wxCheckBox(
      this, myID_WATCHDOG_ENABLED, "Watchdog Enabled", wxDefaultPosition,
      wxDefaultSize, wxCHK_2STATE, wxDefaultValidator, "");
  m_cbxTrackOnStart = new wxCheckBox(
      this, myID_TRACK_ON_START, "Track On Start", wxDefaultPosition,
      wxDefaultSize, wxCHK_2STATE, wxDefaultValidator, "");
  m_cbxQuitOnLossOfTrackIR = new wxCheckBox(
      this, myID_QUIT_ON_LOSS_OF_TRACK_IR, "Quit On Loss Of Track IR",
      wxDefaultPosition, wxDefaultSize, wxCHK_2STATE, wxDefaultValidator, "");

  m_cbxEnableWatchdog->SetValue(pUserData->watchdogEnabled);
  m_cbxTrackOnStart->SetValue(pUserData->trackOnStart);
  m_cbxQuitOnLossOfTrackIR->SetValue(pUserData->quitOnLossOfTrackIr);

  wxBoxSizer* topSizer = new wxBoxSizer(wxVERTICAL);
  topSizer->Add(m_cbxEnableWatchdog, 0, wxALL, 0);
  topSizer->Add(m_cbxTrackOnStart, 0, wxALL, 0);
  topSizer->Add(m_cbxQuitOnLossOfTrackIR, 0, wxALL, 0);

  wxBoxSizer* border = new wxBoxSizer(wxVERTICAL);
  border->Add(topSizer, 0, wxALL, 10);

  SetSizer(border);
}

void cSettingsGeneralPanel::OnEnabledWatchdog(wxCommandEvent& event) {
  m_pUserData->watchdogEnabled = m_cbxEnableWatchdog->IsChecked();
}

void cSettingsGeneralPanel::OnTrackOnStart(wxCommandEvent& event) {
  m_pUserData->trackOnStart = m_cbxTrackOnStart->IsChecked();
}

void cSettingsGeneralPanel::OnQuitOnLossOfTrackIr(wxCommandEvent& event) {
  m_pUserData->quitOnLossOfTrackIr = m_cbxQuitOnLossOfTrackIR->IsChecked();
}

cSettingsAdvancedlPanel::cSettingsAdvancedlPanel(wxWindow* parent,
                                                 SData* pUserData)
    : wxPanel(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize,
              wxTAB_TRAVERSAL, "") {
  m_pUserData = pUserData;
  wxStaticText* txtTrackLocation1 =
      new wxStaticText(this, wxID_ANY, "Path of 'NPClient64.dll':   ");
  m_txtTrackIrDllPath =
      new wxTextCtrl(this, myID_TRACK_IR_DLL_PATH, pUserData->trackIrDllFolder,
                     wxDefaultPosition, wxSize(300, 20), wxTE_LEFT);
  wxStaticText* txtTrackLocation2 = new wxStaticText(
      this, wxID_ANY,
      "Note: a value of 'default' will get from install location.");

  wxBoxSizer* zrDllLocation = new wxBoxSizer(wxHORIZONTAL);
  zrDllLocation->Add(txtTrackLocation1, 0, wxTOP, 5);
  zrDllLocation->Add(m_txtTrackIrDllPath, 1, wxALL | wxEXPAND, 0);

  wxBoxSizer* topSizer = new wxBoxSizer(wxVERTICAL);
  topSizer->Add(zrDllLocation, 0, wxTOP | wxEXPAND, 10);
  topSizer->Add(txtTrackLocation2, 0, wxALL, 0);

  wxBoxSizer* border = new wxBoxSizer(wxVERTICAL);
  border->Add(topSizer, 0, wxALL, 10);

  SetSizer(border);
}

void cSettingsAdvancedlPanel::OnTrackIrDllPath(wxCommandEvent& event) {
  wxString wxsPath = m_txtTrackIrDllPath->GetLineText(0);
  std::string path(wxsPath.mb_str());
  m_pUserData->trackIrDllFolder = path;
}
