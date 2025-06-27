/**
 * Main app frame window and GUI components.
 *
 * --License Boilerplate Placeholder--
 *
 */

#include "frame.hpp"

#include <wx/colour.h>
#include <wx/dataview.h>

#include <algorithm>
#include <filesystem>
#include <string>
#include <string_view>

#include "constants.hpp"
#include "game-titles.hpp"
#include "log.hpp"
#include "pipeserver.hpp"
#include "settings.hpp"
#include "shell.hpp"
#include "threads.hpp"
#include "trackers.hpp"
#include "types.hpp"
#include "utility.hpp"
#include "version.hpp"

#include "ui/control-id.hpp"
#include "ui/dialog-display-edit.hpp"
#include "ui/dialog-profile-selector.hpp"
#include "ui/dialog-settings.hpp"

static const wxSize k_default_button_size = wxSize(110, 25);
static const wxSize k_default_button_size_2 = wxSize(150, 25);
static const wxTextValidator alphanumeric_validator(wxFILTER_ALPHANUMERIC);
static constexpr int k_max_profile_length = 30;

// static constexpr std::string_view k_version_no = "1.1.2";
static constexpr std::string_view k_version_no =
  STR(TIRMOUSE_VER_MAJOR) "." STR(TIRMOUSE_VER_MINOR) "." STR(TIRMOUSE_VER_PATCH);
static constexpr std::string_view k_build_date = __DATE__;
static const std::string k_about_message =
  fmt::format("Version No:  {}\n"
              "Build Date:  {}\n\n"
              "This is a mouse control application using head tracking.\n\n"
              "Author: George Kuegler\n"
              "E-mail: georgekuegler@gmail.com",
              k_version_no,
              k_build_date);

// colors used for testing
const wxColor yellow(255, 255, 0);
const wxColor blue(255, 181, 102);
const wxColor pink(198, 102, 255);
const wxColor green(142, 255, 102);
const wxColor orange(102, 201, 255);

