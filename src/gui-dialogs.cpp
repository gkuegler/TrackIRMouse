/**
 * GUI components for pop-up dialogues.
 * - Settings dialog pop-up
 *
 * --License Boilerplate Placeholder--
 */

#include "gui-dialogs.hpp"

#include <assert.h>
#include <spdlog/common.h>
#include <wx/bookctrl.h>

#include "config.hpp"
#include "gui-control-id.hpp"
#include "log.hpp"
#include "util.hpp"

const static std::array<std::string, 7> LogLevels = {
    "trace", "debug", "info", "warning", "error", "critical", "off"};
const static auto asLogLevels = util::BuildWxArrayString(LogLevels);

cSettingsPopup::cSettingsPopup(wxWindow* parent, config::UserData* pUserData)
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

cSettingsGeneralPanel::cSettingsGeneralPanel(wxWindow* parent,
                                             config::UserData* pUserData)
    : wxPanel(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize,
              wxTAB_TRAVERSAL, "") {
  m_pUserData = pUserData;
  m_cbxEnableWatchdog =
      new wxCheckBox(this, wxID_ANY, "Pipe Server Enabled", wxDefaultPosition,
                     wxDefaultSize, wxCHK_2STATE, wxDefaultValidator, "");
  m_cbxTrackOnStart =
      new wxCheckBox(this, wxID_ANY, "Track On Start", wxDefaultPosition,
                     wxDefaultSize, wxCHK_2STATE, wxDefaultValidator, "");
  m_cbxQuitOnLossOfTrackIR = new wxCheckBox(
      this, wxID_ANY, "Quit On Loss Of Track IR", wxDefaultPosition,
      wxDefaultSize, wxCHK_2STATE, wxDefaultValidator, "");
  wxStaticText* txtLogLabel = new wxStaticText(this, wxID_ANY, "Log Level: ");
  m_cmbLogLevel =
      new wxChoice(this, wxID_ANY, wxDefaultPosition, wxSize(100, 25),
                   asLogLevels, 0, wxDefaultValidator, "");

  m_cbxEnableWatchdog->SetValue(pUserData->watchdogEnabled);
  m_cbxTrackOnStart->SetValue(pUserData->trackOnStart);
  m_cbxQuitOnLossOfTrackIR->SetValue(pUserData->quitOnLossOfTrackIr);
  m_cmbLogLevel->SetSelection(pUserData->logLevel);

  wxBoxSizer* zrLogFile = new wxBoxSizer(wxHORIZONTAL);
  zrLogFile->Add(txtLogLabel, 0, wxALL | wxALIGN_CENTER_VERTICAL, 0);
  zrLogFile->Add(m_cmbLogLevel, 0, wxALL, 0);

  wxBoxSizer* topSizer = new wxBoxSizer(wxVERTICAL);
  topSizer->Add(m_cbxEnableWatchdog, 0, wxALL, 0);
  topSizer->Add(m_cbxTrackOnStart, 0, wxALL, 0);
  topSizer->Add(m_cbxQuitOnLossOfTrackIR, 0, wxALL, 0);
  topSizer->Add(zrLogFile, 0, wxALL, 0);

  wxBoxSizer* border = new wxBoxSizer(wxVERTICAL);
  border->Add(topSizer, 0, wxALL, 10);

  SetSizer(border);

  m_cbxEnableWatchdog->Bind(wxEVT_CHECKBOX,
                            &cSettingsGeneralPanel::OnEnabledWatchdog, this);
  m_cbxTrackOnStart->Bind(wxEVT_CHECKBOX,
                          &cSettingsGeneralPanel::OnTrackOnStart, this);
  m_cbxQuitOnLossOfTrackIR->Bind(
      wxEVT_CHECKBOX, &cSettingsGeneralPanel::OnQuitOnLossOfTrackIr, this);
  m_cmbLogLevel->Bind(wxEVT_CHOICE, &cSettingsGeneralPanel::OnLogLevel, this);
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

void cSettingsGeneralPanel::OnLogLevel(wxCommandEvent& event) {
  auto selection = m_cmbLogLevel->GetSelection();
  m_pUserData->logLevel = static_cast<spdlog::level::level_enum>(selection);
}

cSettingsAdvancedlPanel::cSettingsAdvancedlPanel(wxWindow* parent,
                                                 config::UserData* pUserData)
    : wxPanel(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize,
              wxTAB_TRAVERSAL, "") {
  m_pUserData = pUserData;

  m_cbxAutoFindDll = new wxCheckBox(
      this, wxID_ANY,
      "Auto find trackir dll from registry.\n(This is normal operation if "
      "TrackIR is installed.\nUse of this feature is primarily aimed at users\n"
      "who wish to spoof the dll to provide\ntheir own head tracking info)");

  wxStaticText* txtTrackLocation1 =
      new wxStaticText(this, wxID_ANY, "Path of 'NPClient64.dll':   ");
  m_txtTrackIrDllPath =
      new wxTextCtrl(this, myID_TRACK_IR_DLL_PATH, pUserData->trackIrDllFolder,
                     wxDefaultPosition, wxSize(300, 20), wxTE_LEFT);
  wxStaticText* txtTrackLocation2 = new wxStaticText(
      this, wxID_ANY,
      "Note: a value of 'default' will get from install location.");

  m_cbxAutoFindDll->SetValue(pUserData->autoFindTrackIrDll);

  wxBoxSizer* zrDllLocation = new wxBoxSizer(wxHORIZONTAL);
  zrDllLocation->Add(txtTrackLocation1, 0, wxTOP, 5);
  zrDllLocation->Add(m_txtTrackIrDllPath, 1, wxALL | wxEXPAND, 0);

  wxBoxSizer* topSizer = new wxBoxSizer(wxVERTICAL);
  topSizer->Add(m_cbxAutoFindDll, 0, wxTOP | wxBOTTOM, 10);
  topSizer->Add(zrDllLocation, 0, wxTOP | wxEXPAND, 10);
  topSizer->Add(txtTrackLocation2, 0, wxALL, 0);

  wxBoxSizer* border = new wxBoxSizer(wxVERTICAL);
  border->Add(topSizer, 0, wxALL, 10);

  SetSizer(border);

  m_txtTrackIrDllPath->Bind(wxEVT_TEXT,
                            &cSettingsAdvancedlPanel::OnTrackIrDllPath, this);
  m_txtTrackIrDllPath->Bind(wxEVT_BUTTON,
                            &cSettingsAdvancedlPanel::OnAutoFindDll, this);
}

void cSettingsAdvancedlPanel::OnTrackIrDllPath(wxCommandEvent& event) {
  wxString wxsPath = m_txtTrackIrDllPath->GetLineText(0);
  std::string path(wxsPath.mb_str());
  m_pUserData->trackIrDllFolder = path;
}

void cSettingsAdvancedlPanel::OnAutoFindDll(wxCommandEvent& event) {
  m_pUserData->autoFindTrackIrDll = m_cbxAutoFindDll->IsChecked();
}

cSettingsServerlPanel::cSettingsServerlPanel(wxWindow* parent,
                                             config::UserData* pUserData)
    : wxPanel(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize,
              wxTAB_TRAVERSAL, "") {
  m_pUserData = pUserData;

  m_cbxServerEnabled = new wxCheckBox(
      this, wxID_ANY,
      "Control the application by sending commands through a pipe server.");

  wxStaticText* text_label =
      new wxStaticText(this, wxID_ANY, "Pipe server name: ");
  m_txtServerName =
      new wxTextCtrl(this, myID_TRACK_IR_DLL_PATH, pUserData->trackIrDllFolder,
                     wxDefaultPosition, wxSize(300, 20), wxTE_LEFT);
  // wxStaticText* txtTrackLocation2 = new wxStaticText(
  //     this, wxID_ANY,
  //     "Note: a value of 'default' will get from install location.");

  m_cbxServerEnabled->SetValue(pUserData->watchdogEnabled);

  wxBoxSizer* zrTextEntry = new wxBoxSizer(wxHORIZONTAL);
  zrTextEntry->Add(text_label, 0, wxTOP, 5);
  zrTextEntry->Add(m_cbxServerEnabled, 1, wxALL | wxEXPAND, 0);

  wxBoxSizer* topSizer = new wxBoxSizer(wxVERTICAL);
  topSizer->Add(m_cbxServerEnabled, 0, wxTOP | wxBOTTOM, 10);
  topSizer->Add(zrTextEntry, 0, wxTOP | wxEXPAND, 10);

  wxBoxSizer* border = new wxBoxSizer(wxVERTICAL);
  border->Add(topSizer, 0, wxALL, 10);

  SetSizer(border);

  m_cbxServerEnabled->Bind(wxEVT_CHECKBOX,
                           &cSettingsServerlPanel::OnServerEnabled, this);
  m_txtServerName->Bind(wxEVT_TEXT, &cSettingsServerlPanel::OnServerName, this);
}

void cSettingsServerlPanel::OnServerEnabled(wxCommandEvent&) {
  m_pUserData->watchdogEnabled = m_cbxServerEnabled->IsChecked();
}

void cSettingsServerlPanel::OnServerName(wxCommandEvent&) {
  wxString line_text = m_txtServerName->GetLineText(0);
  m_pUserData->pipeServerName = line_text.mb_str();
}
