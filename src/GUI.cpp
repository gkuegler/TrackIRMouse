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
 *   convert values to doubles in validation step of handle event
 *   fix internal override of handling default display padding
 */

// This was a bug some point and needed to define.
// #define _CRT_SECURE_NO_WARNINGS

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
#include "threads.hpp"
#include "track.hpp"
#include "types.hpp"
#include "util.hpp"
#include "watchdog.hpp"

const constexpr std::string_view kVersionNo = "0.8.1";
const std::string kRotationTitle = "bound (Degrees)";
const std::string kPaddingTitle = "padding (Pixels)";
const wxSize kDefaultButtonSize = wxSize(110, 25);
const wxSize kDefaultButtonSize2 = wxSize(150, 25);
const constexpr int kMaxProfileLength = 30;

const wxColor yellow(255, 255, 0);
const wxColor blue(255, 181, 102);
const wxColor pink(198, 102, 255);
const wxColor green(142, 255, 102);
const wxColor orange(102, 201, 255);
const wxTextValidator validatorAlphanumeric(wxFILTER_ALPHANUMERIC);

wxIMPLEMENT_APP(cApp);

// Center app on main display
wxPoint GetOrigin(const int w, const int h) {
  int desktopWidth = GetSystemMetrics(SM_CXMAXIMIZED);
  int desktopHeight = GetSystemMetrics(SM_CYMAXIMIZED);
  return wxPoint((desktopWidth / 2) - (w / 2), (desktopHeight / 2) - (h / 2));
}

bool cApp::OnInit() {
  // Initialize global default loggers
  auto logger = mylogging::MakeLoggerFromStd("main");
  mylogging::SetGlobalLogger(logger);

  // App initialization constants
  constexpr int appWidth = 1200;
  constexpr int appHeight = 800;

  // Construct child elements first. The main panel contains a text control that
  // is a log target.
  m_frame =
      new cFrame(GetOrigin(appWidth, appHeight), wxSize(appWidth, appHeight));

  // Handle and display messages to text control widget sent from outside GUI
  // thread
  Bind(wxEVT_THREAD, [this](wxThreadEvent &event) {
    cTextCtrl *textrich = m_frame->m_textrich;

    if (event.GetExtraLong() == static_cast<long>(msgcode::close_app)) {
      m_frame->Close(true);
    }

    // Output message in red lettering as an error
    if (event.GetExtraLong() == static_cast<long>(msgcode::red_text)) {
      const auto existingStyle = textrich->GetDefaultStyle();
      textrich->SetDefaultStyle(wxTextAttr(*wxRED));
      textrich->AppendText(event.GetString());
      textrich->SetDefaultStyle(existingStyle);
    }
    // Output text normally; no error
    else {
      textrich->AppendText(event.GetString());
    }
  });

  m_frame->InitializeSettings();
  m_frame->UpdateGuiUsingSettings();
  m_frame->Show();

  // Start the track IR thread if enabled
  auto usr = config::Get()->userData;
  if (usr.trackOnStart) {
    wxCommandEvent event = {};  // blank event to reuse start handler code
    m_frame->OnStart(event);
  }

  // Start the watchdog thread
  m_frame->m_pWatchdogThread = new WatchdogThread(m_frame);
  if (usr.watchdogEnabled) {
    if (m_frame->m_pWatchdogThread->Run() != wxTHREAD_NO_ERROR) {
      spdlog::error("Can't run watchdog thread.");
      delete m_frame->m_pWatchdogThread;
      m_frame->m_pTrackThread = nullptr;
    }
  }

  return true;
}

int cApp::OnExit() {
  // Stop tracking so window handle can be unregistered.
  if (m_frame->m_pTrackThread) {
    track::Stop();
  }
  return 0;
}

//////////////////////////////////////////////////////////////////////
//                            Main Frame                            //
//////////////////////////////////////////////////////////////////////