MainWindow::MainWindow(wxPoint origin, wxSize dimensions, Settings& s)
  : settings_(s)
  , wxFrame(nullptr, wxID_ANY, "Track IR Mouse", origin, dimensions)
{
  SetupMenubar();

  //////////////////////////////////////////////////////////////////////
  //                               Panel                              //
  //////////////////////////////////////////////////////////////////////

  // clang-format off
  
  // Panels 
  auto main = new wxPanel(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxBORDER_NONE);
  auto p_pnl_profile = new wxPanel(main, wxID_ANY, wxDefaultPosition, wxSize(800, 400), wxBORDER_SIMPLE);
  //p_pnl_profile->SetBackgroundColour(orange);
  

  // Logging Window
  label_text_rich_ = new wxStaticText(main, wxID_ANY, "Log Output:");
  p_text_rich_ = new LogOutputControl(main, wxID_ANY, "", wxDefaultPosition, wxSize(300, 10), wxTE_RICH | wxTE_MULTILINE);
  p_text_rich_->SetFont(wxFont(12, wxFONTFAMILY_TELETYPE, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));

  // Track Controls
  auto btn_start_mouse = new wxButton(main, wxID_ANY, "Start/Restart Mouse", wxDefaultPosition, k_default_button_size_2);
  auto btn_stop_mouse = new wxButton(main, wxID_ANY, "Stop Mouse", wxDefaultPosition, k_default_button_size_2);

  // Display Graphic
  p_display_graphic_ = new PanelDisplayGraphic(main, wxSize(650, 200), s);
  //p_display_graphic_->SetBackgroundColour(blue);

  // Profiles Controls
  auto *txtProfiles = new wxStaticText(p_pnl_profile, wxID_ANY, " Active Profile: ");
  // TODO: make this choice control visually taller when not dropped down
  p_combo_profiles_ = new wxChoice(p_pnl_profile, wxID_ANY, wxDefaultPosition, wxSize(100, 25), 0, 0, wxCB_SORT, wxDefaultValidator, "");
  auto btn_add_profile = new wxButton(p_pnl_profile, wxID_ANY, "Add a Profile", wxDefaultPosition, k_default_button_size_2);
  auto btn_remove_profile = new wxButton(p_pnl_profile, wxID_ANY, "Remove a Profile", wxDefaultPosition, k_default_button_size_2);
  auto btn_duplicate_profile = new wxButton(p_pnl_profile, wxID_ANY, "Duplicate this Profile", wxDefaultPosition, k_default_button_size_2);

  // Profiles Box
  p_titles_map_ = std::make_unique<game_title_map_t>(GetTitleIds());
	
  p_text_name_ = new wxTextCtrl(p_pnl_profile, wxID_ANY, "Lorem Ipsum", wxDefaultPosition, wxSize(250, 20), wxTE_LEFT);
  p_text_name_->SetMaxLength(k_max_profile_length);
  p_text_profile_game_title_ = new wxTextCtrl(p_pnl_profile, wxID_ANY, "lorem", wxDefaultPosition, wxSize(200, 20), wxTE_READONLY | wxTE_LEFT);
  p_text_profile_id_ = new wxTextCtrl(p_pnl_profile, wxID_ANY, "2201576", wxDefaultPosition, wxSize(60, 20), wxTE_LEFT, alphanumeric_validator, "");
  auto btn_pick_title = new wxButton(p_pnl_profile, wxID_ANY, "Pick Title", wxDefaultPosition, k_default_button_size, 0, wxDefaultValidator, "");
  p_check_use_default_padding_ = new wxCheckBox(p_pnl_profile, wxID_ANY, "Use Default Padding", wxDefaultPosition, wxDefaultSize, wxCHK_2STATE, wxDefaultValidator, "");
  auto btn_move_edit = new wxButton(p_pnl_profile, wxID_ANY, "Edit", wxDefaultPosition, wxSize(50, 30), 0, wxDefaultValidator, "");
  auto btn_move_up = new wxButton(p_pnl_profile, wxID_ANY, "Up", wxDefaultPosition, wxSize(50, 30), 0, wxDefaultValidator, "");
  auto btn_move_down = new wxButton(p_pnl_profile, wxID_ANY, "Down", wxDefaultPosition, wxSize(50, 30), 0, wxDefaultValidator, "");
  auto btn_add_display = new wxButton(p_pnl_profile, wxID_ANY, "+", wxDefaultPosition, wxSize(50, 30), 0, wxDefaultValidator, "");
  auto btn_remove_display = new wxButton(p_pnl_profile, wxID_ANY, "-", wxDefaultPosition, wxSize(50, 30), 0, wxDefaultValidator, "");

  constexpr int kColumnWidth = 70;
  p_view_mapping_data_ = new wxDataViewListCtrl(p_pnl_profile, wxID_ANY, wxDefaultPosition, wxSize(680, 180), wxDV_HORIZ_RULES, wxDefaultValidator);
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
  zrDisplayControls->Add(btn_move_edit);
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

  auto *zrLogWindow = new wxBoxSizer(wxVERTICAL);
  zrLogWindow->Add(label_text_rich_, 0, wxBOTTOM, 5);
  zrLogWindow->Add(p_text_rich_, 1, wxEXPAND, 0);

  auto *zrApp = new wxBoxSizer(wxVERTICAL);
  zrApp->Add(zrTrackCmds, 0, wxBOTTOM, 0);
  zrApp->Add(p_display_graphic_, 1, wxALL | wxEXPAND, 20);
  zrApp->Add(p_pnl_profile, 0, wxALL, 0);

  auto *top = new wxBoxSizer(wxHORIZONTAL);
  top->Add(zrApp, 0, wxALL | wxEXPAND, BORDER_SPACING_LG );
  top->Add(zrLogWindow, 1, wxRIGHT|wxTOP|wxBOTTOM | wxEXPAND, BORDER_SPACING_LG );

  main->SetSizer(top);
  main->Fit();

  // Bind Keyboard Shortcuts
  wxAcceleratorEntry k1(wxACCEL_CTRL, WXK_CONTROL_S, wxID_SAVE);
  SetAcceleratorTable(wxAcceleratorTable(1, &k1));

  // Set Control Tooltips
  btn_start_mouse->SetToolTip(wxT("Start controlling mouse with head tracking."));
  btn_stop_mouse->SetToolTip(wxT("Stop control of the mouse."));

  // Bind Main Controls
  btn_start_mouse->Bind(wxEVT_BUTTON, &MainWindow::OnStart, this);
  btn_stop_mouse->Bind(wxEVT_BUTTON, &MainWindow::OnStop, this);
  
  p_combo_profiles_->Bind(wxEVT_CHOICE, &MainWindow::OnActiveProfile, this);
  btn_add_profile->Bind(wxEVT_BUTTON, &MainWindow::OnAddProfile, this);
  btn_remove_profile->Bind(wxEVT_BUTTON, &MainWindow::OnRemoveProfile, this);
  btn_duplicate_profile->Bind(wxEVT_BUTTON, &MainWindow::OnDuplicateProfile, this);

  // Bind Profile Info Controls
  p_text_name_->Bind(wxEVT_TEXT, &MainWindow::OnName, this);
  p_text_profile_id_->Bind(wxEVT_TEXT, &MainWindow::OnProfileID, this);
  btn_pick_title->Bind(wxEVT_BUTTON, &MainWindow::OnPickTitle, this);
  p_check_use_default_padding_->Bind(wxEVT_CHECKBOX, &MainWindow::OnUseDefaultPadding, this);
  p_view_mapping_data_->Bind(wxEVT_DATAVIEW_ITEM_EDITING_DONE, &MainWindow::OnMappingData, this);
  btn_move_edit->Bind(wxEVT_BUTTON, &MainWindow::OnDisplayEdit, this);
  btn_move_up->Bind(wxEVT_BUTTON, &MainWindow::OnMoveUp, this);
  btn_move_down->Bind(wxEVT_BUTTON, &MainWindow::OnMoveDown, this);
  btn_add_display->Bind(wxEVT_BUTTON, &MainWindow::OnAddDisplay, this);
  btn_remove_display->Bind(wxEVT_BUTTON, &MainWindow::OnRemoveDisplay, this);
  // clang-format on
}

