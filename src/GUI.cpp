/**
 * Main app entry and GUI components.
 *
 * --License Boilerplate Placeholder--
 *
 * TODO section:
 * transform mapping values for head distance
 * use default padding overwrites user padding values
 * allow user to select custom settings file
 * upon not finding settings on load; in blank profile add number of displays
 * that are the same number of monitors detected
 * implement crtl-s save feature
 * remove generate example settings file action
 * generator an icon
 *
 *
 * profile box:
 *   set size limitations for text for inputs
 *   pick number of displays
 *   configuration window?
 *   duplicate profile
 *
 *   maintain selections to move up and down
 *   convert values to doubles in validation step of handle_ event
 *   fix internal override of handling default display padding
 */

// TODO: change the variable styling
// TODO: change the method styleing?
// TODO: change the function styling?
// TODO: add documentation?
// TODO: prune includes

// bug list
//  TODO: empty profile created by default
//  TODO: example monitors don't automatically update with the rest of the GUI
//  TODO: remove duplicate windows hardware methods

#include "GUI.hpp"

#include <wx/bookctrl.h>
#include <wx/colour.h>
#include <wx/dataview.h>
#include <wx/filedlg.h>
#include <wx/popupwin.h>
#include <wx/valnum.h>
#include <wx/wfstream.h>

#include <algorithm>
#include <string>

#include "config.hpp"
#include "gui-dialogs.hpp"
#include "log.hpp"
#include "pipeserver.hpp"
#include "threads.hpp"
#include "trackers.hpp"
#include "types.hpp"
#include "util.hpp"

const constexpr std::string_view k_version_no = "0.9.3";
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

wxIMPLEMENT_APP(App);

// Center app on main display
wxPoint
GetOrigin(const int w, const int h)
{
  int desktopWidth = GetSystemMetrics(SM_CXMAXIMIZED);
  int desktopHeight = GetSystemMetrics(SM_CYMAXIMIZED);
  return wxPoint((desktopWidth / 2) - (w / 2), (desktopHeight / 2) - (h / 2));
}

//////////////////////////////////////////////////////////////////////
//                         Main Application                         //
//////////////////////////////////////////////////////////////////////

bool
App::OnInit()
{
  // Initialize global default loggers
  mylogging::SetUpLogging();

  // App initialization constants
  constexpr int appWidth = 1200;
  constexpr int appHeight = 900;

  // Construct child elements first. The main panel_ contains a text control
  // that is a log target.
  p_frame_ =
    new Frame(GetOrigin(appWidth, appHeight), wxSize(appWidth, appHeight));

  //////////////////////////////////////////////////////////////////////
  //               Messages From Threads Outside Of GUI               //
  //////////////////////////////////////////////////////////////////////

  Bind(wxEVT_THREAD, [this](wxThreadEvent& event) {
    LogWindow* textrich = p_frame_->p_text_rich_;

    switch (static_cast<msgcode>(event.GetInt())) {
      case msgcode::log: {
        const auto& level = event.GetExtraLong();
        const auto& msg = event.GetString();
        if (spdlog::level::critical == level) {
          wxLogFatalError(msg);
        } else if (spdlog::level::err == level) {
          wxLogError(msg);
        } else if (spdlog::level::warn == level) {
          const auto existing_style = textrich->GetDefaultStyle();
          textrich->SetDefaultStyle(wxTextAttr(*wxRED));
          textrich->AppendText(msg);
          textrich->SetDefaultStyle(existing_style);
        } else {
          textrich->AppendText(msg);
        }
      } break;
      // case msgcode::log_normal_text: {
      //   textrich->AppendText(event.GetString());
      // } break;
      // case msgcode::log_red_text: {
      //   const auto existing_style = textrich->GetDefaultStyle();
      //   textrich->SetDefaultStyle(wxTextAttr(*wxRED));
      //   textrich->AppendText(event.GetString());
      //   textrich->SetDefaultStyle(existing_style);
      // } break;
      case msgcode::toggle_tracking: {
        if (p_frame_->p_track_thread_) {
          p_frame_->p_track_thread_->tracker_->toggle_mouse();
        }
      } break;
      case msgcode::set_mode: {
        if (p_frame_->p_track_thread_) {
          p_frame_->p_track_thread_->tracker_->handler_->set_alternate_mode(
            static_cast<mouse_mode>(event.GetExtraLong()));
        }
      } break;
      case msgcode::close_app:
        p_frame_->Close(true);
        break;
      default:
        break;
    }
  });

  p_frame_->InitializeSettings();
  p_frame_->UpdateGuiFromConfig();
  p_frame_->Show();

  // Start the track IR thread if enabled
  auto usr = config::Get()->user_data;
  if (usr.track_on_start) {
    wxCommandEvent event = {}; // blank event to reuse start handler code
    p_frame_->OnStart(event);
  }

  // Start the pipe server thread.
  // Pipe server is only started at first application startup.
  if (usr.pipe_server_enabled) {
    p_frame_->p_server_thread_ = new ControlServerThread(p_frame_);
    if (p_frame_->p_server_thread_->Run() != wxTHREAD_NO_ERROR) {
      spdlog::error("Can't run server thread.");
      delete p_frame_->p_server_thread_;
      p_frame_->p_server_thread_ = nullptr;
    }
  }

  return true;
}