// TODO: convert to single frame, no separate panel class
cFrame::cFrame(wxPoint origin, wxSize dimensions)
    : wxFrame(nullptr, wxID_ANY, "Track IR Mouse", origin, dimensions) {
  wxMenu *menuFile = new wxMenu;

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

  wxMenu *menuEdit = new wxMenu;
  menuEdit->Append(myID_MENU_SETTINGS, "&Settings", "Edit app settings.");

  wxMenu *menuHelp = new wxMenu;
  // menuHelp->AppendSeparator();
  menuHelp->Append(wxID_ABOUT);

  wxMenuBar *menuBar = new wxMenuBar;
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
  pnlProfile->SetBackgroundColour(orange);
  

  // Logging Window
  m_lbtextrich = new wxStaticText(main, wxID_ANY, "Log Output:");
  m_textrich = new cTextCtrl(main, wxID_ANY, "", wxDefaultPosition, wxSize(300, 10), wxTE_RICH | wxTE_MULTILINE);
  m_textrich->SetFont(wxFont(12, wxFONTFAMILY_TELETYPE, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));

  // Track Controls
  auto btnStartMouse = new wxButton(main, myID_START_TRACK, "Start Mouse", wxDefaultPosition, kDefaultButtonSize);
  auto btnStopMouse = new wxButton(main, myID_STOP_TRACK, "Stop Mouse", wxDefaultPosition, kDefaultButtonSize);
  auto btnHide = new wxButton(main, wxID_ANY, "Show/Hide Log", wxDefaultPosition, kDefaultButtonSize);

  // Display Graphic
  m_displayGraphic = new cDisplayGraphic(main, wxSize(650, 200));
  m_displayGraphic->SetBackgroundColour(blue);

  // Profiles Controls
  auto *txtProfiles = new wxStaticText(pnlProfile, wxID_ANY, " Active Profile: ");
  // TODO: make this choice control visually taller when not dropped down
  m_cmbProfiles = new wxChoice(pnlProfile, myID_PROFILE_SELECTION, wxDefaultPosition, wxSize(100, 25), 0, 0, wxCB_SORT, wxDefaultValidator, "");
  auto btnAddProfile = new wxButton(pnlProfile, wxID_ANY, "Add a Profile", wxDefaultPosition, kDefaultButtonSize2);
  auto btnRemoveProfile = new wxButton(pnlProfile, wxID_ANY, "Remove a Profile", wxDefaultPosition, kDefaultButtonSize2);
  auto btnDuplicateProfile = new wxButton(pnlProfile, wxID_ANY, "Duplicate this Profile", wxDefaultPosition, kDefaultButtonSize2);

  // Profiles Box
  m_titlesMap = std::make_unique<config::game_title_map_t>(config::GetTitleIds());
  m_name = new wxTextCtrl(pnlProfile, wxID_ANY, "Lorem Ipsum", wxDefaultPosition, wxSize(200, 20), wxTE_LEFT);
  m_name->SetMaxLength(kMaxProfileLength);
  m_profileGameTitle = new wxTextCtrl(pnlProfile, wxID_ANY, "lorem", wxDefaultPosition, wxSize(150, 20), wxTE_READONLY | wxTE_LEFT);
  m_profileID = new wxTextCtrl(pnlProfile, wxID_ANY, "2201576", wxDefaultPosition, wxSize(60, 20), wxTE_LEFT, validatorAlphanumeric, "");
  auto btnPickTitle = new wxButton(pnlProfile, wxID_ANY, "Pick Title", wxDefaultPosition, kDefaultButtonSize, 0, wxDefaultValidator, "");
  m_useDefaultPadding = new wxCheckBox(pnlProfile, wxID_ANY, "Use Default Padding", wxDefaultPosition, wxDefaultSize, wxCHK_2STATE, wxDefaultValidator, "");
  auto btnMoveUp = new wxButton(pnlProfile, wxID_ANY, "Up", wxDefaultPosition, wxSize(50, 30), 0, wxDefaultValidator, "");
  auto btnMoveDown = new wxButton(pnlProfile, wxID_ANY, "Down", wxDefaultPosition, wxSize(50, 30), 0, wxDefaultValidator, "");
  auto btnAddDisplay = new wxButton(pnlProfile, wxID_ANY, "+", wxDefaultPosition, wxSize(50, 30), 0, wxDefaultValidator, "");
  auto btnRemoveDisplay = new wxButton(pnlProfile, wxID_ANY, "-", wxDefaultPosition, wxSize(50, 30), 0, wxDefaultValidator, "");

  constexpr int kColumnWidth = 70;
  m_tlcMappingData = new wxDataViewListCtrl(pnlProfile, myID_MAPPING_DATA, wxDefaultPosition, wxSize(680, 180), wxDV_HORIZ_RULES, wxDefaultValidator);
  m_tlcMappingData->AppendTextColumn("Display #", wxDATAVIEW_CELL_INERT, -1, wxALIGN_RIGHT, wxDATAVIEW_COL_RESIZABLE);
  //int col = 1;
  // 
  // TODO: add dataview spin class renderer to control
  ////auto dvcol = wxDataViewColumn("1", wxDataViewSpinRenderer(0, 5), col++);
  //auto rend = new wxDataViewSpinRenderer(0, 5);
  //auto tttl = new wxString("titleee");
  //auto dvcol = new wxDataViewColumn(*tttl, rend, 1);

  //m_tlcMappingData->AppendColumn(dvcol);
   m_tlcMappingData->AppendTextColumn("Left", wxDATAVIEW_CELL_EDITABLE, kColumnWidth, wxALIGN_RIGHT, wxDATAVIEW_COL_RESIZABLE);
  m_tlcMappingData->AppendTextColumn("Right", wxDATAVIEW_CELL_EDITABLE, kColumnWidth, wxALIGN_RIGHT, wxDATAVIEW_COL_RESIZABLE);
  m_tlcMappingData->AppendTextColumn("Top", wxDATAVIEW_CELL_EDITABLE, kColumnWidth, wxALIGN_RIGHT, wxDATAVIEW_COL_RESIZABLE);
  m_tlcMappingData->AppendTextColumn("Bottom", wxDATAVIEW_CELL_EDITABLE, kColumnWidth, wxALIGN_RIGHT, wxDATAVIEW_COL_RESIZABLE);
  m_tlcMappingData->AppendTextColumn("Left", wxDATAVIEW_CELL_EDITABLE, kColumnWidth, wxALIGN_RIGHT, wxDATAVIEW_COL_RESIZABLE);
  m_tlcMappingData->AppendTextColumn("Right", wxDATAVIEW_CELL_EDITABLE, kColumnWidth, wxALIGN_RIGHT, wxDATAVIEW_COL_RESIZABLE);
  m_tlcMappingData->AppendTextColumn("Top", wxDATAVIEW_CELL_EDITABLE, kColumnWidth, wxALIGN_RIGHT, wxDATAVIEW_COL_RESIZABLE);
  m_tlcMappingData->AppendTextColumn("Bottom", wxDATAVIEW_CELL_EDITABLE, kColumnWidth, wxALIGN_RIGHT, wxDATAVIEW_COL_RESIZABLE);

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
  zrDisplayControls->Add(btnMoveUp);
  zrDisplayControls->Add(btnMoveDown);
  zrDisplayControls->Add(btnAddDisplay);
  zrDisplayControls->Add(btnRemoveDisplay);

  auto zrMapping = new wxBoxSizer(wxHORIZONTAL);
  zrMapping->Add(m_tlcMappingData, 0, wxALL, 5);
  zrMapping->Add(zrDisplayControls, 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);

  auto zrInfo = new wxFlexGridSizer(4, 5, 5);
  zrInfo->Add(txtProfileName);
  zrInfo->Add(txtGameTitle);
  zrInfo->Add(txtProfileId);
  zrInfo->AddSpacer(1);
  zrInfo->Add(m_name, 0, wxALIGN_CENTER_VERTICAL, 0);
  zrInfo->Add(m_profileGameTitle, 0, wxALIGN_CENTER_VERTICAL, 0);
  zrInfo->Add(m_profileID, 0, wxALIGN_CENTER_VERTICAL, 0);
  zrInfo->Add(btnPickTitle, 0, wxALIGN_CENTER_VERTICAL, 0);

  auto *zrProfileCmds = new wxBoxSizer(wxHORIZONTAL);
  zrProfileCmds->Add(txtProfiles, 0, wxALIGN_CENTER_VERTICAL, 0);
  zrProfileCmds->Add(m_cmbProfiles, 1, wxEXPAND | wxTOP | wxRIGHT, 1);
  zrProfileCmds->Add(btnAddProfile, 0, wxALL | wxALIGN_CENTER_VERTICAL, 0);
  zrProfileCmds->Add(btnRemoveProfile, 0, wxALL | wxALIGN_CENTER_VERTICAL, 0);
  zrProfileCmds->Add(btnDuplicateProfile, 0, wxALL | wxALIGN_CENTER_VERTICAL, 0);

  auto zrProfilePnl = new wxBoxSizer(wxVERTICAL);
  zrProfilePnl->Add(zrProfileCmds, 0, wxBOTTOM | wxEXPAND, 10);
  zrProfilePnl->Add(txtPanelTitle, 0, wxALL, 5);
  zrProfilePnl->Add(zrInfo, 0, wxALL | wxEXPAND, 5);
  zrProfilePnl->Add(m_useDefaultPadding, 0, wxALL, 5);
  zrProfilePnl->Add(txtHeaders, 0, wxLEFT | wxTOP, 5);
  zrProfilePnl->Add(zrMapping, 0, wxLEFT | wxTOP, 5);

  pnlProfile->SetSizer(zrProfilePnl);

  // Main Panel Layout
  auto *zrTrackCmds = new wxBoxSizer(wxHORIZONTAL);
  zrTrackCmds->Add(btnStartMouse, 0, wxALL, 0);
  zrTrackCmds->Add(btnStopMouse, 0, wxALL, 0);
  zrTrackCmds->Add(btnHide, 0, wxALL, 0);

  auto *zrLogWindow = new wxBoxSizer(wxVERTICAL);
  zrLogWindow->Add(m_lbtextrich, 0, wxBOTTOM, 5);
  zrLogWindow->Add(m_textrich, 1, wxEXPAND, 0);

  auto *zrApp = new wxBoxSizer(wxVERTICAL);
  zrApp->Add(zrTrackCmds, 0, wxBOTTOM, 10);
  zrApp->Add(m_displayGraphic, 1, wxALL | wxEXPAND, 20);
  zrApp->Add(pnlProfile, 0, wxALL, 5);

  auto *top = new wxBoxSizer(wxHORIZONTAL);
  top->Add(zrApp, 2, wxALL | wxEXPAND, 10);
  top->Add(zrLogWindow, 1, wxALL | wxEXPAND, 5);

  main->SetSizer(top);
  main->Fit();

  // Bind Menu Events
  Bind(wxEVT_COMMAND_MENU_SELECTED, &cFrame::OnAbout, this, wxID_ABOUT);
  Bind(wxEVT_COMMAND_MENU_SELECTED, &cFrame::OnExit, this, wxID_EXIT);
  // Bind(wxEVT_COMMAND_MENU_SELECTED, &cFrame::OnOpen, this, wxID_OPEN);
  Bind(wxEVT_COMMAND_MENU_SELECTED, &cFrame::OnSave, this, wxID_SAVE);
  Bind(wxEVT_COMMAND_MENU_SELECTED, &cFrame::OnReload, this, myID_MENU_RELOAD);
  Bind(wxEVT_COMMAND_MENU_SELECTED, &cFrame::OnSettings, this, myID_MENU_SETTINGS);

  // Set Control Tooltips
  btnStartMouse->SetToolTip(wxT("Start controlling mouse with head tracking."));
  btnStopMouse->SetToolTip(wxT("Stop control of the mouse."));

  // Bind Main Controls
  btnStartMouse->Bind(wxEVT_BUTTON, &cFrame::OnStart, this);
  btnStopMouse->Bind(wxEVT_BUTTON, &cFrame::OnStop, this);
  btnHide->Bind(wxEVT_BUTTON, &cFrame::OnShowLog, this);
  
  m_cmbProfiles->Bind(wxEVT_CHOICE, &cFrame::OnActiveProfile, this);
  btnAddProfile->Bind(wxEVT_BUTTON, &cFrame::OnAddProfile, this);
  btnRemoveProfile->Bind(wxEVT_BUTTON, &cFrame::OnRemoveProfile, this);
  btnDuplicateProfile->Bind(wxEVT_BUTTON, &cFrame::OnDuplicateProfile, this);

  // Bind Profile Info Controls
  m_name->Bind(wxEVT_TEXT, &cFrame::OnName, this);
  m_profileID->Bind(wxEVT_TEXT, &cFrame::OnProfileID, this);
  btnPickTitle->Bind(wxEVT_BUTTON, &cFrame::OnPickTitle, this);
  m_useDefaultPadding->Bind(wxEVT_CHECKBOX, &cFrame::OnUseDefaultPadding, this);
  m_tlcMappingData->Bind(wxEVT_DATAVIEW_ITEM_EDITING_DONE, &cFrame::OnMappingData, this);
  btnAddDisplay->Bind(wxEVT_BUTTON, &cFrame::OnAddDisplay, this);
  btnRemoveDisplay->Bind(wxEVT_BUTTON, &cFrame::OnRemoveDisplay, this);
  btnMoveUp->Bind(wxEVT_BUTTON, &cFrame::OnMoveUp, this);
  btnMoveDown->Bind(wxEVT_BUTTON, &cFrame::OnMoveDown, this);
}
// clang-format on