MainWindow::~MainWindow() {}

void
MainWindow::SetupMenubar()
{
  wxMenu* menuFile = new wxMenu;
  // TODO: implement open file? this will require use of the registry
  // menuFile->Append(wxID_OPEN, "&Open\tCtrl-O",
  // "Open a new settings file from disk.");
  menuFile->Append(wxID_PREFERENCES, "&Edit Settings", "Edit app settings.");
  menuFile->Append(wxID_SAVE, "&Save Settings\tCtrl-S", "Save the configuration file");
  menuFile->Append(myID_MENU_RELOAD_SETTINGS, "&Reload Settings", "Reload the settings file.");
  menuFile->AppendSeparator();
  menuFile->Append(wxID_EXIT);

  wxMenu* menuHelp = new wxMenu;
  // TODO: implement a help file
  // menuHelp->Append(wxID_HELP);
  menuHelp->Append(
    myID_MENU_VIEW_LOGFILE, "View Log", "Open the logfile with the default text editor.");
  menuHelp->Append(wxID_ABOUT);

  wxMenuBar* menuBar = new wxMenuBar;
  menuBar->Append(menuFile, "&File");
  menuBar->Append(menuHelp, "&Help");

  // Bind Menu Events
  Bind(wxEVT_COMMAND_MENU_SELECTED, &MainWindow::OnAbout, this, wxID_ABOUT);
  Bind(wxEVT_COMMAND_MENU_SELECTED, &MainWindow::OnExit, this, wxID_EXIT);
  // Bind(wxEVT_COMMAND_MENU_SELECTED, &MainWindow::OnOpen, this, wxID_OPEN);
  Bind(wxEVT_COMMAND_MENU_SELECTED, &MainWindow::OnSave, this, wxID_SAVE);
  Bind(wxEVT_COMMAND_MENU_SELECTED, &MainWindow::OnReload, this, myID_MENU_RELOAD_SETTINGS);
  Bind(wxEVT_COMMAND_MENU_SELECTED, &MainWindow::OnSettings, this, wxID_PREFERENCES);
  Bind(wxEVT_COMMAND_MENU_SELECTED, &MainWindow::OnLogFile, this, myID_MENU_VIEW_LOGFILE);

  SetMenuBar(menuBar);
  // CreateStatusBar(); // not currently using status bar
  // SetStatusText("Lorem Ipsum Status Bar Message");
}

