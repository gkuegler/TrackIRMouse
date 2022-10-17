/**
 * GUI components for pop-up dialogues.
 * - Settings dialog_ pop-up
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

// const static std::array<std::string, 7> k_log_levels = {
//   "trace", "debug", "info", "warning", "error", "critical", "off"
// };
const static auto log_levels_arr =
  util::BuildWxArrayString(std::array<std::string, 7>{ "trace",
                                                       "debug",
                                                       "info",
                                                       "warning",
                                                       "error",
                                                       "critical",
                                                       "off" });

cSettingsPopup::cSettingsPopup(wxWindow* parent, config::UserData* pUserData)
  : wxPropertySheetDialog(parent,
                          wxID_ANY,
                          "Settings",
                          wxPoint(200, 200),
                          wxSize(300, 300),
                          wxDEFAULT_DIALOG_STYLE,
                          "")
{
  CreateButtons(wxOK | wxCANCEL);
  CreateBookCtrl();

  auto* general = new SettingsGeneralPanel(GetBookCtrl(), pUserData);
  auto* advanced = new SettingsAdvancedPanel(GetBookCtrl(), pUserData);
  auto* hotkey = new cSettingsHotkeyPanel(GetBookCtrl(), pUserData);
  GetBookCtrl()->AddPage(general, "General");
  GetBookCtrl()->AddPage(advanced, "Advanced");
  GetBookCtrl()->AddPage(hotkey, "Hotkey");

  LayoutDialog();
}

SettingsGeneralPanel::SettingsGeneralPanel(wxWindow* parent,
                                           config::UserData* pUserData)
  : wxPanel(parent,
            wxID_ANY,
            wxDefaultPosition,
            wxDefaultSize,
            wxTAB_TRAVERSAL,
            "")
{

  p_user_data_ = pUserData;

  // clang-format off
  p_enable_watchdog_ = new wxCheckBox(this, wxID_ANY, "Pipe Server Enabled", wxDefaultPosition, wxDefaultSize, wxCHK_2STATE, wxDefaultValidator, "");
  p_track_on_start_ = new wxCheckBox(this, wxID_ANY, "Track On Start", wxDefaultPosition, wxDefaultSize, wxCHK_2STATE, wxDefaultValidator, "");
  p_quit_on_loss_of_track_ir = new wxCheckBox(this, wxID_ANY, "Quit On Loss Of Track IR", wxDefaultPosition, wxDefaultSize, wxCHK_2STATE, wxDefaultValidator, "");
  p_log_level_ = new wxChoice(this, wxID_ANY, wxDefaultPosition, wxSize(100, 25), log_levels_arr, 0, wxDefaultValidator, "");
  wxStaticText* txtLogLabel = new wxStaticText(this, wxID_ANY, "Log Level: ");
  // clang-format on

  p_enable_watchdog_->SetValue(pUserData->watchdog_enabled);
  p_track_on_start_->SetValue(pUserData->track_on_start);
  p_quit_on_loss_of_track_ir->SetValue(pUserData->quit_on_loss_of_trackir);
  p_log_level_->SetSelection(pUserData->log_level);

  wxBoxSizer* zrLogFile = new wxBoxSizer(wxHORIZONTAL);
  zrLogFile->Add(txtLogLabel, 0, wxALL | wxALIGN_CENTER_VERTICAL, 0);
  zrLogFile->Add(p_log_level_, 0, wxALL, 0);

  wxBoxSizer* topSizer = new wxBoxSizer(wxVERTICAL);
  topSizer->Add(p_enable_watchdog_, 0, wxALL, 0);
  topSizer->Add(p_track_on_start_, 0, wxALL, 0);
  topSizer->Add(p_quit_on_loss_of_track_ir, 0, wxALL, 0);
  topSizer->Add(zrLogFile, 0, wxALL, 0);

  wxBoxSizer* border = new wxBoxSizer(wxVERTICAL);
  border->Add(topSizer, 0, wxALL, 10);

  SetSizer(border);

  p_enable_watchdog_->Bind(
    wxEVT_CHECKBOX, &SettingsGeneralPanel::OnEnabledWatchdog, this);
  p_track_on_start_->Bind(
    wxEVT_CHECKBOX, &SettingsGeneralPanel::OnTrackOnStart, this);
  p_quit_on_loss_of_track_ir->Bind(
    wxEVT_CHECKBOX, &SettingsGeneralPanel::OnQuitOnLossOfTrackIr, this);
  p_log_level_->Bind(wxEVT_CHOICE, &SettingsGeneralPanel::OnLogLevel, this);
}

void
SettingsGeneralPanel::OnEnabledWatchdog(wxCommandEvent& event)
{
  p_user_data_->watchdog_enabled = p_enable_watchdog_->IsChecked();
}

void
SettingsGeneralPanel::OnTrackOnStart(wxCommandEvent& event)
{
  p_user_data_->track_on_start = p_track_on_start_->IsChecked();
}

void
SettingsGeneralPanel::OnQuitOnLossOfTrackIr(wxCommandEvent& event)
{
  p_user_data_->quit_on_loss_of_trackir =
    p_quit_on_loss_of_track_ir->IsChecked();
}

void
SettingsGeneralPanel::OnLogLevel(wxCommandEvent& event)
{
  auto selection = p_log_level_->GetSelection();
  p_user_data_->log_level = static_cast<spdlog::level::level_enum>(selection);
}

SettingsAdvancedPanel::SettingsAdvancedPanel(wxWindow* parent,
                                             config::UserData* pUserData)
  : wxPanel(parent,
            wxID_ANY,
            wxDefaultPosition,
            wxDefaultSize,
            wxTAB_TRAVERSAL,
            "")
{
  p_user_data_ = pUserData;

  p_auto_find_dll_ = new wxCheckBox(
    this,
    wxID_ANY,
    "Auto find trackir dll from registry.\n(This is normal operation if "
    "TrackIR is installed.\nUse of this feature is primarily aimed at users\n"
    "who wish to spoof the dll to provide\ntheir own head tracking info)");

  wxStaticText* txtTrackLocation1 =
    new wxStaticText(this, wxID_ANY, "Path of 'NPClient64.dll':   ");
  p_track_ir_dll_path = new wxTextCtrl(this,
                                       myID_TRACK_IR_DLL_PATH,
                                       pUserData->track_ir_dll_folder,
                                       wxDefaultPosition,
                                       wxSize(300, 20),
                                       wxTE_LEFT);
  wxStaticText* txtTrackLocation2 = new wxStaticText(
    this,
    wxID_ANY,
    "Note: a value of 'default' will get from install location.");

  p_auto_find_dll_->SetValue(pUserData->auto_find_track_ir_dll);

  wxBoxSizer* zrDllLocation = new wxBoxSizer(wxHORIZONTAL);
  zrDllLocation->Add(txtTrackLocation1, 0, wxTOP, 5);
  zrDllLocation->Add(p_track_ir_dll_path, 1, wxALL | wxEXPAND, 0);

  wxBoxSizer* topSizer = new wxBoxSizer(wxVERTICAL);
  topSizer->Add(p_auto_find_dll_, 0, wxTOP | wxBOTTOM, 10);
  topSizer->Add(zrDllLocation, 0, wxTOP | wxEXPAND, 10);
  topSizer->Add(txtTrackLocation2, 0, wxALL, 0);

  wxBoxSizer* border = new wxBoxSizer(wxVERTICAL);
  border->Add(topSizer, 0, wxALL, 10);

  SetSizer(border);

  p_track_ir_dll_path->Bind(
    wxEVT_TEXT, &SettingsAdvancedPanel::OnTrackIrDllPath, this);
  p_track_ir_dll_path->Bind(
    wxEVT_BUTTON, &SettingsAdvancedPanel::OnAutoFindDll, this);
}

void
SettingsAdvancedPanel::OnTrackIrDllPath(wxCommandEvent& event)
{
  wxString wxsPath = p_track_ir_dll_path->GetLineText(0);
  std::string path(wxsPath.mb_str());
  p_user_data_->track_ir_dll_folder = path;
}

void
SettingsAdvancedPanel::OnAutoFindDll(wxCommandEvent& event)
{
  p_user_data_->auto_find_track_ir_dll = p_auto_find_dll_->IsChecked();
}

cSettingsServerPanel::cSettingsServerPanel(wxWindow* parent,
                                           config::UserData* pUserData)
  : wxPanel(parent,
            wxID_ANY,
            wxDefaultPosition,
            wxDefaultSize,
            wxTAB_TRAVERSAL,
            "")
{
  p_user_data_ = pUserData;

  p_server_enabled_ = new wxCheckBox(
    this,
    wxID_ANY,
    "Control the application by sending commands through a pipe server.");

  wxStaticText* text_label =
    new wxStaticText(this, wxID_ANY, "Pipe server name: ");
  p_server_name_ = new wxTextCtrl(this,
                                  myID_TRACK_IR_DLL_PATH,
                                  pUserData->track_ir_dll_folder,
                                  wxDefaultPosition,
                                  wxSize(300, 20),
                                  wxTE_LEFT);
  // wxStaticText* txtTrackLocation2 = new wxStaticText(
  //     this, wxID_ANY,
  //     "Note: a value of 'default' will get from install location.");

  p_server_enabled_->SetValue(pUserData->watchdog_enabled);

  wxBoxSizer* zrTextEntry = new wxBoxSizer(wxHORIZONTAL);
  zrTextEntry->Add(text_label, 0, wxTOP, 5);
  zrTextEntry->Add(p_server_enabled_, 1, wxALL | wxEXPAND, 0);

  wxBoxSizer* topSizer = new wxBoxSizer(wxVERTICAL);
  topSizer->Add(p_server_enabled_, 0, wxTOP | wxBOTTOM, 10);
  topSizer->Add(zrTextEntry, 0, wxTOP | wxEXPAND, 10);

  wxBoxSizer* border = new wxBoxSizer(wxVERTICAL);
  border->Add(topSizer, 0, wxALL, 10);

  SetSizer(border);

  p_server_enabled_->Bind(
    wxEVT_CHECKBOX, &cSettingsServerPanel::OnServerEnabled, this);
  p_server_name_->Bind(wxEVT_TEXT, &cSettingsServerPanel::OnServerName, this);
}

void
cSettingsServerPanel::OnServerEnabled(wxCommandEvent&)
{
  p_user_data_->watchdog_enabled = p_server_enabled_->IsChecked();
}

void
cSettingsServerPanel::OnServerName(wxCommandEvent&)
{
  wxString line_text = p_server_name_->GetLineText(0);
  p_user_data_->pipe_server_name = line_text.mb_str();
}

cSettingsHotkeyPanel::cSettingsHotkeyPanel(wxWindow* parent,
                                           config::UserData* pUserData)
  : wxPanel(parent,
            wxID_ANY,
            wxDefaultPosition,
            wxDefaultSize,
            wxTAB_TRAVERSAL,
            "")
{
  p_user_data_ = pUserData;
  p_hotkey_enabled_ =
    new wxCheckBox(this,
                   wxID_ANY,
                   "Enable alternate mouse modes with hotkey: F18",
                   wxDefaultPosition,
                   wxDefaultSize,
                   wxCHK_2STATE,
                   wxDefaultValidator,
                   "");

  p_hotkey_enabled_->SetValue(pUserData->hotkey_enabled);

  wxBoxSizer* topSizer = new wxBoxSizer(wxVERTICAL);
  topSizer->Add(p_hotkey_enabled_, 0, wxALL, 0);

  wxBoxSizer* border = new wxBoxSizer(wxVERTICAL);
  border->Add(topSizer, 0, wxALL, 10);

  SetSizer(border);

  p_hotkey_enabled_->Bind(
    wxEVT_CHECKBOX, &cSettingsHotkeyPanel::OnHotkeyEnable, this);
}

void
cSettingsHotkeyPanel::OnHotkeyEnable(wxCommandEvent& event)
{
  p_user_data_->hotkey_enabled = p_hotkey_enabled_->IsChecked();
}