void cFrame::OnExit(wxCommandEvent &event) { Close(true); }

void cFrame::OnAbout(wxCommandEvent &event) {
  std::string msg = fmt::format(
      "Version No.  {}\nThis is a mouse control application using head "
      "tracking.\n"
      "Author: George Kuegler\n"
      "E-mail: georgekuegler@gmail.com",
      kVersionNo);

  wxMessageBox(msg, "About TrackIRMouse", wxOK | wxICON_NONE);
}

// Not implementing till future. Having the user pick their own settings
// filename would require the use of saved data outside this program
// void cFrame::OnOpen(wxCommandEvent &event) {
//   const char lpFilename[MAX_PATH] = {0};
//   DWORD result = GetModuleFileNameA(0, (LPSTR)lpFilename, MAX_PATH);
//   wxString defaultFilePath;
//   if (result) {
//     wxString executablePath(lpFilename, static_cast<size_t>(result));
//     defaultFilePath =
//         executablePath.substr(0, executablePath.find_last_of("\\/"));
//     // defaultFilePath = wxString(lpFilename, static_cast<size_t>(result));
//     spdlog::debug("default file path for dialog: {}", defaultFilePath);
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

void cFrame::OnSave(wxCommandEvent &event) {
  config::Get()->SaveToFile("settings.toml");
}