void
MainWindow::UpdateGuiFromSettings()
{
  // Populate GUI With Settings
  PopulateComboBoxWithProfiles();
  UpdateProfilePanelFromSettings();
}

void
MainWindow::OnExit(wxCommandEvent& event)
{
  Close(true);
}

void
MainWindow::OnAbout(wxCommandEvent& event)
{
  wxMessageBox(k_about_message, "About TrackIRMouse", wxOK | wxICON_NONE);
}

void
MainWindow::OnSave(wxCommandEvent& event)
{
  try {
    settings_.SaveToFile();
  } catch (std::exception& ex) {
    spdlog::error("Couldn't save settings to file.\n\n{}", ex.what());
  }
}

void
MainWindow::OnReload(wxCommandEvent& event)
{
  // TODO: implement by making frame or app own a ref to settings.
  // if (!LoadSettingsFile()) {
  //   this->Close();
  // }
  // UpdateGuiFromSettings();
}

void
MainWindow::OnSettings(wxCommandEvent& event)
{

  DialogSettings dlg(this, settings_);

  // Show the settings pop up while disabling input on main window.
  int results = dlg.ShowModal();

  // Accept the changes.
  if (wxID_OK == results || wxID_APPLY == results) {

    dlg.ApplySettings(settings_);

    if (settings_.hotkey_enabled) {
      StartScrollAlternateHooksAndHotkeys();
    } else {
      RemoveHooks();
    }
    if (settings_.pipe_server_enabled) {
      StartPipeServer();
    } else {
      StopPipeServer();
    }
  }

  // Additionally save our new settings to disk.
  if (wxID_APPLY == results) {
    if (wxID_APPLY == results) {
      wxCommandEvent event;
      OnSave(event);
    }
  }

  // The user pressed the cancel button.
  if (wxID_CANCEL == results) {
    spdlog::debug("settings rejected");
  }
}

void
MainWindow::OnLogFile(wxCommandEvent& event)
{
  // open the help file with default editor
  // windows will prompt the user for a suitable program if not found
  // auto path = GetFullPath(settings->file_name_);

  // const std::string name = LOG_FILE_NAME;
  //  auto path = std::filesystem::absolute(LOG_FILE_NAME);
  // const std::string path = utility::GetExecutableFolder() + name;
  auto path = utility::GetAbsolutePathBasedFromExeFolder(LOG_FILE_NAME);
  try {
    ExecuteShellCommand(GetHandle(), "open", path);
  } catch (std::runtime_error& ex) {
    spdlog::error("Couldn't open '{}':{}\n", path, ex.what());
  }
}

void
MainWindow::OnScrollAlternateHotkey(wxKeyEvent& event)
{
  SPDLOG_TRACE("hot key event");
  wxCriticalSectionLocker enter(cs_track_thread_);
  if (track_thread_) {
    track_thread_->handler_->toggle_alternate_mode();
  }
}

void
MainWindow::StartScrollAlternateHooksAndHotkeys()
{
  // Bind Global/System-wide Hotkeys
  Bind(wxEVT_HOTKEY, &MainWindow::OnScrollAlternateHotkey, this, HOTKEY_ID_SCROLL_ALTERNATE);

  // Bind Systemwide Keyboard Hooks
  // Use EVT_HOTKEY(hotkeyId, fnc) in the event table to capture the event.
  // This function is currently only implemented under Windows.
  // It is used in the Windows CE port for detecting hardware button presses.
  if (!hotkey_alternate_mode_) {
    hotkey_alternate_mode_ =
      std::make_unique<GlobalHotkey>(GetHandle(), HOTKEY_ID_SCROLL_ALTERNATE, wxMOD_NONE, VK_F18);
  }
  if (!hook_window_changed_) {
    hook_window_changed_ = std::make_unique<HookWindowChanged>();
  }
}

void
MainWindow::RemoveHooks()
{
  hotkey_alternate_mode_.reset();
  hook_window_changed_.reset();
}

