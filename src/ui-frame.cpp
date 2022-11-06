/**
 * Main app frame window and GUI components.
 *
 * --License Boilerplate Placeholder--
 *
 */

#include "ui-frame.hpp"

#include <wx/colour.h>
#include <wx/dataview.h>

#include <algorithm>
#include <string>

#include "config.hpp"
#include "log.hpp"
#include "pipeserver.hpp"
#include "threads.hpp"
#include "trackers.hpp"
#include "types.hpp"
#include "ui-control-id.hpp"
#include "ui-dialogs.hpp"
#include "util.hpp"

const constexpr std::string_view k_version_no = "0.10.1";
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

Frame::Frame(wxPoint origin, wxSize dimensions)
  : wxFrame(nullptr, wxID_ANY, "Track IR Mouse", origin, dimensions)
{
  //////////////////////////////////////////////////////////////////////
  //                            Menu Bar                              //
  //////////////////////////////////////////////////////////////////////
  wxMenu* menuFile = new wxMenu;
  // TODO: implement open file? this will require use of the registry
  // menuFile->Append(wxID_OPEN, "&Open\tCtrl-O",
  // "Open a new settings file from disk.");
  menuFile->Append(wxID_SAVE, "&Save\tCtrl-S", "Save the configuration file");
  menuFile->Append(myID_MENU_RELOAD, "&Reload", "Reload the settings file.");
  menuFile->AppendSeparator();
  menuFile->Append(wxID_EXIT);

  wxMenu* menuEdit = new wxMenu;
  menuEdit->Append(myID_MENU_SETTINGS, "&Settings", "Edit app settings.");

  wxMenu* menuHelp = new wxMenu;
  // TODO: implement a help file
  // menuHelp->Append(wxID_HELP);
  menuHelp->Append(wxID_ABOUT);

  wxMenuBar* menuBar = new wxMenuBar;
  menuBar->Append(menuFile, "&File");
  menuBar->Append(menuEdit, "&Edit");
  menuBar->Append(menuHelp, "&Help");

  SetMenuBar(menuBar);
  // CreateStatusBar();  // not currently using status bar
  // SetStatusText("0");

  //////////////////////////////////////////////////////////////////////
  //                               Panel                              //
  //////////////////////////////////////////////////////////////////////

  // TODO: make app expand on show log window
  // TODO: make app retract on hide log window
  // Layout paneles
  // clang-format off
  
  // Panels 
  auto main = new wxPanel(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxBORDER_SIMPLE);
  auto p_pnl_profile = new wxPanel(main, wxID_ANY, wxDefaultPosition, wxSize(800, 400), wxBORDER_SIMPLE);
  // auto m_pnlDisplayConfig = new wxPanel(profile);???????
  //p_pnl_profile->SetBackgroundColour(orange);
  

  // Logging Window
  label_text_rich_ = new wxStaticText(main, wxID_ANY, "Log Output:");
  p_text_rich_ = new LogWindow(main, wxID_ANY, "", wxDefaultPosition, wxSize(300, 10), wxTE_RICH | wxTE_MULTILINE);
  p_text_rich_->SetFont(wxFont(12, wxFONTFAMILY_TELETYPE, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));

  // Track Controls
  auto btn_start_mouse = new wxButton(main, myID_START_TRACK, "Start Mouse", wxDefaultPosition, k_default_button_size);
  auto btn_stop_mouse = new wxButton(main, myID_STOP_TRACK, "Stop Mouse", wxDefaultPosition, k_default_button_size);
  auto btn_hide_log = new wxButton(main, wxID_ANY, "Show/Hide Log", wxDefaultPosition, k_default_button_size);

  // Display Graphic
  p_display_graphic_ = new cDisplayGraphic(main, wxSize(650, 200));
  //p_display_graphic_->SetBackgroundColour(blue);

  // Profiles Controls
  auto *txtProfiles = new wxStaticText(p_pnl_profile, wxID_ANY, " Active Profile: ");
  // TODO: make this choice control visually taller when not dropped down
  p_combo_profiles_ = new wxChoice(p_pnl_profile, myID_PROFILE_SELECTION, wxDefaultPosition, wxSize(100, 25), 0, 0, wxCB_SORT, wxDefaultValidator, "");
  auto btn_add_profile = new wxButton(p_pnl_profile, wxID_ANY, "Add a Profile", wxDefaultPosition, k_default_button_size_2);
  auto btn_remove_profile = new wxButton(p_pnl_profile, wxID_ANY, "Remove a Profile", wxDefaultPosition, k_default_button_size_2);
  auto btn_duplicate_profile = new wxButton(p_pnl_profile, wxID_ANY, "Duplicate this Profile", wxDefaultPosition, k_default_button_size_2);

  // Profiles Box
  p_titles_map_ = std::make_unique<config::game_title_map_t>(config::GetTitleIds());
	
  p_text_name_ = new wxTextCtrl(p_pnl_profile, wxID_ANY, "Lorem Ipsum", wxDefaultPosition, wxSize(250, 20), wxTE_LEFT);
  p_text_name_->SetMaxLength(k_max_profile_length);
  p_text_profile_game_title_ = new wxTextCtrl(p_pnl_profile, wxID_ANY, "lorem", wxDefaultPosition, wxSize(200, 20), wxTE_READONLY | wxTE_LEFT);
  p_text_profile_id_ = new wxTextCtrl(p_pnl_profile, wxID_ANY, "2201576", wxDefaultPosition, wxSize(60, 20), wxTE_LEFT, alphanumeric_validator, "");
  auto btn_pick_title = new wxButton(p_pnl_profile, wxID_ANY, "Pick Title", wxDefaultPosition, k_default_button_size, 0, wxDefaultValidator, "");
  p_check_use_default_padding_ = new wxCheckBox(p_pnl_profile, wxID_ANY, "Use Default Padding", wxDefaultPosition, wxDefaultSize, wxCHK_2STATE, wxDefaultValidator, "");
  auto btn_move_up = new wxButton(p_pnl_profile, wxID_ANY, "Up", wxDefaultPosition, wxSize(50, 30), 0, wxDefaultValidator, "");
  auto btn_move_down = new wxButton(p_pnl_profile, wxID_ANY, "Down", wxDefaultPosition, wxSize(50, 30), 0, wxDefaultValidator, "");
  auto btn_add_display = new wxButton(p_pnl_profile, wxID_ANY, "+", wxDefaultPosition, wxSize(50, 30), 0, wxDefaultValidator, "");
  auto btn_remove_display = new wxButton(p_pnl_profile, wxID_ANY, "-", wxDefaultPosition, wxSize(50, 30), 0, wxDefaultValidator, "");

  constexpr int kColumnWidth = 70;
  p_view_mapping_data_ = new wxDataViewListCtrl(p_pnl_profile, myID_MAPPING_DATA, wxDefaultPosition, wxSize(680, 180), wxDV_HORIZ_RULES, wxDefaultValidator);
  p_view_mapping_data_->AppendTextColumn("Display #", wxDATAVIEW_CELL_INERT, -1, wxALIGN_RIGHT, wxDATAVIEW_COL_RESIZABLE);
  
  
  // TODO: add dataview spin class renderer to control
  ////auto dvcol = wxDataViewColumn("1", wxDataViewSpinRenderer(0, 5), col++);
  //auto rend = new wxDataViewSpinRenderer(0, 5);
  //auto tttl = new wxString("titleee");
  //auto dvcol = new wxDataViewColumn(*tttl, rend, 1);

  //p_view_mapping_data_->AppendColumn(dvcol);
  p_view_mapping_data_->AppendTextColumn("Left", wxDATAVIEW_CELL_EDITABLE, kColumnWidth, wxALIGN_RIGHT, wxDATAVIEW_COL_RESIZABLE);
  p_view_mapping_data_->AppendTextColumn("Right", wxDATAVIEW_CELL_EDITABLE, kColumnWidth, wxALIGN_RIGHT, wxDATAVIEW_COL_RESIZABLE);
  p_view_mapping_data_->AppendTextColumn("Top", wxDATAVIEW_CELL_EDITABLE, kColumnWidth, wxALIGN_RIGHT, wxDATAVIEW_COL_RESIZABLE);
  p_view_mapping_data_->AppendTextColumn("Bottom", wxDATAVIEW_CELL_EDITABLE, kColumnWidth, wxALIGN_RIGHT, wxDATAVIEW_COL_RESIZABLE);
  p_view_mapping_data_->AppendTextColumn("Left", wxDATAVIEW_CELL_EDITABLE, kColumnWidth, wxALIGN_RIGHT, wxDATAVIEW_COL_RESIZABLE);
  p_view_mapping_data_->AppendTextColumn("Right", wxDATAVIEW_CELL_EDITABLE, kColumnWidth, wxALIGN_RIGHT, wxDATAVIEW_COL_RESIZABLE);
  p_view_mapping_data_->AppendTextColumn("Top", wxDATAVIEW_CELL_EDITABLE, kColumnWidth, wxALIGN_RIGHT, wxDATAVIEW_COL_RESIZABLE);
  p_view_mapping_data_->AppendTextColumn("Bottom", wxDATAVIEW_CELL_EDITABLE, kColumnWidth, wxALIGN_RIGHT, wxDATAVIEW_COL_RESIZABLE);

  auto txtPanelTitle = new wxStaticText(p_pnl_profile, wxID_ANY, "Active Profile - Panel");
  auto txtProfileName = new wxStaticText(p_pnl_profile, wxID_ANY, "Name:  ");
  auto txtProfileId = new wxStaticText(p_pnl_profile, wxID_ANY, "Game ID:  ");
  auto txtGameTitle = new wxStaticText(p_pnl_profile, wxID_ANY, "Game Title:  ");
  auto txtHeaders = new wxStaticText(p_pnl_profile, wxID_ANY,
                                              "                           |------------- Rotational "
                                              "Bounds Mapping -----------|-------------------- "
                                              "Display Edge Padding -------------------|");



  // Profile Info Panel
  auto zrDisplayControls = new wxBoxSizer(wxVERTICAL);
  zrDisplayControls->Add(btn_move_up);
  zrDisplayControls->Add(btn_move_down);
  zrDisplayControls->Add(btn_add_display);
  zrDisplayControls->Add(btn_remove_display);

  auto zrMapping = new wxBoxSizer(wxHORIZONTAL);
  zrMapping->Add(p_view_mapping_data_, 0, wxALL, 5);
  zrMapping->Add(zrDisplayControls, 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);

  auto zrInfo = new wxFlexGridSizer(4, 5, 5);
  zrInfo->Add(txtProfileName);
  zrInfo->Add(txtGameTitle);
  zrInfo->Add(txtProfileId);
  zrInfo->AddSpacer(1);
  zrInfo->Add(p_text_name_, 0, wxALIGN_CENTER_VERTICAL, 0);
  zrInfo->Add(p_text_profile_game_title_, 0, wxALIGN_CENTER_VERTICAL, 0);
  zrInfo->Add(p_text_profile_id_, 0, wxALIGN_CENTER_VERTICAL, 0);
  zrInfo->Add(btn_pick_title, 0, wxALIGN_CENTER_VERTICAL, 0);

  auto *zrProfileCmds = new wxBoxSizer(wxHORIZONTAL);
  zrProfileCmds->Add(txtProfiles, 0, wxALIGN_CENTER_VERTICAL, 0);
  zrProfileCmds->Add(p_combo_profiles_, 1, wxEXPAND | wxTOP | wxRIGHT, 1);
  zrProfileCmds->Add(btn_add_profile, 0, wxALL | wxALIGN_CENTER_VERTICAL, 0);
  zrProfileCmds->Add(btn_remove_profile, 0, wxALL | wxALIGN_CENTER_VERTICAL, 0);
  zrProfileCmds->Add(btn_duplicate_profile, 0, wxALL | wxALIGN_CENTER_VERTICAL, 0);

  auto zrProfilePnl = new wxBoxSizer(wxVERTICAL);
  zrProfilePnl->Add(zrProfileCmds, 0, wxBOTTOM | wxEXPAND, 10);
  zrProfilePnl->Add(txtPanelTitle, 0, wxALL, 5);
  zrProfilePnl->Add(zrInfo, 0, wxALL | wxEXPAND, 5);
  zrProfilePnl->Add(p_check_use_default_padding_, 0, wxALL, 5);
  zrProfilePnl->Add(txtHeaders, 0, wxLEFT | wxTOP, 5);
  zrProfilePnl->Add(zrMapping, 0, wxLEFT | wxTOP, 5);

  p_pnl_profile->SetSizer(zrProfilePnl);

  // Main Panel Layout
  auto *zrTrackCmds = new wxBoxSizer(wxHORIZONTAL);
  zrTrackCmds->Add(btn_start_mouse, 0, wxALL, 0);
  zrTrackCmds->Add(btn_stop_mouse, 0, wxALL, 0);
  zrTrackCmds->Add(btn_hide_log, 0, wxALL, 0);

  auto *zrLogWindow = new wxBoxSizer(wxVERTICAL);
  zrLogWindow->Add(label_text_rich_, 0, wxBOTTOM, 5);
  zrLogWindow->Add(p_text_rich_, 1, wxEXPAND, 0);

  auto *zrApp = new wxBoxSizer(wxVERTICAL);
  zrApp->Add(zrTrackCmds, 0, wxBOTTOM, 10);
  zrApp->Add(p_display_graphic_, 1, wxALL | wxEXPAND, 20);
  zrApp->Add(p_pnl_profile, 0, wxALL, 5);

  auto *top = new wxBoxSizer(wxHORIZONTAL);
  top->Add(zrApp, 2, wxALL | wxEXPAND, 10);
  top->Add(zrLogWindow, 1, wxALL | wxEXPAND, 5);

  main->SetSizer(top);
  main->Fit();

  // Bind Keyboard Shortcuts
  wxAcceleratorEntry k1(wxACCEL_CTRL, WXK_CONTROL_S, wxID_SAVE);
  SetAcceleratorTable(wxAcceleratorTable(1, &k1));

	// Bind Systemwide Keyboard Hooks
	// Use EVT_HOTKEY(hotkeyId, fnc) in the event table to capture the event.
	// This function is currently only implemented under Windows.
	// It is used in the Windows CE port for detecting hardware button presses.
	hotkey_alternate_mode_ = std::make_unique<GlobalHotkey>(GetHandle(), HOTKEY_ID_SCROLL_ALTERNATE, wxMOD_NONE, VK_F18);
	Bind(wxEVT_HOTKEY, &Frame::OnGlobalHotkey, this, hotkey_alternate_mode_->hk_id);

	hook_window_changed_ = std::make_unique<WindowChangedHook>();

  // Bind Menu Events
  Bind(wxEVT_COMMAND_MENU_SELECTED, &Frame::OnAbout, this, wxID_ABOUT);
  Bind(wxEVT_COMMAND_MENU_SELECTED, &Frame::OnExit, this, wxID_EXIT);
  // Bind(wxEVT_COMMAND_MENU_SELECTED, &Frame::OnOpen, this, wxID_OPEN);
  Bind(wxEVT_COMMAND_MENU_SELECTED, &Frame::OnSave, this, wxID_SAVE);
  Bind(wxEVT_COMMAND_MENU_SELECTED, &Frame::OnReload, this, myID_MENU_RELOAD);
  Bind(wxEVT_COMMAND_MENU_SELECTED, &Frame::OnSettings, this, myID_MENU_SETTINGS);

  // Set Control Tooltips
  btn_start_mouse->SetToolTip(wxT("Start controlling mouse with head tracking."));
  btn_stop_mouse->SetToolTip(wxT("Stop control of the mouse."));

  // Bind Main Controls
  btn_start_mouse->Bind(wxEVT_BUTTON, &Frame::OnStart, this);
  btn_stop_mouse->Bind(wxEVT_BUTTON, &Frame::OnStop, this);
  btn_hide_log->Bind(wxEVT_BUTTON, &Frame::OnShowLog, this);
  
  p_combo_profiles_->Bind(wxEVT_CHOICE, &Frame::OnActiveProfile, this);
  btn_add_profile->Bind(wxEVT_BUTTON, &Frame::OnAddProfile, this);
  btn_remove_profile->Bind(wxEVT_BUTTON, &Frame::OnRemoveProfile, this);
  btn_duplicate_profile->Bind(wxEVT_BUTTON, &Frame::OnDuplicateProfile, this);

  // Bind Profile Info Controls
  p_text_name_->Bind(wxEVT_TEXT, &Frame::OnName, this);
  p_text_profile_id_->Bind(wxEVT_TEXT, &Frame::OnProfileID, this);
  btn_pick_title->Bind(wxEVT_BUTTON, &Frame::OnPickTitle, this);
  p_check_use_default_padding_->Bind(wxEVT_CHECKBOX, &Frame::OnUseDefaultPadding, this);
  p_view_mapping_data_->Bind(wxEVT_DATAVIEW_ITEM_EDITING_DONE, &Frame::OnMappingData, this);
  btn_add_display->Bind(wxEVT_BUTTON, &Frame::OnAddDisplay, this);
  btn_remove_display->Bind(wxEVT_BUTTON, &Frame::OnRemoveDisplay, this);
  btn_move_up->Bind(wxEVT_BUTTON, &Frame::OnMoveUp, this);
  btn_move_down->Bind(wxEVT_BUTTON, &Frame::OnMoveDown, this);
}
// clang-format on

