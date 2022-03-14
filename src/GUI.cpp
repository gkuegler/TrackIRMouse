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

constexpr std::string_view kVersionNo = "0.8.0";
const std::string kRotationTitle = "bound (degrees)";
const std::string kPaddingTitle = "padding (pixels)";
const wxSize kDefaultButtonSize = wxSize(110, 25);

wxIMPLEMENT_APP(CGUIApp);

// Center app on main display
wxPoint GetOrigin(const int w, const int h) {
  int desktopWidth = GetSystemMetrics(SM_CXMAXIMIZED);
  int desktopHeight = GetSystemMetrics(SM_CYMAXIMIZED);
  return wxPoint((desktopWidth / 2) - (w / 2), (desktopHeight / 2) - (h / 2));
}

bool CGUIApp::OnInit() {
  // Initialize global default loggers
  mylogging::SetUpLogging();

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
    cTextCtrl *textrich = m_frame->m_panel->m_textrich;

    if (event.GetExtraLong() == static_cast<long>(msgcode::close_app)) {
      m_frame->Close(true);
    }

    // Output message in red lettering as an error
    if (event.GetExtraLong() == static_cast<long>(msgcode::red_text)) {
      wxTextAttr attrExisting = textrich->GetDefaultStyle();
      textrich->SetDefaultStyle(wxTextAttr(*wxRED));
      textrich->AppendText(event.GetString());
      textrich->SetDefaultStyle(attrExisting);
    }
    // Output text normally; no error
    else {
      textrich->AppendText(event.GetString());
    }
  });

  m_frame->LoadSettingsFromFile();
  m_frame->UpdateGuiFromSettings();
  m_frame->Show();

  // Start the track IR thread if enabled
  auto usr = config::GetUserData();
  if (usr.trackOnStart) {
    wxCommandEvent event = {};  // blank event to reuse start handler code
    m_frame->m_panel->OnTrackStart(event);
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

int CGUIApp::OnExit() {
  // Stop tracking so window handle can be unregistered.
  if (m_frame->m_pTrackThread) {
    track::TrackStop();
  }
  return 0;
}

//////////////////////////////////////////////////////////////////////
//                            Main Frame                            //
//////////////////////////////////////////////////////////////////////

cFrame::cFrame(wxPoint origin, wxSize dimensions)
    : wxFrame(nullptr, wxID_ANY, "Track IR Mouse", origin, dimensions) {
  wxMenu *menuFile = new wxMenu;
  // TODO: implement open file? this will require use of the registry
  // menuFile->Append(wxID_OPEN, "&Open\tCtrl-O",
  // "Open a new settings file from disk.");
  // menuFile->Append(wxID_SAVE, "&Save\tCtrl-s", "Save the configuration
  // file.");
  // TODO: implement ctrl-s keyboard shortcut.
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
  // SetStatusText("0");

  m_panel = new cPanel(this);
  m_panel->GetSizer()->Fit(this);
  m_panel->Fit();

  Bind(wxEVT_COMMAND_MENU_SELECTED, &cFrame::OnAbout, this, wxID_ABOUT);
  Bind(wxEVT_COMMAND_MENU_SELECTED, &cFrame::OnExit, this, wxID_EXIT);
  Bind(wxEVT_COMMAND_MENU_SELECTED, &cFrame::OnOpen, this, wxID_OPEN);
  Bind(wxEVT_COMMAND_MENU_SELECTED, &cFrame::OnSave, this, wxID_SAVE);
  Bind(wxEVT_COMMAND_MENU_SELECTED, &cFrame::OnReload, this, myID_MENU_RELOAD);
  Bind(wxEVT_COMMAND_MENU_SELECTED, &cFrame::OnSettings, this,
       myID_MENU_SETTINGS);
}

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

void cFrame::OnOpen(wxCommandEvent &event) {
  const char lpFilename[MAX_PATH] = {0};
  DWORD result = GetModuleFileNameA(0, (LPSTR)lpFilename, MAX_PATH);
  wxString defaultFilePath;
  if (result) {
    wxString executablePath(lpFilename, static_cast<size_t>(result));
    defaultFilePath =
        executablePath.substr(0, executablePath.find_last_of("\\/"));
    // defaultFilePath = wxString(lpFilename, static_cast<size_t>(result));
    spdlog::debug("default file path for dialog: {}", defaultFilePath);
  } else {
    defaultFilePath = wxEmptyString;
  }
  wxFileDialog openFileDialog(this, "Open Settings File", defaultFilePath,
                              wxEmptyString, "Toml (*.toml)|*.toml",
                              wxFD_OPEN | wxFD_FILE_MUST_EXIST);

  if (openFileDialog.ShowModal() == wxID_CANCEL) {
    return;  // the user changed their mind...
  }

  // proceed loading the file chosen by the user;
  // this can be done with e.g. wxWidgets input streams:
  wxFileInputStream input_stream(openFileDialog.GetPath());
  if (!input_stream.IsOk()) {
    wxLogError("Cannot open file '%s'.", openFileDialog.GetPath());
    return;
  }
  wxLogError("Method not implemented yet!");
}

void cFrame::OnSave(wxCommandEvent &event) { config::WriteSettingsToFile(); }

void cFrame::LoadSettingsFromFile() {
  constexpr auto filename = "settings.toml";
  try {
    config::LoadSettingsFromFile(filename);
  } catch (const toml::syntax_error &ex) {
    wxLogFatalError(
        "Failed To Parse toml Settings File:\n\n%s\n\nPlease fix error or save "
        "new file.",
        ex.what());
  } catch (const toml::type_error &ex) {
    wxLogFatalError("Incorrect type when loading settings.\n\n%s", ex.what());
  } catch (const std::out_of_range &ex) {
    wxLogFatalError("Missing data.\n\n%s", ex.what());
  } catch (std::runtime_error &ex) {
    wxLogFatalError("%s\n\nWas expecting to open \"%s\"", ex.what(), filename);
  } catch (...) {
    wxLogFatalError(
        "exception has gone unhandled loading and verifying settings");
  }
}

void cFrame::UpdateGuiFromSettings() {
  // Populate GUI With Settings
  m_panel->PopulateComboBoxWithProfiles();
}

void cFrame::OnReload(wxCommandEvent &event) {
  // TODO: urgent, delete existing settings
  // a smarter idea would be to make a settings builder function would make sure
  // that the settings can be loaded first before replacing existing settings
  LoadSettingsFromFile();
  UpdateGuiFromSettings();
}

void cFrame::OnSettings(wxCommandEvent &event) {
  auto userData = config::GetUserData();

  // Show the settings pop up while disabling input on main window
  cSettingsPopup dlg(this, &userData);
  int results = dlg.ShowModal();

  if (wxID_OK == results) {
    spdlog::debug("settings applied.");
    auto &usr = config::GetUserDataMutable();
    usr = userData;
  } else if (wxID_CANCEL == results) {
    spdlog::debug("settings rejected");
  }
}

void cFrame::OnGenerateExample(wxCommandEvent &event) {
  wxLogError("Method not implemented yet!");
}

//////////////////////////////////////////////////////////////////////
//                            Main Panel                            //
//////////////////////////////////////////////////////////////////////

cTextCtrl::cTextCtrl(wxWindow *parent, wxWindowID id, const wxString &value,
                     const wxPoint &pos, const wxSize &size, int style)
    : wxTextCtrl(parent, id, value, pos, size, style) {}

cPanel::cPanel(cFrame *parent) : wxPanel(parent) {
  m_parent = parent;

  m_btnStartMouse =
      new wxButton(this, myID_START_TRACK, "Start Mouse", wxDefaultPosition,
                   kDefaultButtonSize, 0, wxDefaultValidator, "");
  m_btnStopMouse =
      new wxButton(this, myID_STOP_TRACK, "Stop Mouse", wxDefaultPosition,
                   kDefaultButtonSize, 0, wxDefaultValidator, "");

  m_displayGraphic = new cDisplayGraphic(this, wxSize(650, 200));

  wxStaticText *txtProfiles =
      new wxStaticText(this, wxID_ANY, " Active Profile:        ");

  m_cmbProfiles =
      new wxChoice(this, myID_PROFILE_SELECTION, wxDefaultPosition,
                   wxSize(100, 25), 0, 0, wxCB_SORT, wxDefaultValidator, "");

  m_btnAddProfile =
      new wxButton(this, myID_ADD_PROFILE, "Add a Profile", wxDefaultPosition,
                   kDefaultButtonSize, 0, wxDefaultValidator, "");
  m_btnRemoveProfile = new wxButton(this, myID_REMOVE_PROFILE, "Remove Profile",
                                    wxDefaultPosition, kDefaultButtonSize, 0,
                                    wxDefaultValidator, "");
  m_btnDuplicateProfile = new wxButton(
      this, myID_DUPLICATE_PROFILE, "Duplicate Profile", wxDefaultPosition,
      kDefaultButtonSize, 0, wxDefaultValidator, "");

  m_pnlDisplayConfig = new cPanelConfiguration(this);

  wxStaticText *txtLogOutputTitle =
      new wxStaticText(this, wxID_ANY, "Log Output:");

  m_textrich = new cTextCtrl(this, wxID_ANY, "", wxDefaultPosition,
                             wxSize(300, 10), wxTE_RICH | wxTE_MULTILINE);

  m_textrich->SetFont(wxFont(12, wxFONTFAMILY_TELETYPE, wxFONTSTYLE_NORMAL,
                             wxFONTWEIGHT_NORMAL));

  // Main Panel Layout
  auto *zrTrackCmds = new wxBoxSizer(wxHORIZONTAL);
  zrTrackCmds->Add(m_btnStartMouse, 0, wxALL, 0);
  zrTrackCmds->Add(m_btnStopMouse, 0, wxALL, 0);

  auto *zrProfCmds = new wxBoxSizer(wxHORIZONTAL);
  zrProfCmds->Add(txtProfiles, 0, wxALIGN_CENTER_VERTICAL, 0);
  zrProfCmds->Add(m_cmbProfiles, 1, wxEXPAND | wxTOP | wxRIGHT, 1);
  zrProfCmds->Add(m_btnAddProfile, 0, wxALL | wxALIGN_CENTER_VERTICAL, 0);
  zrProfCmds->Add(m_btnRemoveProfile, 0, wxALL | wxALIGN_CENTER_VERTICAL, 0);
  zrProfCmds->Add(m_btnDuplicateProfile, 0, wxALL | wxALIGN_CENTER_VERTICAL, 0);

  auto *zrBlockLeft = new wxBoxSizer(wxVERTICAL);
  zrBlockLeft->Add(zrTrackCmds, 0, wxBOTTOM, 10);
  zrBlockLeft->Add(m_displayGraphic, 0, wxALL, 20);
  zrBlockLeft->Add(zrProfCmds, 0, wxEXPAND | wxBOTTOM, 10);
  zrBlockLeft->Add(m_pnlDisplayConfig, 0, wxALL, 0);

  auto *zrLogWindow = new wxBoxSizer(wxVERTICAL);
  zrLogWindow->Add(txtLogOutputTitle, 0, wxBOTTOM, 5);
  zrLogWindow->Add(m_textrich, 1, wxEXPAND, 0);

  auto *topSizer = new wxBoxSizer(wxHORIZONTAL);
  topSizer->Add(zrBlockLeft, 0, wxALL | wxEXPAND, 10);
  topSizer->Add(zrLogWindow, 1, wxALL | wxEXPAND, 5);

  SetSizer(topSizer);

  // Set Control Tooltips
  m_btnStartMouse->SetToolTip(
      wxT("Start controlling mouse with head tracking."));
  m_btnStopMouse->SetToolTip(wxT("Stop control of the mouse."));

  m_btnStartMouse->Bind(wxEVT_BUTTON, &cPanel::OnTrackStart, this);
  m_btnStopMouse->Bind(wxEVT_BUTTON, &cPanel::OnTrackStop, this);
  m_cmbProfiles->Bind(wxEVT_CHOICE, &cPanel::OnActiveProfile, this);
  m_btnAddProfile->Bind(wxEVT_BUTTON, &cPanel::OnAddProfile, this);
  m_btnRemoveProfile->Bind(wxEVT_BUTTON, &cPanel::OnRemoveProfile, this);
  m_btnDuplicateProfile->Bind(wxEVT_BUTTON, &cPanel::OnDuplicateProfile, this);
}

void cPanel::PopulateComboBoxWithProfiles() {
  spdlog::trace("repopulating profiles combobox");
  m_cmbProfiles->Clear();
  for (auto &item : config::GetProfileNames()) {
    m_cmbProfiles->Append(item);
  }

  int index = m_cmbProfiles->FindString(config::GetActiveProfile().name);
  if (wxNOT_FOUND != index) {
    m_cmbProfiles->SetSelection(index);
    m_pnlDisplayConfig->LoadDisplaySettings();
  } else {
    wxFAIL_MSG("unable to find new profile in drop-down");
  }
}

void cPanel::OnTrackStart(wxCommandEvent &event) {
  // TODO: remove race condition
  // use std::unique_ptr<> or std::week_ptr<> & std::shared_ptr<> to create a
  // custom class signaler when the class gets deleted; so does the pointer and
  // its value use a randomly generated number. generate a new number on track
  // start. kind of like a key phob. use rotating codes or rand genkeys. in the
  // thread check validity of key, and exit if not valid.

  if (m_parent->m_pTrackThread) {
    spdlog::warn("Please stop mouse before restarting.");
    wxCommandEvent event = {};
    OnTrackStop(event);
    // wait for destruction of thread
    // thread will set its pointer to NULL upon destruction
    while (true) {
      Sleep(8);
      wxCriticalSectionLocker enter(m_parent->m_pThreadCS);
      if (m_parent->m_pTrackThread == NULL) {
        break;
      }
    }
  }

  // Threads run in detached mode by default.
  m_parent->m_pTrackThread =
      new TrackThread(this->m_parent, this->m_parent->GetHandle());

  if (m_parent->m_pTrackThread->Run() != wxTHREAD_NO_ERROR) {
    wxLogError("Can't run the thread!");
    delete m_parent->m_pTrackThread;
    m_parent->m_pTrackThread = nullptr;
    return;
  }

  spdlog::info("Started Mouse.");

  // after the call to wxThread::Run(), the m_pThread pointer is "unsafe":
  // at any moment the thread may cease to exist (because it completes its
  // work). To avoid dangling pointers ~MyThread() will set m_pThread to
  // nullptr when the thread dies.
}

void cPanel::OnTrackStop(wxCommandEvent &event) {
  // Threads run in detached mode by default.
  // Right is responsible for setting m_pTrackThread to nullptr when it
  // finishes and destroys itself.
  if (m_parent->m_pTrackThread) {
    // This will gracefully exit the tracking loop and return control to the
    // thread. This will cause the thread to die off and delete itself.
    track::TrackStop();
    spdlog::info("Stopped mouse.");
  } else {
    spdlog::warn("Track thread not running!");
  }
}

void cPanel::OnActiveProfile(wxCommandEvent &event) {
  int index = m_cmbProfiles->GetSelection();
  // TODO: make all strings utf-8
  // auto name = std::string((m_cmbProfiles->GetString(index)).mb_str());
  auto name = std::string((m_cmbProfiles->GetString(index)).utf8_str());
  config::SetActiveProfile(name);
  m_pnlDisplayConfig->LoadDisplaySettings();
}

void cPanel::OnAddProfile(wxCommandEvent &event) {
  wxTextEntryDialog dlg(this, "Add Profile",
                        "Specify a Name for the New Profile");
  dlg.SetTextValidator(wxFILTER_ALPHANUMERIC);
  if (dlg.ShowModal() == wxID_OK) {
    wxString value = dlg.GetValue();
    config::AddProfile(std::string(value.mb_str()));
    PopulateComboBoxWithProfiles();
  } else {
    spdlog::debug("Add profile action canceled.");
  }
}

void cPanel::OnRemoveProfile(wxCommandEvent &event) {
  wxArrayString choices;
  for (auto &name : config::GetProfileNames()) {
    choices.Add(name);
  }

  const wxString msg = "Delete a Profile";
  const wxString msg2 = "Press OK";
  wxMultiChoiceDialog dlg(this, msg, msg2, choices, wxOK | wxCANCEL,
                          wxDefaultPosition);

  if (wxID_OK == dlg.ShowModal()) {
    auto selections = dlg.GetSelections();
    for (auto &index : selections) {
      config::RemoveProfile(choices[index].ToStdString());
    }

    m_pnlDisplayConfig->LoadDisplaySettings();
    PopulateComboBoxWithProfiles();
  }
}

void cPanel::OnDuplicateProfile(wxCommandEvent &event) {
  config::DuplicateActiveProfile();
  m_pnlDisplayConfig->LoadDisplaySettings();
  PopulateComboBoxWithProfiles();
}

//////////////////////////////////////////////////////////////////////
//                      Display Settings Panel                      //
//////////////////////////////////////////////////////////////////////

cPanelConfiguration::cPanelConfiguration(cPanel *parent)
    : wxPanel(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize,
              wxSIMPLE_BORDER | wxTAB_TRAVERSAL) {
  m_parent = parent;
  m_titlesMap =
      std::make_unique<config::game_title_map_t>(config::GetTitleIds());
  m_name = new wxTextCtrl(this, myID_PROFILE_NAME, "Lorem Ipsum",
                          wxDefaultPosition, wxSize(200, 20),
                          wxTE_LEFT | wxTE_READONLY, wxDefaultValidator, "");
  // TODO: convert this into a drop down titleMap to select valid ID numbers;
  // also display their titles? probably not necessary couldn't guarantee that a
  // title is correct

  m_profileGameTitle =
      new wxTextCtrl(this, wxID_ANY, "lorem", wxDefaultPosition,
                     wxSize(150, 20), wxTE_READONLY | wxTE_LEFT);

  m_profileID =
      new wxTextCtrl(this, wxID_ANY, "2201576", wxDefaultPosition,
                     wxSize(60, 20), wxTE_LEFT, wxDefaultValidator, "");
  m_btnPickTitle = new wxButton(this, wxID_ANY, "Pick Title", wxDefaultPosition,
                                kDefaultButtonSize, 0, wxDefaultValidator, "");

  // std::vector<std::string> idlist = {"13302", "2025"};
  // auto idChoices = BuildWxArrayString(idlist);

  // m_profileID = new wxComboBox(this, wxID_ANY, "", wxDefaultPosition,
  //                              wxSize(80, 20), idChoices, wxCB_DROPDOWN,
  //                              wxMakeIntegerValidator(&m_ival), "");
  m_useDefaultPadding =
      new wxCheckBox(this, wxID_ANY, "Use Default Padding", wxDefaultPosition,
                     wxDefaultSize, wxCHK_2STATE, wxDefaultValidator, "");
  m_btnAddDisplay = new wxButton(this, wxID_ANY, "+", wxDefaultPosition,
                                 wxSize(30, 30), 0, wxDefaultValidator, "");
  m_btnRemoveDisplay = new wxButton(this, wxID_ANY, "-", wxDefaultPosition,
                                    wxSize(30, 30), 0, wxDefaultValidator, "");
  m_btnMoveUp = new wxButton(this, wxID_ANY, "Up", wxDefaultPosition,
                             wxSize(50, 30), 0, wxDefaultValidator, "");
  m_btnMoveDown = new wxButton(this, wxID_ANY, "Down", wxDefaultPosition,
                               wxSize(50, 30), 0, wxDefaultValidator, "");

  constexpr int kColumnWidth = 70;

  m_tlcMappingData = new wxDataViewListCtrl(
      this, myID_MAPPING_DATA, wxDefaultPosition, wxSize(680, 180),
      wxDV_HORIZ_RULES, wxDefaultValidator);

  m_tlcMappingData->AppendTextColumn("Display #", wxDATAVIEW_CELL_INERT, -1,
                                     wxALIGN_RIGHT, wxDATAVIEW_COL_RESIZABLE);
  m_tlcMappingData->AppendTextColumn("Left", wxDATAVIEW_CELL_EDITABLE,
                                     kColumnWidth, wxALIGN_RIGHT,
                                     wxDATAVIEW_COL_RESIZABLE);
  m_tlcMappingData->AppendTextColumn("Right", wxDATAVIEW_CELL_EDITABLE,
                                     kColumnWidth, wxALIGN_RIGHT,
                                     wxDATAVIEW_COL_RESIZABLE);
  m_tlcMappingData->AppendTextColumn("Top", wxDATAVIEW_CELL_EDITABLE,
                                     kColumnWidth, wxALIGN_RIGHT,
                                     wxDATAVIEW_COL_RESIZABLE);
  m_tlcMappingData->AppendTextColumn("Bottom", wxDATAVIEW_CELL_EDITABLE,
                                     kColumnWidth, wxALIGN_RIGHT,
                                     wxDATAVIEW_COL_RESIZABLE);
  m_tlcMappingData->AppendTextColumn("Left", wxDATAVIEW_CELL_EDITABLE,
                                     kColumnWidth, wxALIGN_RIGHT,
                                     wxDATAVIEW_COL_RESIZABLE);
  m_tlcMappingData->AppendTextColumn("Right", wxDATAVIEW_CELL_EDITABLE,
                                     kColumnWidth, wxALIGN_RIGHT,
                                     wxDATAVIEW_COL_RESIZABLE);
  m_tlcMappingData->AppendTextColumn("Top", wxDATAVIEW_CELL_EDITABLE,
                                     kColumnWidth, wxALIGN_RIGHT,
                                     wxDATAVIEW_COL_RESIZABLE);
  m_tlcMappingData->AppendTextColumn("Bottom", wxDATAVIEW_CELL_EDITABLE,
                                     kColumnWidth, wxALIGN_RIGHT,
                                     wxDATAVIEW_COL_RESIZABLE);

  wxStaticText *txtPanelTitle =
      new wxStaticText(this, wxID_ANY, "Active Profile");
  wxStaticText *txtProfileName = new wxStaticText(this, wxID_ANY, "Name:  ");
  wxStaticText *txtProfileId =
      new wxStaticText(this, wxID_ANY, "TrackIR Profile ID:  ");
  wxStaticText *txtGameTitle =
      new wxStaticText(this, wxID_ANY, "Game Title:  ");
  wxStaticText *txtHeaders =
      new wxStaticText(this, wxID_ANY,
                       "                           |------------- Rotational "
                       "Bounds Mapping -----------|-------------------- "
                       "Display Edge Padding -------------------|");

  wxBoxSizer *row1 = new wxBoxSizer(wxHORIZONTAL);
  row1->Add(txtProfileName, 0, wxALIGN_CENTER_VERTICAL, 0);
  row1->Add(m_name, 0, wxALIGN_CENTER_VERTICAL, 0);
  row1->Add(txtGameTitle, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 10);
  row1->Add(m_profileGameTitle, 0, wxALIGN_CENTER_VERTICAL, 0);
  row1->Add(txtProfileId, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 10);
  row1->Add(m_profileID, 0, wxALIGN_CENTER_VERTICAL, 0);
  row1->Add(m_btnPickTitle, 0, wxALIGN_CENTER_VERTICAL, 0);

  wxBoxSizer *displayControls = new wxBoxSizer(wxHORIZONTAL);
  displayControls->Add(m_btnMoveUp, 0, wxALL, 0);
  displayControls->Add(m_btnMoveDown, 0, wxALL, 0);
  displayControls->Add(m_btnAddDisplay, 0, wxALL, 0);
  displayControls->Add(m_btnRemoveDisplay, 0, wxALL, 0);

  wxBoxSizer *topSizer = new wxBoxSizer(wxVERTICAL);
  topSizer->Add(txtPanelTitle, 0, wxALL, 5);
  topSizer->Add(row1, 0, wxALL | wxEXPAND, 5);
  topSizer->Add(m_useDefaultPadding, 0, wxALL, 5);
  topSizer->Add(displayControls, 0, wxALL, 5);
  topSizer->Add(txtHeaders, 0, wxLEFT, 5);
  topSizer->Add(m_tlcMappingData, 0, wxALL, 5);

  SetSizer(topSizer);

  // Set Tool Tip
  // myButton->SetToolTip(wxT("Some helpful tip for this button")); // button
  // is a wxButton*
  m_name->Bind(wxEVT_TEXT, &cPanelConfiguration::OnName, this);
  m_profileID->Bind(wxEVT_TEXT, &cPanelConfiguration::OnProfileID, this);
  m_btnPickTitle->Bind(wxEVT_BUTTON, &cPanelConfiguration::OnPickTitle, this);
  m_useDefaultPadding->Bind(wxEVT_CHECKBOX,
                            &cPanelConfiguration::OnUseDefaultPadding, this);
  m_btnAddDisplay->Bind(wxEVT_BUTTON, &cPanelConfiguration::OnAddDisplay, this);
  m_btnRemoveDisplay->Bind(wxEVT_BUTTON, &cPanelConfiguration::OnRemoveDisplay,
                           this);
  m_btnMoveUp->Bind(wxEVT_BUTTON, &cPanelConfiguration::OnMoveUp, this);
  m_btnMoveDown->Bind(wxEVT_BUTTON, &cPanelConfiguration::OnMoveDown, this);
  m_tlcMappingData->Bind(wxEVT_DATAVIEW_ITEM_EDITING_DONE,
                         &cPanelConfiguration::OnMappingData, this);
}

void cPanelConfiguration::LoadDisplaySettings() {
  auto profile = config::GetActiveProfile();
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
}

void cPanelConfiguration::OnName(wxCommandEvent &event) {
  wxString text = m_name->GetLineText(0);
  // TODO: change all instances of .mb_str()
  // TODO: start using const?????
  auto &profile = config::GetActiveProfileMutable();
  profile.name = text.ToStdString();
  auto &usr = config::GetUserDataMutable();
  usr.activeProfileName = text.ToStdString();
  m_parent->PopulateComboBoxWithProfiles();
}

void cPanelConfiguration::OnProfileID(wxCommandEvent &event) {
  auto number = m_profileID->GetValue();  // from txt entry inheritted
  long value;
  if (!number.ToLong(&value)) {  // wxWidgets has odd conversions
    // TODO: verify only numbers allowed in profile id text control
    // Text control should only allow numbers.
    spdlog::error("Couldn't convert value to integer.");
    return;
  };
  auto &profile = config::GetActiveProfileMutable();
  profile.profileId = static_cast<int>(value);
  LoadDisplaySettings();
}

void cPanelConfiguration::OnPickTitle(wxCommandEvent &event) {
  // auto map = config::GetTitleIds();
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
    auto &config = config::GetActiveProfileMutable();
    config.profileId = id;

    LoadDisplaySettings();
  }
  return;
}