void
MainWindow::StartPipeServer()
{
  wxCriticalSectionLocker enter(cs_pipe_thread_);

  // Check to see if the pipe server is currently running.
  if (pipe_server_thread_) {
    return;
  }

  auto name = settings_.pipe_server_name;
  pipe_server_thread_ = new ThreadPipeServer(this, name);
  if (pipe_server_thread_->Run() != wxTHREAD_NO_ERROR) {
    spdlog::error("Can't run pipe server thread.");
    delete pipe_server_thread_;
    pipe_server_thread_ = NULL;
  }
}

void
MainWindow::StopPipeServer()
{
  GracefullyDeleteThreadAndWait<ThreadPipeServer>(pipe_server_thread_, cs_pipe_thread_);
}

void
MainWindow::PopulateComboBoxWithProfiles()
{
  spdlog::trace("repopulating profiles combobox");
  p_combo_profiles_->Clear();
  for (auto& item : settings_.GetProfileNames()) {
    p_combo_profiles_->Append(item);
  }

  int index = p_combo_profiles_->FindString(settings_.GetActiveProfile().name);
  if (wxNOT_FOUND != index) {
    p_combo_profiles_->SetSelection(index);
    UpdateProfilePanelFromSettings();
  } else {
    wxFAIL_MSG("unable to find new profile in drop-down");
  }
}

void
MainWindow::OnStart(wxCommandEvent& event)
{
  // Note:
  // Threads run in detached mode by default.
  // After the call to wxThread::Run(), the thread pointer is "unsafe".
  // At any moment the thread may cease to exist (because it completes its work)
  // and cause NULL exceptions upon access.
  // To avoid dangling pointers, ~MyThread() will enter the critical section and
  // set track_thread_ to nullptr.

  GracefullyDeleteThreadAndWait<ThreadHeadTracking>(track_thread_, cs_track_thread_);

  try {
    // Make a copy of settings and state.
    auto s = Settings(settings_);

    // Apply default padding values if necessary.
    s.ApplyNecessaryDefaults();

    if (track_thread_) {
      throw std::logic_error("track thread should not exist");
    }

    track_thread_ = new ThreadHeadTracking(this, this->GetHandle(), s);
  } catch (const std::runtime_error& e) {
    spdlog::error(e.what());
    return;
  }

  if (track_thread_->Run() == wxTHREAD_NO_ERROR) { // returns immediately
    spdlog::info("Started Mouse.");
    // this->SetStatusText("Running");
  } else {
    spdlog::error("Can't run the tracking thread!");
    delete track_thread_;
    // Don't set 'track_thread_ = nullptr'.
    // The thread's destructor is responsible for setting
    // it's pointer do 'nullptr'.
    return;
  }
}

void
MainWindow::OnStop(wxCommandEvent& event)
{

  GracefullyDeleteThreadAndWait<ThreadHeadTracking>(track_thread_, cs_track_thread_);
  return;
}

void
MainWindow::OnActiveProfile(wxCommandEvent& event)
{
  const int index = p_combo_profiles_->GetSelection();
  // TODO: make all strings utf-8 and globalization?
  // make a validator that only allows ASCII characters
  const auto name = p_combo_profiles_->GetString(index).ToStdString();
  settings_.SetActiveProfile(name);
  UpdateProfilePanelFromSettings();
}

void
MainWindow::OnAddProfile(wxCommandEvent& event)
{
  wxTextEntryDialog dlg(this, "Add Profile", "Specify a Name for the New Profile");
  // dlg.SetTextValidator(wxFILTER_ALPHANUMERIC);
  dlg.SetMaxLength(k_max_profile_length);
  if (dlg.ShowModal() == wxID_OK) {
    wxString value = dlg.GetValue();
    settings_.AddProfile(std::string(value.ToStdString()));
    UpdateGuiFromSettings();
  } else {
    spdlog::debug("Add profile action canceled.");
  }
}