void
Frame::OnExit(wxCommandEvent& event)
{
  Close(true);
}

void
Frame::OnAbout(wxCommandEvent& event)
{
  std::string msg = fmt::format(
    "Version No.  {}\nThis is a mouse control application using head "
    "tracking.\n"
    "Author: George Kuegler\n"
    "E-mail: georgekuegler@gmail.com",
    k_version_no);

  wxMessageBox(msg, "About TrackIRMouse", wxOK | wxICON_NONE);
}

void
Frame::OnSave(wxCommandEvent& event)
{
  config::Get()->save_to_file("settings.toml");
}

void
Frame::OnGlobalHotkey(wxKeyEvent& event)
{
  spdlog::debug("hot key event");
  if (track_thread_) {
    track_thread_->handler_->toggle_alternate_mode();
  }
}

void
Frame::InitializeSettings()
{
  const std::string filename = "settings.toml";
  config::ConfigReturn result = config::LoadFromFile(filename);
  if (retcode::success == result.code) {
    config::Set(result.config);
  } else {
    const wxString ok = "Load Empty User Settings";
    const wxString cancel = "Quit";
    const wxString instructions =
      wxString::Format("\n\nPress \"%s\" to load a default user settings "
                       "template.\nWarning: "
                       "data may be overwritten if you "
                       "continue with this option and then later save.\n"
                       "Press \"%s\" to exit the program.",
                       ok,
                       cancel);
    auto dlg = wxMessageDialog(this,
                               result.err_msg + instructions,
                               "Error",
                               wxICON_ERROR | wxOK | wxCANCEL);
    dlg.SetOKCancelLabels(ok, cancel);

    // display reason for error to user
    // give user the chance to quit application (preventing possible data
    // loss and manually fixing the error) or load default/empty config
    if (dlg.ShowModal() == wxID_OK) {
      config::Set(config::Config());
    } else {
      spdlog::warn("user closed app when presented with invalid settings load");
      Close(true);
    }
  }
}