int
App::OnExit()
{
  return 0;
}

//////////////////////////////////////////////////////////////////////
//                            Main Frame                            //
//////////////////////////////////////////////////////////////////////

// TODO: convert to single frame, no separate panel_ class
Frame::Frame(wxPoint origin, wxSize dimensions)
  : wxFrame(nullptr, wxID_ANY, "Track IR Mouse", origin, dimensions)
{
  wxMenu* menuFile = new wxMenu;

  //////////////////////////////////////////////////////////////////////
  //                            Menu Bar                              //
  //////////////////////////////////////////////////////////////////////
  // TODO: implement open file? this will require use of the registry
  // menuFile->Append(wxID_OPEN, "&Open\tCtrl-O",
  // TODO: implement ctrl-s keyboard shortcut.
  // "Open a new settings file from disk.");
  // menuFile->Append(wxID_SAVE, "&Save\tCtrl-s", "Save the configuration
  // file.");
  menuFile->Append(wxID_SAVE);
  menuFile->Append(myID_MENU_RELOAD, "&Reload", "Reload the settings file.");
  menuFile->AppendSeparator();
  menuFile->Append(wxID_EXIT);

  wxMenu* menuEdit = new wxMenu;
  menuEdit->Append(myID_MENU_SETTINGS, "&Settings", "Edit app settings.");

  wxMenu* menuHelp = new wxMenu;
  // menuHelp->AppendSeparator();
  menuHelp->Append(wxID_ABOUT);

  wxMenuBar* menuBar = new wxMenuBar;
  menuBar->Append(menuFile, "&File");
  menuBar->Append(menuEdit, "&Edit");
  menuBar->Append(menuHelp, "&Help");

  SetMenuBar(menuBar);

  CreateStatusBar();
  //////////////////////////////////////////////////////////////////////
  //                               Panel                              //
  //////////////////////////////////////////////////////////////////////
  // SetStatusText("0");
  // TODO: make app expand on show log window
  // TODO: make app retract on hide log window
  // Layout paneles
  // clang-format off
  
  // Panels 
  auto main = new wxPanel(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxBORDER_SIMPLE);
  auto pnlProfile = new wxPanel(main, wxID_ANY, wxDefaultPosition, wxSize(800, 400), wxBORDER_SIMPLE);
  // auto m_pnlDisplayConfig = new wxPanel(profile);???????
  //pnlProfile->SetBackgroundColour(orange);
  

  // Logging Window
  label_text_rich_ = new wxStaticText(main, wxID_ANY, "Log Output:");
  p_text_rich_ = new LogWindow(main, wxID_ANY, "", wxDefaultPosition, wxSize(300, 10), wxTE_RICH | wxTE_MULTILINE);
  p_text_rich_->SetFont(wxFont(12, wxFONTFAMILY_TELETYPE, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));

  // Track Controls
  auto btnStartMouse = new wxButton(main, myID_START_TRACK, "Start Mouse", wxDefaultPosition, k_default_button_size);
  auto btnStopMouse = new wxButton(main, myID_STOP_TRACK, "Stop Mouse", wxDefaultPosition, k_default_button_size);
  auto btnHide = new wxButton(main, wxID_ANY, "Show/Hide Log", wxDefaultPosition, k_default_button_size);

  // Display Graphic
  p_display_graphic_ = new cDisplayGraphic(main, wxSize(650, 200));
  //p_display_graphic_->SetBackgroundColour(blue);

  // Profiles Controls
  auto *txtProfiles = new wxStaticText(pnlProfile, wxID_ANY, " Active Profile: ");
  // TODO: make this choice control visually taller when not dropped down
  p_combo_profiles_ = new wxChoice(pnlProfile, myID_PROFILE_SELECTION, wxDefaultPosition, wxSize(100, 25), 0, 0, wxCB_SORT, wxDefaultValidator, "");
  auto btnAddProfile = new wxButton(pnlProfile, wxID_ANY, "Add a Profile", wxDefaultPosition, k_default_button_size_2);
  auto btnRemoveProfile = new wxButton(pnlProfile, wxID_ANY, "Remove a Profile", wxDefaultPosition, k_default_button_size_2);
  auto btnDuplicateProfile = new wxButton(pnlProfile, wxID_ANY, "Duplicate this Profile", wxDefaultPosition, k_default_button_size_2);

  // Profiles Box
  p_titles_map_ = std::make_unique<config::game_title_map_t>(config::GetTitleIds());
	//titles_ = MakeGameTitleVector(p_titles_map_);
  p_text_name_ = new wxTextCtrl(pnlProfile, wxID_ANY, "Lorem Ipsum", wxDefaultPosition, wxSize(250, 20), wxTE_LEFT);
  p_text_name_->SetMaxLength(k_max_profile_length);
  p_text_profile_game_title_ = new wxTextCtrl(pnlProfile, wxID_ANY, "lorem", wxDefaultPosition, wxSize(200, 20), wxTE_READONLY | wxTE_LEFT);
  p_text_profile_id_ = new wxTextCtrl(pnlProfile, wxID_ANY, "2201576", wxDefaultPosition, wxSize(60, 20), wxTE_LEFT, alphanumeric_validator, "");
  auto btn_pick_title = new wxButton(pnlProfile, wxID_ANY, "Pick Title", wxDefaultPosition, k_default_button_size, 0, wxDefaultValidator, "");
  p_check_use_default_padding_ = new wxCheckBox(pnlProfile, wxID_ANY, "Use Default Padding", wxDefaultPosition, wxDefaultSize, wxCHK_2STATE, wxDefaultValidator, "");
  auto btn_move_up = new wxButton(pnlProfile, wxID_ANY, "Up", wxDefaultPosition, wxSize(50, 30), 0, wxDefaultValidator, "");
  auto btn_move_down = new wxButton(pnlProfile, wxID_ANY, "Down", wxDefaultPosition, wxSize(50, 30), 0, wxDefaultValidator, "");
  auto btn_add_display = new wxButton(pnlProfile, wxID_ANY, "+", wxDefaultPosition, wxSize(50, 30), 0, wxDefaultValidator, "");
  auto btn_remove_display = new wxButton(pnlProfile, wxID_ANY, "-", wxDefaultPosition, wxSize(50, 30), 0, wxDefaultValidator, "");

  constexpr int kColumnWidth = 70;
  p_view_mapping_data_ = new wxDataViewListCtrl(pnlProfile, myID_MAPPING_DATA, wxDefaultPosition, wxSize(680, 180), wxDV_HORIZ_RULES, wxDefaultValidator);
  p_view_mapping_data_->AppendTextColumn("Display #", wxDATAVIEW_CELL_INERT, -1, wxALIGN_RIGHT, wxDATAVIEW_COL_RESIZABLE);
  //int col = 1;
  // 
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

  auto txtPanelTitle = new wxStaticText(pnlProfile, wxID_ANY, "Active Profile - Panel");
  auto txtProfileName = new wxStaticText(pnlProfile, wxID_ANY, "Name:  ");
  auto txtProfileId = new wxStaticText(pnlProfile, wxID_ANY, "Game ID:  ");
  auto txtGameTitle = new wxStaticText(pnlProfile, wxID_ANY, "Game Title:  ");
  auto txtHeaders = new wxStaticText(pnlProfile, wxID_ANY,
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
  zrProfileCmds->Add(btnAddProfile, 0, wxALL | wxALIGN_CENTER_VERTICAL, 0);
  zrProfileCmds->Add(btnRemoveProfile, 0, wxALL | wxALIGN_CENTER_VERTICAL, 0);
  zrProfileCmds->Add(btnDuplicateProfile, 0, wxALL | wxALIGN_CENTER_VERTICAL, 0);

  auto zrProfilePnl = new wxBoxSizer(wxVERTICAL);
  zrProfilePnl->Add(zrProfileCmds, 0, wxBOTTOM | wxEXPAND, 10);
  zrProfilePnl->Add(txtPanelTitle, 0, wxALL, 5);
  zrProfilePnl->Add(zrInfo, 0, wxALL | wxEXPAND, 5);
  zrProfilePnl->Add(p_check_use_default_padding_, 0, wxALL, 5);
  zrProfilePnl->Add(txtHeaders, 0, wxLEFT | wxTOP, 5);
  zrProfilePnl->Add(zrMapping, 0, wxLEFT | wxTOP, 5);

  pnlProfile->SetSizer(zrProfilePnl);

  // Main Panel Layout
  auto *zrTrackCmds = new wxBoxSizer(wxHORIZONTAL);
  zrTrackCmds->Add(btnStartMouse, 0, wxALL, 0);
  zrTrackCmds->Add(btnStopMouse, 0, wxALL, 0);
  zrTrackCmds->Add(btnHide, 0, wxALL, 0);

  auto *zrLogWindow = new wxBoxSizer(wxVERTICAL);
  zrLogWindow->Add(label_text_rich_, 0, wxBOTTOM, 5);
  zrLogWindow->Add(p_text_rich_, 1, wxEXPAND, 0);

  auto *zrApp = new wxBoxSizer(wxVERTICAL);
  zrApp->Add(zrTrackCmds, 0, wxBOTTOM, 10);
  zrApp->Add(p_display_graphic_, 1, wxALL | wxEXPAND, 20);
  zrApp->Add(pnlProfile, 0, wxALL, 5);

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
	//This function is currently only implemented under Windows.
	//It is used in the Windows CE port for detecting hardware button presses.
	// TODO: wrap hotkeys in their own class to ensure destructor is called, or make a table
	hotkey_alternate_mode_ = std::make_unique<GlobalHotkey>(GetHandle(), HOTKEY_ID_SCROLL_ALTERNATE, wxMOD_NONE, VK_F18);  // b key
	Bind(wxEVT_HOTKEY, &Frame::OnGlobalHotkey, this, hotkey_alternate_mode_->profile_id_);


  // Bind Menu Events
  Bind(wxEVT_COMMAND_MENU_SELECTED, &Frame::OnAbout, this, wxID_ABOUT);
  Bind(wxEVT_COMMAND_MENU_SELECTED, &Frame::OnExit, this, wxID_EXIT);
  // Bind(wxEVT_COMMAND_MENU_SELECTED, &Frame::OnOpen, this, wxID_OPEN);
  Bind(wxEVT_COMMAND_MENU_SELECTED, &Frame::OnSave, this, wxID_SAVE);
  Bind(wxEVT_COMMAND_MENU_SELECTED, &Frame::OnReload, this, myID_MENU_RELOAD);
  Bind(wxEVT_COMMAND_MENU_SELECTED, &Frame::OnSettings, this, myID_MENU_SETTINGS);

  // Set Control Tooltips
  btnStartMouse->SetToolTip(wxT("Start controlling mouse with head tracking."));
  btnStopMouse->SetToolTip(wxT("Stop control of the mouse."));

  // Bind Main Controls
  btnStartMouse->Bind(wxEVT_BUTTON, &Frame::OnStart, this);
  btnStopMouse->Bind(wxEVT_BUTTON, &Frame::OnStop, this);
  btnHide->Bind(wxEVT_BUTTON, &Frame::OnShowLog, this);
  
  p_combo_profiles_->Bind(wxEVT_CHOICE, &Frame::OnActiveProfile, this);
  btnAddProfile->Bind(wxEVT_BUTTON, &Frame::OnAddProfile, this);
  btnRemoveProfile->Bind(wxEVT_BUTTON, &Frame::OnRemoveProfile, this);
  btnDuplicateProfile->Bind(wxEVT_BUTTON, &Frame::OnDuplicateProfile, this);

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

// Not implementing till future. Having the user pick their own settings
// filename would require the use of saved data outside this program
// void Frame::OnOpen(wxCommandEvent &event) {
//   const char lpFilename[MAX_PATH] = {0};
//   DWORD result = GetModuleFileNameA(0, (LPSTR)lpFilename, MAX_PATH);
//   wxString defaultFilePath;
//   if (result) {
//     wxString executablePath(lpFilename, static_cast<size_t>(result));
//     defaultFilePath =
//         executablePath.substr(0, executablePath.find_last_of("\\/"));
//     // defaultFilePath = wxString(lpFilename, static_cast<size_t>(result));
//     spdlog::debug("default file path for dialog_: {}", defaultFilePath);
//   } else {
//     defaultFilePath = wxEmptyString;
//   }
//   wxFileDialog openFileDialog(this, "Open Settings File", defaultFilePath,
//                               wxEmptyString, "Toml (*.toml)|*.toml",
//                               wxFD_OPEN | wxFD_FILE_MUST_EXIST);
//   if (openFileDialog.ShowModal() == wxID_CANCEL) {
//     return;  // the user changed their mind...
//   }
//   // proceed loading the file chosen by the user;
//   // this can be done with e.g. wxWidgets input streams:
//   wxFileInputStream input_stream(openFileDialog.GetPath());
//   if (!input_stream.IsOk()) {
//     wxLogError("Cannot open file '%s'.", openFileDialog.GetPath());
//     return;
//   }
//   wxLogError("Method not implemented yet!");
// }

void
Frame::OnSave(wxCommandEvent& event)
{
  config::Get()->save_to_file("settings.toml");
}

void
Frame::OnGlobalHotkey(wxKeyEvent& event)
{
  spdlog::debug("hot key event");
  if (p_track_thread_) {
    p_track_thread_->handler_->toggle_alternate_mode();
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
  // TODO: urgent, delete existing settings
  // a smarter idea would be to make a settings builder function would
  // make sure that the settings can be loaded first before replacing
  // existing settings
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
  // set p_track_thread_ to nullptr.

  bool wait_for_stop = false;
  { // scope critical section locker
    // TODO: find a better way to enter critical section
    wxCriticalSectionLocker enter(p_cs_track_thread);
    if (p_track_thread_) {
      p_track_thread_->tracker_->stop();
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
        if (p_track_thread_ == NULL) {
          break;
        }
      } // leave critical section
    }
  }

  try {
    p_track_thread_ = new TrackThread(this, GetHandle(), config::Get());
  } catch (const std::runtime_error& e) {
    spdlog::error(e.what());
    return;
  }

  if (p_track_thread_->Run() == wxTHREAD_NO_ERROR) { // returns immediately
    spdlog::info("Started Mouse.");
  } else {
    spdlog::error("Can't run the tracking thread!");
    delete p_track_thread_;
    // p_track_thread_ = nullptr; // thread destructor should do this anyway?
    return;
  }
}

void
Frame::OnStop(wxCommandEvent& event)
{
  // Threads run in detached mode by default.
  // The thread is responsible for setting p_track_thread_ to nullptr when
  // it finishes and destroys itself.
  if (p_track_thread_) {
    // This will gracefully exit the tracking loop and return control to
    // the wxThread class. The thread will then finish and and delete
    // itself.
    p_track_thread_->tracker_->stop();
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
    if (number > 180) {
      spdlog::error(
        "Value can't be greater than 180. This is a limitation of the "
        "TrackIR software.");
      return;
    }
    if (number < -180) {
      spdlog::error("Value can't be less than -180. This is a limitation of "
                    "the TrackIR "
                    "software.");
      return;
    }
    profile.displays[row].rotation[column - 1] = static_cast<double>(number);
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