void cPanelConfiguration::OnUseDefaultPadding(wxCommandEvent &event) {
  auto &profile = config::GetActiveProfileMutable();
  profile.useDefaultPadding = m_useDefaultPadding->IsChecked();
  LoadDisplaySettings();
}

void cPanelConfiguration::OnMappingData(wxDataViewEvent &event) {
  auto &profile = config::GetActiveProfileMutable();

  // finding column
  wxVariant value = event.GetValue();
  int column = event.GetColumn();

  // finding associated row; there is no event->GetRow()!?
  wxDataViewItem item = event.GetItem();
  wxDataViewIndexListModel *model =
      (wxDataViewIndexListModel *)(event.GetModel());
  int row = model->GetRow(item);

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
  m_parent->m_displayGraphic->PaintNow();
}

void cPanelConfiguration::OnAddDisplay(wxCommandEvent &event) {
  auto &profile = config::GetActiveProfileMutable();
  // TODO: can this be emplace back?
  profile.displays.push_back(config::Display({0, 0, 0, 0}, {0, 0, 0, 0}));
  LoadDisplaySettings();
  m_parent->m_displayGraphic->PaintNow();
}
void cPanelConfiguration::OnRemoveDisplay(wxCommandEvent &event) {
  auto &profile = config::GetActiveProfileMutable();
  auto index = m_tlcMappingData->GetSelectedRow();
  if (wxNOT_FOUND != index) {
    profile.displays.erase(profile.displays.begin() + index);
    LoadDisplaySettings();
    m_parent->m_displayGraphic->PaintNow();
  } else {
    wxLogError("Display row not selected.");
  }
}