void cFrame::InitializeSettings() {
  const std::string filename = "settings.toml";
  config::ConfigReturn result = config::LoadFromFile(filename);
  if (retcode::success == result.code) {
    config::Set(result.config);
  } else {
    const wxString ok = "Load Empty User Settings";
    const wxString cancel = "Quit";
    const wxString instructions = wxString::Format(
        "\n\nPress \"%s\" to load a default user settings template.\nWarning: "
        "data may be overwritten if you "
        "continue with this option and then later save.\n"
        "Press \"%s\" to exit the program.",
        ok, cancel);
    auto dlg = wxMessageDialog(this, result.err_msg + instructions, "Error",
                               wxICON_ERROR | wxOK | wxCANCEL);
    dlg.SetOKCancelLabels(ok, cancel);

    // display reason for error to user
    // give user the chance to quit application (preventing possible data loss
    // and manually fixing the error) or load default/empty config
    if (dlg.ShowModal() == wxID_OK) {
      auto config = config::Config();
      config::Set(config);
    } else {
      spdlog::warn("user closed app when presented with invalid settings load");
      Close(true);
    }
  }
}

void cFrame::UpdateGuiUsingSettings() {
  // Populate GUI With Settings
  PopulateComboBoxWithProfiles();
  LoadDisplaySettings();
}

