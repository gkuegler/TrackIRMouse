/**
 * Main app frame window and GUI components.
 *
 * --License Boilerplate Placeholder--
 *
 */

#include <array>
#include <string>

#include <wx/statline.h>
#include <wx/wx.h>

#include "config.hpp"
#include "utility.hpp"

class OkayCancelDialogueButtons : public wxPanel
{
public:
  wxWindow* parent;
  wxDialog* dialogue;

  OkayCancelDialogueButtons(wxWindow* parent_, wxDialog* dialogue_)
    : wxPanel(parent_)
  {
    parent = parent_;
    dialogue = dialogue_;
    auto okay =
      new wxButton(this, wxID_ANY, "Apply", wxDefaultPosition, wxSize(110, 25));
    auto save = new wxButton(
      this, wxID_ANY, "Apply && Save", wxDefaultPosition, wxSize(110, 25));
    auto cancel = new wxButton(
      this, wxID_ANY, "Cancel", wxDefaultPosition, wxSize(110, 25));

    auto top = new wxBoxSizer(wxHORIZONTAL);
    top->Add(okay, 0, wxALL, 0);
    top->Add(save, 0, wxALL, 0);
    top->Add(cancel, 0, wxALL, 0);
    SetSizer(top);

    okay->Bind(wxEVT_BUTTON, &OkayCancelDialogueButtons::OnOkay, this);
    save->Bind(wxEVT_BUTTON, &OkayCancelDialogueButtons::OnApply, this);
    cancel->Bind(wxEVT_BUTTON, &OkayCancelDialogueButtons::OnCancel, this);
  }

  void OnOkay(wxCommandEvent& event) { dialogue->EndModal(wxID_OK); }
  void OnApply(wxCommandEvent& event) { dialogue->EndModal(wxID_APPLY); }
  void OnCancel(wxCommandEvent& event) { dialogue->EndModal(wxID_CANCEL); }
};

class SettingsFrame : public wxDialog
{
private:
  wxCheckBox* check_track_on_start;
  wxCheckBox* check_quit_on_loss;
  wxCheckBox* check_mouse_mode_hotkey;
  wxCheckBox* check_enable_pipe_server;
  wxChoice* choice_log_level;
  wxCheckBox* check_auto_find_dll;
  wxTextCtrl* text_dll_folder_location;

public:
  SettingsFrame(wxWindow* parent, const config::UserData& user_data);
  ~SettingsFrame(){};
  void ApplySettings(config::UserData& user_data);
};