void
Frame::UpdateGuiFromConfig()
{
  // Populate GUI With Settings
  PopulateComboBoxWithProfiles();
  UpdateProfilePanelFromConfig();
}

void
Frame::OnReload(wxCommandEvent& event)
{
  InitializeSettings();
  UpdateGuiFromConfig();
}

void
Frame::OnSettings(wxCommandEvent& event)
{
  auto config = config::Get();
  auto usr = config->user_data; // copy user data

  // Show the settings pop up while disabling input on main window
  cSettingsPopup dlg(this, &usr);
  int results = dlg.ShowModal();

  if (wxID_OK == results) {
    spdlog::debug("settings applied.");
    config->user_data = usr; // apply settings

    // reload any resources from setting changes:
    // TODO: make resources reload on setting changes
    auto logger_ = spdlog::get("main");
    logger_->set_level(usr.log_level);
  } else if (wxID_CANCEL == results) {
    spdlog::debug("settings rejected");
  }
}

void
Frame::PopulateComboBoxWithProfiles()
{
  spdlog::trace("repopulating profiles combobox");
  p_combo_profiles_->Clear();
  for (auto& item : config::Get()->GetProfileNames()) {
    p_combo_profiles_->Append(item);
  }

  int index =
    p_combo_profiles_->FindString(config::Get()->GetActiveProfile().name);
  if (wxNOT_FOUND != index) {
    p_combo_profiles_->SetSelection(index);
    UpdateProfilePanelFromConfig();
  } else {
    wxFAIL_MSG("unable to find new profile in drop-down");
  }
}