void cPanelConfiguration::OnMoveUp(wxCommandEvent &event) {
  auto &profile = config::GetActiveProfileMutable();
  auto index = m_tlcMappingData->GetSelectedRow();
  if (wxNOT_FOUND == index) {
    wxLogError("Display row not selected.");
  } else if (0 == index) {
    return;  // selection is at the top. do nothing
  } else {
    std::swap(profile.displays[index], profile.displays[index - 1]);
    LoadDisplaySettings();

    // This is a terrible hack to find the row of the item we just moved
    auto model = (wxDataViewIndexListModel *)m_tlcMappingData->GetModel();
    auto item = (model->GetItem(index - 1));
    wxDataViewItemArray items(size_t(1));  // no braced init constructor :(
    items[0] = item;                       // assumed single selection mode
    m_tlcMappingData->SetSelections(items);
    m_parent->m_displayGraphic->PaintNow();
  }
}
void cPanelConfiguration::OnMoveDown(wxCommandEvent &event) {
  auto &profile = config::GetActiveProfileMutable();
  auto index = m_tlcMappingData->GetSelectedRow();
  if (wxNOT_FOUND == index) {
    wxLogError("Display row not selected.");
  } else if ((m_tlcMappingData->GetItemCount() - 1) == index) {
    return;  // selection is at the bottom. do nothing
  } else {
    std::swap(profile.displays[index], profile.displays[index + 1]);
    LoadDisplaySettings();

    // This is a terrible hack to find the row of the item we just moved
    auto model = (wxDataViewIndexListModel *)m_tlcMappingData->GetModel();
    auto item = (model->GetItem(index + 1));
    wxDataViewItemArray items(size_t(1));  // no braced init constructor :(
    items[0] = item;                       // assumed single selection mode
    m_tlcMappingData->SetSelections(items);
    m_parent->m_displayGraphic->PaintNow();
  }
}