SettingsFrame::SettingsFrame(wxWindow* parent,
                             const config::UserData& user_data)
  : wxDialog(parent, wxID_ANY, "Track IR - Settings")
{
  const wxSize k_default_button_size = wxSize(110, 25);
  const wxSize k_default_button_size_2 = wxSize(150, 25);
  const constexpr int k_max_profile_length = 30;
  const wxTextValidator alphanumeric_validator(wxFILTER_ALPHANUMERIC);

  // colors used for testing
  const wxColor yellow(255, 255, 0);
  const wxColor blue(255, 181, 102);
  const wxColor pink(198, 102, 255);
  const wxColor green(142, 255, 102);
  const wxColor orange(102, 201, 255);

  // Panels
  auto panel = new wxPanel(
    this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxBORDER_NONE);
  auto buttons = new OkayCancelDialogueButtons(panel, this);

  // Logging Window
  auto label_start_up_settings =
    new wxStaticText(panel, wxID_ANY, "Startup Behavior  ");
  auto label_basic_settings =
    new wxStaticText(panel, wxID_ANY, "Basic Settings  ");
  auto label_advanced_settings =
    new wxStaticText(panel, wxID_ANY, "Advanced Settings  ");

  auto static_line = new wxStaticLine(
    panel, wxID_ANY, wxDefaultPosition, wxSize(100, 2), wxLI_HORIZONTAL);
  auto static_line_2 = new wxStaticLine(
    panel, wxID_ANY, wxDefaultPosition, wxSize(100, 2), wxLI_HORIZONTAL);
  auto static_line_3 = new wxStaticLine(
    panel, wxID_ANY, wxDefaultPosition, wxSize(100, 2), wxLI_HORIZONTAL);

  check_track_on_start =
    new wxCheckBox(panel, wxID_ANY, "Start mouse control when app starts");

  check_quit_on_loss = new wxCheckBox(
    panel, wxID_ANY, "Quit app when connection is lost to NP TrackIR");

  check_mouse_mode_hotkey = new wxCheckBox(
    panel, wxID_ANY, "Enable alternate mouse modes with hotkey: F18");

  check_enable_pipe_server =
    new wxCheckBox(panel,
                   wxID_ANY,
                   "Enable commands through the named pipe server "
                   "'\\\\.\\pipe\\watchdog'");

  // ADVANCED SETTINGS

  const auto log_levels =
    utility::BuildWxArrayString(std::array<std::string, 7>{
      "trace", "debug", "info", "warning", "error", "critical", "off" });
  auto label_log_level = new wxStaticText(panel, wxID_ANY, "Log Level:  ");
  choice_log_level = new wxChoice(
    panel, wxID_ANY, wxDefaultPosition, wxSize(100, 25), log_levels, 0);

  check_auto_find_dll = new wxCheckBox(
    panel, wxID_ANY, "Look up 'NPClient64.dll' location from registry.");

  auto label_auto_find = new wxStaticText(
    panel,
    wxID_ANY,
    "Uncheck this option to manually specify the folder where 'NPClient64.dll' "
    "is installed.\n"
    "This option is default enabled when Natural Point TrackIR 5 is installed "
    "Normally.\n"
    "This feature is primarily aimed at users who wish to spoof the dll\n"
    "and provide their own data source.");

  auto label_dll_folder_location = new wxStaticText(
    panel, wxID_ANY, "Manual Folder Location of 'NPClient64.dll':");

  text_dll_folder_location = new wxTextCtrl(
    panel, wxID_ANY, "", wxDefaultPosition, wxSize(300, 20), wxTE_LEFT);

  // Set Defaults
  check_auto_find_dll->SetValue(user_data.auto_find_track_ir_dll);
  check_track_on_start->SetValue(user_data.track_on_start);
  check_quit_on_loss->SetValue(user_data.quit_on_loss_of_trackir);
  check_mouse_mode_hotkey->SetValue(user_data.hotkey_enabled);
  check_enable_pipe_server->SetValue(user_data.pipe_server_enabled);
  // user_data.log_level = choice_log_level->GetString();
  check_auto_find_dll->SetValue(user_data.auto_find_track_ir_dll);
  // user_data.auto_find_track_ir_dll = text_dll_folder_location->IsChecked();

  constexpr int SMALL_SPACE = 6;
  constexpr int BIG_SPACE = 12;
  constexpr int BORDER_SPACE = 12;

  // Static Line Titles
  auto label_sizer_1 = new wxBoxSizer(wxHORIZONTAL);
  label_sizer_1->Add(label_start_up_settings, 0, wxALIGN_CENTER_VERTICAL, 0);
  label_sizer_1->Add(static_line, 1, wxALIGN_CENTER_VERTICAL, 0);

  auto label_sizer_2 = new wxBoxSizer(wxHORIZONTAL);
  label_sizer_2->Add(label_basic_settings, 0, wxALIGN_CENTER_VERTICAL, 0);
  label_sizer_2->Add(static_line_2, 1, wxALIGN_CENTER_VERTICAL, 0);

  auto label_sizer_3 = new wxBoxSizer(wxHORIZONTAL);
  label_sizer_3->Add(
    label_advanced_settings, 0, wxALL | wxALIGN_CENTER_VERTICAL, 0);
  label_sizer_3->Add(static_line_3, 1, wxALL | wxALIGN_CENTER_VERTICAL, 0);

  // Horizontal Sub-Sizers
  auto log_levels_sizer = new wxBoxSizer(wxHORIZONTAL);
  log_levels_sizer->Add(
    label_log_level, 0, wxLEFT | wxALIGN_CENTER_VERTICAL, SMALL_SPACE);
  log_levels_sizer->Add(
    choice_log_level, 0, wxALL | wxALIGN_CENTER_VERTICAL, 0);

  // Main Layout
  auto top_sizer = new wxBoxSizer(wxVERTICAL);
  // start up settings
  top_sizer->Add(label_sizer_1, 0, wxEXPAND | wxBOTTOM, BIG_SPACE);
  top_sizer->Add(check_track_on_start, 0, wxLEFT | wxALIGN_LEFT, SMALL_SPACE);
  top_sizer->Add(
    check_quit_on_loss, 0, wxLEFT | wxTOP | wxALIGN_LEFT, SMALL_SPACE);
  // basic settings
  top_sizer->Add(label_sizer_2, 0, wxEXPAND | wxTOP | wxBOTTOM, BIG_SPACE);
  top_sizer->Add(
    check_mouse_mode_hotkey, 0, wxLEFT | wxALIGN_LEFT, SMALL_SPACE);
  top_sizer->Add(
    check_enable_pipe_server, 0, wxLEFT | wxTOP | wxALIGN_LEFT, SMALL_SPACE);
  // advanced settings
  top_sizer->Add(label_sizer_3, 0, wxEXPAND | wxTOP | wxBOTTOM, BIG_SPACE);
  top_sizer->Add(log_levels_sizer, 0, wxTOP | wxBOTTOM, SMALL_SPACE);
  top_sizer->Add(
    check_auto_find_dll, 0, wxLEFT | wxTOP | wxALIGN_LEFT, SMALL_SPACE);
  top_sizer->Add(
    label_auto_find, 0, wxLEFT | wxTOP | wxALIGN_LEFT, SMALL_SPACE);
  top_sizer->Add(
    label_dll_folder_location, 0, wxALL | wxALIGN_LEFT, SMALL_SPACE);
  top_sizer->Add(text_dll_folder_location, 0, wxALL | wxEXPAND, SMALL_SPACE);

  auto* border_sizer = new wxBoxSizer(wxVERTICAL);
  border_sizer->Add(top_sizer, 0, wxALL | wxEXPAND, BORDER_SPACE);
  border_sizer->Add(
    buttons, 0, wxALL | wxALIGN_CENTER_HORIZONTAL, BORDER_SPACE);

  panel->SetSizer(border_sizer);

  // Fit the frame around the size of my controls.
  border_sizer->Fit(this);
}

void
SettingsFrame::ApplySettings(config::UserData& user_data)
{
  user_data.auto_find_track_ir_dll = check_auto_find_dll->IsChecked();
  user_data.track_on_start = check_track_on_start->IsChecked();
  user_data.quit_on_loss_of_trackir = check_quit_on_loss->IsChecked();
  user_data.hotkey_enabled = check_mouse_mode_hotkey->IsChecked();
  user_data.pipe_server_enabled = check_enable_pipe_server->IsChecked();
  // p_user_data_->log_level =
  // static_cast<spdlog::level::level_enum>(selection);
  //  user_data.log_level = choice_log_level->GetString();
  user_data.auto_find_track_ir_dll = check_auto_find_dll->IsChecked();
  // user_data.auto_find_track_ir_dll = text_dll_folder_location->IsChecked();
}