void
Frame::OnStart(wxCommandEvent& event)
{
  // Note:
  // Threads run in detached mode by default.
  // After the call to wxThread::Run(), the thread pointer is "unsafe".
  // At any moment the thread may cease to exist (because it completes its work)
  // and cause NULL exceptions upon access.
  // To avoid dangling pointers, ~MyThread() will enter the critical section and
  // set track_thread_ to nullptr.

  bool wait_for_stop = false;
  { // scope critical section locker
    // TODO: find a better way to enter critical section
    wxCriticalSectionLocker enter(p_cs_track_thread);
    if (track_thread_) {
      track_thread_->tracker_->stop();
      wait_for_stop = true;
    }
  } // leave critical section

  if (wait_for_stop) {
    // TODO: set a timeout here? and close app if thread doesn't die
    while (true) {
      Sleep(8);

      { // scope critical section locker
        // TODO: find a better way to enter critical section
        wxCriticalSectionLocker enter(p_cs_track_thread);
        if (track_thread_ == NULL) {
          break;
        }
      } // leave critical section
    }
  }

  try {
    track_thread_ = new TrackThread(this, GetHandle(), config::Get());
  } catch (const std::runtime_error& e) {
    spdlog::error(e.what());
    return;
  }

  if (track_thread_->Run() == wxTHREAD_NO_ERROR) { // returns immediately
    spdlog::info("Started Mouse.");
  } else {
    spdlog::error("Can't run the tracking thread!");
    delete track_thread_;
    // track_thread_ = nullptr; // thread destructor should do this anyway?
    return;
  }
}