void cFrame::OnReload(wxCommandEvent &event) {
  // TODO: urgent, delete existing settings
  // a smarter idea would be to make a settings builder function would make sure
  // that the settings can be loaded first before replacing existing settings
  InitializeSettings();
  UpdateGuiUsingSettings();
}

void cFrame::OnSettings(wxCommandEvent &event) {
  auto config = config::Get();
  auto usr = config->userData;

  // Show the settings pop up while disabling input on main window
  cSettingsPopup dlg(this, &usr);
  int results = dlg.ShowModal();

  if (wxID_OK == results) {
    spdlog::debug("settings applied.");
    config->userData = usr;

    auto logger = spdlog::get("main");
    logger->set_level(usr.logLevel);
  } else if (wxID_CANCEL == results) {
    spdlog::debug("settings rejected");
  }
}

void cFrame::PopulateComboBoxWithProfiles() {
  spdlog::trace("repopulating profiles combobox");
  m_cmbProfiles->Clear();
  for (auto &item : config::Get()->GetProfileNames()) {
    m_cmbProfiles->Append(item);
  }

  int index = m_cmbProfiles->FindString(config::Get()->GetActiveProfile().name);
  if (wxNOT_FOUND != index) {
    m_cmbProfiles->SetSelection(index);
    LoadDisplaySettings();
  } else {
    wxFAIL_MSG("unable to find new profile in drop-down");
  }
}