void
MainWindow::OnRemoveProfile(wxCommandEvent& event)
{
  wxArrayString choices;
  for (auto& name : settings_.GetProfileNames()) {
    choices.Add(name);
  }

  const wxString msg = "Delete a Profile";
  const wxString msg2 = "Press OK";
  wxMultiChoiceDialog dlg(this, msg, msg2, choices, wxOK | wxCANCEL, wxDefaultPosition);

  if (wxID_OK == dlg.ShowModal()) {
    auto selections = dlg.GetSelections();
    for (auto& index : selections) {
      settings_.RemoveProfile(choices[index].ToStdString());
    }

    UpdateGuiFromSettings();
  }
}

void
MainWindow::OnDuplicateProfile(wxCommandEvent& event)
{
  settings_.DuplicateActiveProfile();
  UpdateGuiFromSettings();
}

void
MainWindow::UpdateProfilePanelFromSettings()
{
  const auto& profile = settings_.GetActiveProfile();

  // SetValue causes an event to be sent for text control.
  // SetValue does not cause an event to be sent for checkboxes.
  // Use ChangeEvent instead.
  // We don't want to register event when we're just loading values.
  p_text_name_->ChangeValue(profile.name);
  p_check_use_default_padding_->SetValue(profile.use_default_padding);
  p_text_profile_id_->ChangeValue(wxString::Format("%d", profile.profile_id));

  // get the game title from profile id
  auto* titles = p_titles_map_.get();
  p_text_profile_game_title_->ChangeValue((*titles)[std::to_string(profile.profile_id)]);

  p_view_mapping_data_->DeleteAllItems();

  int displayNum = 0;

  for (int i = 0; i < profile.displays.size(); i++) {
    wxVector<wxVariant> row;
    row.push_back(wxVariant(wxString::Format("%d", i)));

    for (int j = 0; j < 4; j++) { // left, right, top, bottom
      row.push_back(wxVariant(wxString::Format("%7.2f", profile.displays[i].rotation[j])));
    }
    for (int j = 0; j < 4; j++) { // left, right, top, bottom
      if (p_check_use_default_padding_->IsChecked()) {
        row.push_back(wxVariant(wxString::Format("%s", "(default)")));
      } else {
        row.push_back(wxVariant(wxString::Format("%d", profile.displays[i].padding[j])));
      }
    }
    p_view_mapping_data_->AppendItem(row);
  }
  // display graphic->PaintNow();
}

void
MainWindow::OnName(wxCommandEvent& event)
{
  const auto text = p_text_name_->GetLineText(0).ToStdString();
  auto& profile = settings_.GetActiveProfile();
  profile.name = text;
  settings_.active_profile_name = text;
  UpdateGuiFromSettings();
}

void
MainWindow::OnProfileID(wxCommandEvent& event)
{
  // from txt entry inheritted
  const auto number = p_text_profile_id_->GetValue();
  long value;
  if (!number.ToLong(&value)) { // wxWidgets has odd conversions
    spdlog::error("Couldn't convert value to integer.");
    return;
  };
  auto& profile = settings_.GetActiveProfile();
  profile.profile_id = static_cast<int>(value);
  UpdateProfilePanelFromSettings();
}

void
MainWindow::OnPickTitle(wxCommandEvent& event)
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

  std::sort(
    titles_named.begin(),
    titles_named.end(),
    [](const std::pair<std::string, std::string>& left,
       const std::pair<std::string, std::string>& right) { return left.first < right.first; });

  std::sort(titles_no_name.begin(),
            titles_no_name.end(),
            [](const std::pair<std::string, std::string>& left,
               const std::pair<std::string, std::string>& right) {
              return std::stoi(left.second) < std::stoi(right.second);
            });

  // construct array used for displaying game titles to user
  GameTitleVector titles(titles_named);
  titles.insert(titles.end(), titles_no_name.begin(), titles_no_name.end());

  int current_profile_id = 0;
  DialogProfileIdSelector dlg(this, current_profile_id, titles);
  if (dlg.ShowModal() == wxID_OK) {
    auto& profile = settings_.GetActiveProfile();
    profile.profile_id = dlg.GetSelectedProfileId();
    UpdateProfilePanelFromSettings();
  }
  return;
}

void
MainWindow::OnUseDefaultPadding(wxCommandEvent& event)
{
  auto& profile = settings_.GetActiveProfile();
  profile.use_default_padding = p_check_use_default_padding_->IsChecked();
  UpdateProfilePanelFromSettings();
}