void
Frame::OnStop(wxCommandEvent& event)
{
  // Threads run in detached mode by default.
  // The thread is responsible for setting track_thread_ to nullptr when
  // it finishes and destroys itself.
  if (track_thread_) {
    // This will gracefully exit the tracking loop and return control to
    // the wxThread class. The thread will then finish and and delete
    // itself.
    track_thread_->tracker_->stop();
  } else {
    spdlog::warn("Track thread not running!");
  }
}

void
Frame::OnShowLog(wxCommandEvent& event)
{
  if (p_text_rich_->IsShown()) {
    p_text_rich_->Hide();
    label_text_rich_->Hide();
  } else {
    p_text_rich_->Show();
    label_text_rich_->Show();
  }
}

void
Frame::OnActiveProfile(wxCommandEvent& event)
{
  const int index = p_combo_profiles_->GetSelection();
  // TODO: make all strings utf-8
  const auto name = p_combo_profiles_->GetString(index).ToStdString();
  config::Get()->SetActiveProfile(name);
  UpdateProfilePanelFromConfig();
}

void
Frame::OnAddProfile(wxCommandEvent& event)
{
  wxTextEntryDialog dlg(
    this, "Add Profile", "Specify a Name for the New Profile");
  // dlg.SetTextValidator(wxFILTER_ALPHANUMERIC);
  dlg.SetMaxLength(k_max_profile_length);
  if (dlg.ShowModal() == wxID_OK) {
    wxString value = dlg.GetValue();
    config::Get()->AddProfile(std::string(value.ToStdString()));
    UpdateGuiFromConfig();
  } else {
    spdlog::debug("Add profile action canceled.");
  }
}