void cFrame::OnStart(wxCommandEvent &event) {
  if (m_pTrackThread) {
    spdlog::warn("Please stop mouse before restarting.");
    wxCommandEvent event = {};
    OnStop(event);
    // wait for destruction of thread
    // thread will set its pointer to NULL upon destruction
    while (true) {
      Sleep(8);
      wxCriticalSectionLocker enter(m_pThreadCS);
      if (m_pTrackThread == NULL) {
        break;
      }
    }
  }

  // Threads run in detached mode by default.
  // TODO: pass in config object
  m_pTrackThread = new TrackThread(this, GetHandle());

  if (m_pTrackThread->Run() != wxTHREAD_NO_ERROR) {
    wxLogError("Can't run the thread!");
    delete m_pTrackThread;
    m_pTrackThread = nullptr;
    return;
  }

  spdlog::info("Started Mouse.");

  // after the call to wxThread::Run(), the m_pThread pointer is "unsafe":
  // at any moment the thread may cease to exist (because it completes its
  // work). To avoid dangling pointers ~MyThread() will set m_pThread to
  // nullptr when the thread dies.
}

void cFrame::OnStop(wxCommandEvent &event) {
  // Threads run in detached mode by default.
  // Right is responsible for setting m_pTrackThread to nullptr when it
  // finishes and destroys itself.
  if (m_pTrackThread) {
    // This will gracefully exit the tracking loop and return control to the
    // thread. This will cause the thread to die off and delete itself.
    track::Stop();
    spdlog::info("Stopped mouse.");
  } else {
    spdlog::warn("Track thread not running!");
  }
}

void cFrame::OnShowLog(wxCommandEvent &event) {
  if (m_textrich->IsShown()) {
    m_textrich->Hide();
    m_lbtextrich->Hide();
  } else {
    m_textrich->Show();
    m_lbtextrich->Show();
  }
}

void cFrame::OnActiveProfile(wxCommandEvent &event) {
  const int index = m_cmbProfiles->GetSelection();
  // TODO: make all strings utf-8
  const auto name = m_cmbProfiles->GetString(index).ToStdString();
  config::Get()->SetActiveProfile(name);
  LoadDisplaySettings();
}

void cFrame::OnAddProfile(wxCommandEvent &event) {
  wxTextEntryDialog dlg(this, "Add Profile",
                        "Specify a Name for the New Profile");
  // dlg.SetTextValidator(wxFILTER_ALPHANUMERIC);
  dlg.SetMaxLength(kMaxProfileLength);
  if (dlg.ShowModal() == wxID_OK) {
    wxString value = dlg.GetValue();
    config::Get()->AddProfile(std::string(value.ToStdString()));
    PopulateComboBoxWithProfiles();
  } else {
    spdlog::debug("Add profile action canceled.");
  }
}

void cFrame::OnRemoveProfile(wxCommandEvent &event) {
  wxArrayString choices;
  for (auto &name : config::Get()->GetProfileNames()) {
    choices.Add(name);
  }

  const wxString msg = "Delete a Profile";
  const wxString msg2 = "Press OK";
  wxMultiChoiceDialog dlg(this, msg, msg2, choices, wxOK | wxCANCEL,
                          wxDefaultPosition);

  if (wxID_OK == dlg.ShowModal()) {
    auto selections = dlg.GetSelections();
    for (auto &index : selections) {
      config::Get()->RemoveProfile(choices[index].ToStdString());
    }

    LoadDisplaySettings();
    PopulateComboBoxWithProfiles();
  }
}

void cFrame::OnDuplicateProfile(wxCommandEvent &event) {
  config::Get()->DuplicateActiveProfile();
  LoadDisplaySettings();
  PopulateComboBoxWithProfiles();
}