void
MainWindow::OnMappingData(wxDataViewEvent& event)
{
  auto& profile = settings_.GetActiveProfile();

  // finding column
  const wxVariant value = event.GetValue();
  const int column = event.GetColumn();

  // finding associated row; there is no event->GetRow()!?
  const wxDataViewItem item = event.GetItem();
  const wxDataViewIndexListModel* model = (wxDataViewIndexListModel*)(event.GetModel());
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
      spdlog::error("Value can't be greater than 180. This is a limitation of the "
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

  UpdateProfilePanelFromSettings();
  p_display_graphic_->PaintNow();
}

void
MainWindow::OnAddDisplay(wxCommandEvent& event)
{
  auto& profile = settings_.GetActiveProfile();
  profile.displays.emplace_back();
  UpdateProfilePanelFromSettings();
  p_display_graphic_->PaintNow();
}

void
MainWindow::OnRemoveDisplay(wxCommandEvent& event)
{
  auto& profile = settings_.GetActiveProfile();
  const auto index = p_view_mapping_data_->GetSelectedRow();
  if (wxNOT_FOUND != index) {
    profile.displays.erase(profile.displays.begin() + index);
    UpdateProfilePanelFromSettings();
    p_display_graphic_->PaintNow();
  } else {
    spdlog::warn("row not selected");
  }
}

void
MainWindow::OnMoveUp(wxCommandEvent& event)
{
  auto& profile = settings_.GetActiveProfile();
  const auto index = p_view_mapping_data_->GetSelectedRow();
  if (wxNOT_FOUND == index) {
    spdlog::warn("row not selected");
  } else if (0 == index) {
    spdlog::warn("row is already at top");
    return; // selection is at the top. do nothing
  } else {
    std::swap(profile.displays[index], profile.displays[index - 1]);
    UpdateProfilePanelFromSettings();

    // Change the selection index to match the item we just moved
    // This is a terrible hack to find the row of the item we just moved
    const auto model = (wxDataViewIndexListModel*)p_view_mapping_data_->GetModel();
    const auto item = model->GetItem(index - 1);
    wxDataViewItemArray items(size_t(1)); // no braced init constructor :(
    items[0] = item;                      // assumed single selection mode
    p_view_mapping_data_->SetSelections(items);
    p_display_graphic_->PaintNow();
  }
}

void
MainWindow::OnMoveDown(wxCommandEvent& event)
{
  auto& profile = settings_.GetActiveProfile();
  const auto index = p_view_mapping_data_->GetSelectedRow();
  if (wxNOT_FOUND == index) {
    spdlog::warn("row not selected");
  } else if ((p_view_mapping_data_->GetItemCount() - 1) == index) {
    spdlog::warn("row is already at bottom");
    return; // selection is at the bottom. do nothing
  } else {
    std::swap(profile.displays[index], profile.displays[index + 1]);
    UpdateProfilePanelFromSettings();

    // Change the selection index to match the item we just moved
    // This is a terrible hack to find the row of the item we just moved
    const auto model = (wxDataViewIndexListModel*)p_view_mapping_data_->GetModel();
    const auto item = (model->GetItem(index + 1));
    wxDataViewItemArray items(size_t(1)); // no braced init constructor :(
    items[0] = item;                      // assumed single selection mode
    p_view_mapping_data_->SetSelections(items);
    p_display_graphic_->PaintNow();
  }
}

void
MainWindow::OnDisplayEdit(wxCommandEvent& event)
{
  // Get the index of the selected row.
  const auto index = p_view_mapping_data_->GetSelectedRow();
  if (wxNOT_FOUND == index) {
    spdlog::warn("row not selected");
    return;
  }

  auto& display = settings_.GetActiveProfile().displays[index];
  // Show the edit pop up while disabling input on main window
  DialogDisplayEdit dlg(this, display);
  int results = dlg.ShowModal();

  if (wxID_OK == results) {
    spdlog::debug("changes accepted");
    dlg.ApplyChanges(display);

    // Update the GUI.
    UpdateProfilePanelFromSettings();
    p_display_graphic_->PaintNow();
  } else {
    spdlog::debug("settings rejected");
  }
}