void
Frame::OnRemoveProfile(wxCommandEvent& event)
{
  wxArrayString choices;
  for (auto& name : config::Get()->GetProfileNames()) {
    choices.Add(name);
  }

  const wxString msg = "Delete a Profile";
  const wxString msg2 = "Press OK";
  wxMultiChoiceDialog dlg(
    this, msg, msg2, choices, wxOK | wxCANCEL, wxDefaultPosition);

  if (wxID_OK == dlg.ShowModal()) {
    auto selections = dlg.GetSelections();
    for (auto& index : selections) {
      config::Get()->RemoveProfile(choices[index].ToStdString());
    }

    UpdateGuiFromConfig();
  }
}

void
Frame::OnDuplicateProfile(wxCommandEvent& event)
{
  config::Get()->DuplicateActiveProfile();
  UpdateGuiFromConfig();
}

void
Frame::UpdateProfilePanelFromConfig()
{
  const auto config = config::Get(); // get shared pointer
  const auto& profile = config->GetActiveProfile();

  // SetValue causes an event to be sent for text control.
  // SetValue does not cause an event to be sent for checkboxes.
  // Use ChangeEvent instead.
  // We don't want to register event when we're just loading values.
  p_text_name_->ChangeValue(profile.name);
  p_check_use_default_padding_->SetValue(profile.use_default_padding);
  p_text_profile_id_->ChangeValue(wxString::Format("%d", profile.profile_id));

  // get the game title from profile id
  auto* titles = p_titles_map_.get();
  p_text_profile_game_title_->ChangeValue(
    (*titles)[std::to_string(profile.profile_id)]);

  p_view_mapping_data_->DeleteAllItems();

  int displayNum = 0;

  for (int i = 0; i < profile.displays.size(); i++) {
    wxVector<wxVariant> row;
    row.push_back(wxVariant(wxString::Format("%d", i)));

    for (int j = 0; j < 4; j++) { // left, right, top, bottom
      row.push_back(
        wxVariant(wxString::Format("%7.2f", profile.displays[i].rotation[j])));
    }
    for (int j = 0; j < 4; j++) { // left, right, top, bottom
      if (p_check_use_default_padding_->IsChecked()) {
        row.push_back(wxVariant(wxString::Format("%s", "(default)")));
      } else {
        row.push_back(
          wxVariant(wxString::Format("%d", profile.displays[i].padding[j])));
      }
    }
    p_view_mapping_data_->AppendItem(row);
  }
  // display graphic->PaintNow();
}