void cFrame::LoadDisplaySettings() {
  const auto profile = config::Get()->GetActiveProfile();
  // auto map = config::GetTitleIds();

  // SetValue causes an event to be sent for text control.
  // SetValue does not cause an event to be sent for checkboxes.
  // Use ChangeEvent instead.
  // We don't want to register event when we're just loading values.
  m_name->ChangeValue(profile.name);
  auto *titles = m_titlesMap.get();
  m_profileGameTitle->ChangeValue((*titles)[std::to_string(profile.profileId)]);
  m_profileID->ChangeValue(wxString::Format("%d", profile.profileId));
  m_useDefaultPadding->SetValue(profile.useDefaultPadding);

  m_tlcMappingData->DeleteAllItems();

  int displayNum = 0;

  for (int i = 0; i < profile.displays.size(); i++) {
    wxVector<wxVariant> row;
    row.push_back(wxVariant(wxString::Format("%d", i)));

    for (int j = 0; j < 4; j++) {  // left, right, top, bottom
      row.push_back(wxVariant(
          wxString::Format("%7.2f", profile.displays[i].rotation[j])));
    }
    for (int j = 0; j < 4; j++) {  // left, right, top, bottom
      if (m_useDefaultPadding->IsChecked()) {
        row.push_back(wxVariant(wxString::Format("%s", "(default)")));
      } else {
        row.push_back(
            wxVariant(wxString::Format("%d", profile.displays[i].padding[j])));
      }
    }
    m_tlcMappingData->AppendItem(row);
  }
  // display graphic->PaintNow();
}

void cFrame::OnName(wxCommandEvent &event) {
  const auto text = m_name->GetLineText(0).ToStdString();
  auto &profile = config::Get()->GetActiveProfile();
  profile.name = text;
  config::Get()->userData.activeProfileName = text;
  // TODO: fix parent hierarchy
  // m_parent->PopulateComboBoxWithProfiles();
}

void cFrame::OnProfileID(wxCommandEvent &event) {
  const auto number = m_profileID->GetValue();  // from txt entry inheritted
  long value;
  if (!number.ToLong(&value)) {  // wxWidgets has odd conversions
    spdlog::error("Couldn't convert value to integer.");
    return;
  };
  auto &profile = config::Get()->GetActiveProfile();
  profile.profileId = static_cast<int>(value);
  LoadDisplaySettings();
}

void cFrame::OnPickTitle(wxCommandEvent &event) {
  auto *map = m_titlesMap.get();
  game_titles_t titlesNamed;
  game_titles_t titlesEmptyName;
  titlesNamed.reserve(map->size());
  titlesEmptyName.reserve(map->size());
  // titlesRaw.push_back({"Aerofly", "2025"});
  // titlesRaw.push_back({"FreeSpace2", "13302"});
  for (auto &[key, item] : *map) {
    if (item.empty()) {
      titlesEmptyName.push_back({item, key});
    } else {
      titlesNamed.push_back({item, key});
    }
  }

  std::sort(titlesNamed.begin(), titlesNamed.end(),
            [](const std::pair<std::string, std::string> &left,
               const std::pair<std::string, std::string> &right) {
              return left.first < right.first;
            });

  std::sort(titlesEmptyName.begin(), titlesEmptyName.end(),
            [](const std::pair<std::string, std::string> &left,
               const std::pair<std::string, std::string> &right) {
              return std::stoi(left.second) < std::stoi(right.second);
            });
  game_titles_t titles;
  // preallocate memory
  titles.reserve(titlesNamed.size() + titlesEmptyName.size());
  titles.insert(titles.end(), titlesNamed.begin(), titlesNamed.end());
  titles.insert(titles.end(), titlesEmptyName.begin(), titlesEmptyName.end());

  int id = 0;
  cProfileIdSelector dlg(this, &id, titles);
  if (dlg.ShowModal() == wxID_OK) {
    auto &profile = config::Get()->GetActiveProfile();
    profile.profileId = id;

    LoadDisplaySettings();
  }
  return;
}

void cFrame::OnUseDefaultPadding(wxCommandEvent &event) {
  auto &profile = config::Get()->GetActiveProfile();
  profile.useDefaultPadding = m_useDefaultPadding->IsChecked();
  LoadDisplaySettings();
}