void
Frame::OnName(wxCommandEvent& event)
{
  const auto text = p_text_name_->GetLineText(0).ToStdString();
  auto& profile = config::Get()->GetActiveProfile();
  profile.name = text;
  config::Get()->user_data.active_profile_name = text;
  UpdateGuiFromConfig();
}

void
Frame::OnProfileID(wxCommandEvent& event)
{
  // from txt entry inheritted
  const auto number = p_text_profile_id_->GetValue();
  ;
  long value;
  if (!number.ToLong(&value)) { // wxWidgets has odd conversions
    spdlog::error("Couldn't convert value to integer.");
    return;
  };
  auto& profile = config::Get()->GetActiveProfile();
  profile.profile_id = static_cast<int>(value);
  UpdateProfilePanelFromConfig();
}

void
Frame::OnPickTitle(wxCommandEvent& event)
{
  // a defaut map should have been provided at load time
  // if the map failed to load from file
  wxASSERT((*p_titles_map_).empty() == false);

  // seperate named and unamed for sorting purposes
  // in this case I would like title ID #'s with names to be sorted
  // alphabetically up front, while the ID #'s with no name to be sorted
  // by ID # after.
  GameTitleVector titles_named;
  GameTitleVector titles_no_name;
  titles_named.reserve(p_titles_map_->size());
  titles_no_name.reserve(p_titles_map_->size());

  for (auto& [key, item] : *p_titles_map_) {
    if (item.empty()) {
      titles_no_name.push_back({ item, key });
    } else {
      titles_named.push_back({ item, key });
    }
  }

  std::sort(titles_named.begin(),
            titles_named.end(),
            [](const std::pair<std::string, std::string>& left,
               const std::pair<std::string, std::string>& right) {
              return left.first < right.first;
            });

  std::sort(titles_no_name.begin(),
            titles_no_name.end(),
            [](const std::pair<std::string, std::string>& left,
               const std::pair<std::string, std::string>& right) {
              return std::stoi(left.second) < std::stoi(right.second);
            });

  // construct array used for displaying game titles to user
  GameTitleVector titles(titles_named);
  titles.insert(titles.end(), titles_no_name.begin(), titles_no_name.end());

  int selected_profile_id = 0;
  cProfileIdSelector dlg(this, &selected_profile_id, titles);
  if (dlg.ShowModal() == wxID_OK) {
    auto& profile = config::Get()->GetActiveProfile();
    profile.profile_id = selected_profile_id;
    UpdateProfilePanelFromConfig();
  }
  return;
}

void
Frame::OnUseDefaultPadding(wxCommandEvent& event)
{
  auto& profile = config::Get()->GetActiveProfile();
  profile.use_default_padding = p_check_use_default_padding_->IsChecked();
  UpdateProfilePanelFromConfig();
}

void
Frame::OnMappingData(wxDataViewEvent& event)
{
  auto& profile = config::Get()->GetActiveProfile();

  // finding column
  const wxVariant value = event.GetValue();
  const int column = event.GetColumn();

  // finding associated row; there is no event->GetRow()!?
  const wxDataViewItem item = event.GetItem();
  const wxDataViewIndexListModel* model =
    (wxDataViewIndexListModel*)(event.GetModel());
  const int row = model->GetRow(item);

  // if rotation value was selected
  // note 0th column made non-editable
  if (column < 5) {
    double number; // wxWidgets has odd string conversion procedures
    if (!value.GetString().ToDouble(&number)) {
      spdlog::error("Value could not be converted to a number.");
      return;
    }
    // TODO: use a wxValidator
    // this will prevent the value from sticking in the box on non valid
    // input make numbers only allowed too. validate input
    if (number > 180.0) {
      spdlog::error(
        "Value can't be greater than 180. This is a limitation of the "
        "TrackIR software.");
      return;
    }
    if (number < -180.0) {
      spdlog::error("Value can't be less than -180. This is a limitation of "
                    "the TrackIR "
                    "software.");
      return;
    }
    profile.displays[row].rotation[column - 1] = number;
  } else {       // padding value selected
    long number; // wxWidgets has odd string conversion procedures
    if (!value.GetString().ToLong(&number)) {
      wxLogError("Value could not be converted to a number.");
      return;
    }
    if (number < 0) {
      spdlog::error("Padding value can't be negative.");
      return;
    }
    profile.displays[row].padding[column - 5] = static_cast<int>(number);
  }

  UpdateProfilePanelFromConfig();
  p_display_graphic_->PaintNow();
}

void
Frame::OnAddDisplay(wxCommandEvent& event)
{
  auto& profile = config::Get()->GetActiveProfile();
  // TODO: can this be emplace back?
  profile.displays.push_back(
    config::UserDisplay({ 0, 0, 0, 0 }, { 0, 0, 0, 0 }));
  UpdateProfilePanelFromConfig();
  p_display_graphic_->PaintNow();
}

void
Frame::OnRemoveDisplay(wxCommandEvent& event)
{
  auto& profile = config::Get()->GetActiveProfile();
  const auto index = p_view_mapping_data_->GetSelectedRow();
  if (wxNOT_FOUND != index) {
    profile.displays.erase(profile.displays.begin() + index);
    UpdateProfilePanelFromConfig();
    p_display_graphic_->PaintNow();
  } else {
    spdlog::warn("row not selected");
  }
}

void
Frame::OnMoveUp(wxCommandEvent& event)
{
  auto& profile = config::Get()->GetActiveProfile();
  const auto index = p_view_mapping_data_->GetSelectedRow();
  if (wxNOT_FOUND == index) {
    spdlog::warn("row not selected");
  } else if (0 == index) {
    spdlog::warn("row is already at top");
    return; // selection is at the top. do nothing
  } else {
    std::swap(profile.displays[index], profile.displays[index - 1]);
    UpdateProfilePanelFromConfig();

    // Change the selection index to match the item we just moved
    // This is a terrible hack to find the row of the item we just moved
    const auto model =
      (wxDataViewIndexListModel*)p_view_mapping_data_->GetModel();
    const auto item = model->GetItem(index - 1);
    wxDataViewItemArray items(size_t(1)); // no braced init constructor :(
    items[0] = item;                      // assumed single selection mode
    p_view_mapping_data_->SetSelections(items);
    p_display_graphic_->PaintNow();
  }
}

void
Frame::OnMoveDown(wxCommandEvent& event)
{
  auto& profile = config::Get()->GetActiveProfile();
  const auto index = p_view_mapping_data_->GetSelectedRow();
  if (wxNOT_FOUND == index) {
    spdlog::warn("row not selected");
  } else if ((p_view_mapping_data_->GetItemCount() - 1) == index) {
    spdlog::warn("row is already at bottom");
    return; // selection is at the bottom. do nothing
  } else {
    std::swap(profile.displays[index], profile.displays[index + 1]);
    UpdateProfilePanelFromConfig();

    // Change the selection index to match the item we just moved
    // This is a terrible hack to find the row of the item we just moved
    const auto model =
      (wxDataViewIndexListModel*)p_view_mapping_data_->GetModel();
    const auto item = (model->GetItem(index + 1));
    wxDataViewItemArray items(size_t(1)); // no braced init constructor :(
    items[0] = item;                      // assumed single selection mode
    p_view_mapping_data_->SetSelections(items);
    p_display_graphic_->PaintNow();
  }
}