void cFrame::OnMappingData(wxDataViewEvent &event) {
  auto &profile = config::Get()->GetActiveProfile();

  // finding column
  const wxVariant value = event.GetValue();
  const int column = event.GetColumn();

  // finding associated row; there is no event->GetRow()!?
  const wxDataViewItem item = event.GetItem();
  const wxDataViewIndexListModel *model =
      (wxDataViewIndexListModel *)(event.GetModel());
  const int row = model->GetRow(item);

  // if rotation value was selected
  // note 0th column made non-editable
  if (column < 5) {
    double number;  // wxWidgets has odd string conversion procedures
    if (!value.GetString().ToDouble(&number)) {
      spdlog::error("Value could not be converted to a number.");
      return;
    }
    // TODO: use a wxValidator
    // this will prevent the value from sticking in the box on non valid input
    // make numbers only allowed too.
    // validate input
    if (number > 180) {
      spdlog::error(
          "Value can't be greater than 180. This is a limitation of the "
          "TrackIR software.");
      return;
    }
    if (number < -180) {
      spdlog::error(
          "Value can't be less than -180. This is a limitation of "
          "the TrackIR "
          "software.");
      return;
    }
    profile.displays[row].rotation[column - 1] = static_cast<double>(number);
  } else {        // padding value selected
    long number;  // wxWidgets has odd string conversion procedures
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

  LoadDisplaySettings();
  m_displayGraphic->PaintNow();
}

void cFrame::OnAddDisplay(wxCommandEvent &event) {
  auto &profile = config::Get()->GetActiveProfile();
  // TODO: can this be emplace back?
  profile.displays.push_back(config::Display({0, 0, 0, 0}, {0, 0, 0, 0}));
  LoadDisplaySettings();
  m_displayGraphic->PaintNow();
}

void cFrame::OnRemoveDisplay(wxCommandEvent &event) {
  auto &profile = config::Get()->GetActiveProfile();
  // TODO: add const here to index, and everywhere possible
  const auto index = m_tlcMappingData->GetSelectedRow();
  if (wxNOT_FOUND != index) {
    profile.displays.erase(profile.displays.begin() + index);
    LoadDisplaySettings();
    m_displayGraphic->PaintNow();
  } else {
    wxLogError("Display row not selected.");
  }
}

void cFrame::OnMoveUp(wxCommandEvent &event) {
  auto &profile = config::Get()->GetActiveProfile();
  const auto index = m_tlcMappingData->GetSelectedRow();
  if (wxNOT_FOUND == index) {
    wxLogError("Display row not selected.");
  } else if (0 == index) {
    return;  // selection is at the top. do nothing
  } else {
    std::swap(profile.displays[index], profile.displays[index - 1]);
    LoadDisplaySettings();

    // Change the selection index to match the item we just moved
    // This is a terrible hack to find the row of the item we just moved
    const auto model = (wxDataViewIndexListModel *)m_tlcMappingData->GetModel();
    const auto item = model->GetItem(index - 1);
    wxDataViewItemArray items(size_t(1));  // no braced init constructor :(
    items[0] = item;                       // assumed single selection mode
    m_tlcMappingData->SetSelections(items);
    m_displayGraphic->PaintNow();
  }
}
void cFrame::OnMoveDown(wxCommandEvent &event) {
  auto &profile = config::Get()->GetActiveProfile();
  const auto index = m_tlcMappingData->GetSelectedRow();
  if (wxNOT_FOUND == index) {
    wxLogError("Display row not selected.");
  } else if ((m_tlcMappingData->GetItemCount() - 1) == index) {
    return;  // selection is at the bottom. do nothing
  } else {
    std::swap(profile.displays[index], profile.displays[index + 1]);
    LoadDisplaySettings();

    // Change the selection index to match the item we just moved
    // This is a terrible hack to find the row of the item we just moved
    const auto model = (wxDataViewIndexListModel *)m_tlcMappingData->GetModel();
    const auto item = (model->GetItem(index + 1));
    wxDataViewItemArray items(size_t(1));  // no braced init constructor :(
    items[0] = item;                       // assumed single selection mode
    m_tlcMappingData->SetSelections(items);
    m_displayGraphic->PaintNow();
  }
}
