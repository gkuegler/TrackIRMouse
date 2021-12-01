/*
label other settings
label active profile
move msgs to bottom
create advanced settings dialog
    settings menu bar
remove profile should open up a dialogue with a list box
automatically restart tracking
transform mapping values for head distance

profile box:
  set size limitations for text for inputs
  pick number of displays
  configuration window?
  duplicate profile

The Algorithm Design Manual - Steven S. Skiena
*/

// This was a bug some point and needed to define.
// #define _CRT_SECURE_NO_WARNINGS

#include "GUI.h"

#include <wx/bookctrl.h>
#include <wx/colour.h>
#include <wx/dataview.h>
#include <wx/filedlg.h>
#include <wx/popupwin.h>
#include <wx/wfstream.h>

#include <string>

#include "Config.h"
#include "Exceptions.h"
#include "GUIDialogs.h"
#include "Log.h"
#include "Track.h"
constexpr std::string_view kVersionNo = "0.6.0";
const wxSize kDefaultButtonSize = wxSize(100, 25);

wxIMPLEMENT_APP(CGUIApp);

bool CGUIApp::OnInit() {
  // Initialize global logger
  auto logger = spdlog::basic_logger_mt("mainlogger", "log-trackir.txt", true);
  spdlog::set_level(spdlog::level::info);

  // Construct child elements
  m_frame = new cFrame();

  // Build out this function in order to past more than
  // log messages back to my main application
  Bind(wxEVT_THREAD, [this](wxThreadEvent &event) {
    cTextCtrl *textrich = m_frame->m_panel->m_textrich;

    if (1 == event.GetInt()) {
      m_frame->Close(true);
    }

    // Output message in red lettering as an error
    if (1 == event.GetExtraLong()) {
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

  CConfig *config = GetGlobalConfig();

  // Parse Settings File
  try {
    // wxLogError("press okay to continue");
    config->ParseFile("settings.toml");
  } catch (const toml::syntax_error &ex) {
    wxLogFatalError("Failed To Parse toml Settings File:\n%s", ex.what());
  } catch (std::runtime_error &ex) {
    wxLogFatalError(
        "Failed To Parse Settings File.\n\"settings.toml\" File Likely Not "
        "Found.\n%s",
        ex.what());
  } catch (...) {
    wxLogFatalError("An unhandled exception occurred when loading toml file.");
  }

  try {
    config->LoadSettings();
  }
  // type_error inherits from toml::exception, needs to be caught first
  catch (const toml::type_error &ex) {
    wxLogFatalError("Incorrect type when loading settings.\n\n%s", ex.what());
  }
  // toml::exception is base exception class
  catch (const toml::exception &ex) {
    wxLogFatalError("std::exception:\n%s", ex.what());
  } catch (const Exception &ex) {
    wxLogFatalError("My Custom Exception:\n%s", ex.what());
  } /*catch (...) {
    wxLogFatalError(
        "exception has gone unhandled loading and verifying settings");
  }*/

  // Populate GUI With Settings
  m_frame->m_panel->PopulateComboBoxWithProfiles(GetGlobalConfigCopy());
  m_frame->m_panel->PopulateSettings();
  m_frame->m_panel->m_pnlDisplayConfig->LoadDisplaySettings();

  m_frame->Show();

  // Start the track IR thread if enabled
  if (config->data.trackOnStart) {
    LogToFile(fmt::format(
        "checking the start track at start up -> watchdogEnabled: {}",
        config->data.trackOnStart));
    wxCommandEvent event = {};  // blank event to reuse start handler
    m_frame->m_panel->OnTrackStart(event);
  }

  return true;
}

// cRemoveProfile::cRemoveProfile(cFrame *parent)
//     : wxMultiChoiceDialog(parent, "delete this profile",
//                           "press enter to delete the profile", 0,
//                           std::vector<std::string>{}, wxOK,
//                           wxDefaultPosition) {
// }

//////////////////////////////////////////////////////////////////////
//                            Main Frame                            //
//////////////////////////////////////////////////////////////////////

cFrame::cFrame()
    : wxFrame(nullptr, wxID_ANY, "Track IR Mouse", wxPoint(200, 200),
              wxSize(1050, 600)) {
  wxMenu *menuFile = new wxMenu;
  // menuFile->Append(wxID_OPEN, "&Open\tCtrl-O",
  // "Open a new settings file from disk.");
  menuFile->Append(wxID_SAVE, "&Save\tCtrl-s", "Save the configuration file.");
  menuFile->AppendSeparator();
  menuFile->Append(wxID_EXIT);

  wxMenu *menuEdit = new wxMenu;
  menuEdit->Append(myID_SETTINGS, "&Settings", "Edit settings.");

  wxMenu *menuHelp = new wxMenu;
  menuHelp->Append(
      myID_GEN_EXMPL, "Generate Example Settings File",
      "Use this option if the existing settings file has become corrupted");
  menuHelp->AppendSeparator();
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

  // m_settingsPopup = new cSettingsPopup(this);
  // m_settingsPopup->GetSizer()->Fit(this);
}

void cFrame::OnExit(wxCommandEvent &event) { Close(true); }

void cFrame::OnAbout(wxCommandEvent &event) {
  std::string msg = fmt::format(
      "Version No.:  {}\nThis is a mouse control application using head "
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
    LogToWix(defaultFilePath);
  } else {
    defaultFilePath = wxEmptyString;
  }

  wxFileDialog openFileDialog(this, "Open Settings File", defaultFilePath,
                              wxEmptyString, "Toml (*.toml)|*.toml",
                              wxFD_OPEN | wxFD_FILE_MUST_EXIST);

  if (openFileDialog.ShowModal() == wxID_CANCEL)
    return;  // the user changed their mind...

  // proceed loading the file chosen by the user;
  // this can be done with e.g. wxWidgets input streams:
  wxFileInputStream input_stream(openFileDialog.GetPath());
  if (!input_stream.IsOk()) {
    wxLogError("Cannot open file '%s'.", openFileDialog.GetPath());
    return;
  }
  wxLogError("Method not implemented yet!");
}

void cFrame::OnSave(wxCommandEvent &event) {
  CConfig config = GetGlobalConfigCopy();
  config.SaveSettings();
}

void cFrame::OnSettings(wxCommandEvent &event) {
  CConfig configd = GetGlobalConfigCopy();
  SData userData = configd.data;

  // Frame inherits from window
  cSettingsPopup dlg(this, &userData);

  // Show the settings pop up while disabling input on main window
  int results = dlg.ShowModal();
  LogToFile(fmt::format("results of Modal settings: {}", results));

  if (wxID_OK == results) {
    LogToFile(fmt::format("user clicked okay on settings"));
    // values should be saved
    CConfig *config = GetGlobalConfig();
    config->data = userData;
  } else if (wxID_CANCEL == results) {
    LogToFile(fmt::format("user clicked the cancel button on settings"));
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

cPanel::cPanel(cFrame *frame) : wxPanel(frame) {
  m_parent = frame;

  m_btnStartMouse =
      new wxButton(this, myID_START_TRACK, "Start Mouse", wxDefaultPosition,
                   kDefaultButtonSize, 0, wxDefaultValidator, "");
  m_btnStopMouse =
      new wxButton(this, myID_STOP_TRACK, "Stop Mouse", wxDefaultPosition,
                   kDefaultButtonSize, 0, wxDefaultValidator, "");

  wxStaticText *txtProfiles =
      new wxStaticText(this, wxID_ANY, "Active Profile:   ");

  m_cmbProfiles =
      new wxChoice(this, myID_PROFILE_SELECTION, wxDefaultPosition,
                   wxDefaultSize, 0, 0, wxCB_SORT, wxDefaultValidator, "");

  m_btnAddProfile =
      new wxButton(this, myID_ADD_PROFILE, "Add Profile", wxDefaultPosition,
                   kDefaultButtonSize, 0, wxDefaultValidator, "");
  m_btnRemoveProfile = new wxButton(this, myID_REMOVE_PROFILE, "Remove Profile",
                                    wxDefaultPosition, kDefaultButtonSize, 0,
                                    wxDefaultValidator, "");

  m_pnlDisplayConfig = new cPanelConfiguration(this);

  // wxString start_message(fmt::format("{:-^50}\n", "MouseTrackIR
  // Application"));
  wxStaticText *txtLogOutputTitle =
      new wxStaticText(this, wxID_ANY, "Log Output:");

  m_textrich = new cTextCtrl(this, wxID_ANY, "", wxDefaultPosition,
                             wxSize(300, 10), wxTE_RICH | wxTE_MULTILINE);

  m_textrich->SetFont(wxFont(12, wxFONTFAMILY_TELETYPE, wxFONTSTYLE_NORMAL,
                             wxFONTWEIGHT_NORMAL));

  wxBoxSizer *zrTrackCmds = new wxBoxSizer(wxHORIZONTAL);
  zrTrackCmds->Add(m_btnStartMouse, 0, wxALL, 0);
  zrTrackCmds->Add(m_btnStopMouse, 0, wxALL, 0);

  wxBoxSizer *zrProfCmds = new wxBoxSizer(wxHORIZONTAL);
  zrProfCmds->Add(txtProfiles, 0, wxALIGN_CENTER_VERTICAL, 0);
  zrProfCmds->Add(m_cmbProfiles, 0, wxALIGN_CENTER_VERTICAL, 0);
  zrProfCmds->Add(m_btnAddProfile, 0, wxALL, 0);
  zrProfCmds->Add(m_btnRemoveProfile, 0, wxALL, 0);

  wxBoxSizer *zrBlock1Left = new wxBoxSizer(wxVERTICAL);
  zrBlock1Left->Add(zrTrackCmds, 0, wxBOTTOM, 10);

  wxBoxSizer *zrBlock1 = new wxBoxSizer(wxHORIZONTAL);
  zrBlock1->Add(zrBlock1Left, 0, wxALL, 0);
  // Future use is for graphical display
  // zrBlock1->Add(zrBlock1Right, 0, wxALL, 10);

  wxBoxSizer *zrBlock2Left = new wxBoxSizer(wxVERTICAL);
  zrBlock2Left->Add(zrProfCmds, 0, wxBOTTOM, 10);
  zrBlock2Left->Add(m_pnlDisplayConfig, 0, wxALL, 0);

  wxBoxSizer *zrBlock2Right = new wxBoxSizer(wxVERTICAL);
  zrBlock2Right->Add(txtLogOutputTitle, 0, wxBOTTOM, 5);
  zrBlock2Right->Add(m_textrich, 1, wxEXPAND, 0);

  wxBoxSizer *zrBlock2 = new wxBoxSizer(wxHORIZONTAL);
  zrBlock2->Add(zrBlock2Left, 0, wxALL | wxEXPAND, 5);
  zrBlock2->Add(zrBlock2Right, 1, wxALL | wxEXPAND, 5);

  wxBoxSizer *topSizer = new wxBoxSizer(wxVERTICAL);
  topSizer->Add(zrBlock1, 0, wxALL | wxEXPAND, 10);
  topSizer->Add(zrBlock2, 0, wxALL | wxEXPAND, 10);

  SetSizer(topSizer);

  // Set Control Tooltips
  m_btnStartMouse->SetToolTip(
      wxT("Start controlling mouse with head tracking."));
  m_btnStopMouse->SetToolTip(wxT("Stop control of the mouse."));
}

void cPanel::PopulateComboBoxWithProfiles(CConfig config) {
  m_cmbProfiles->Clear();
  for (auto &item : config.GetProfileNames()) {
    m_cmbProfiles->Append(item);
  }
}

void cPanel::PopulateSettings() {
  CConfig config = GetGlobalConfigCopy();
  wxString activeProfile = config.data.activeProfileName;
  LogToFile(fmt::format("activeProfile: {}", activeProfile));
  int index = m_cmbProfiles->FindString(activeProfile);
  if (wxNOT_FOUND == index) {
    wxLogError("active profile not found in available loaded profiles");
  } else {
    m_cmbProfiles->SetSelection(index);
  }
}

void cPanel::OnTrackStart(wxCommandEvent &event) {
  if (m_parent->m_pTrackThread) {
    LogToWixError("Please stop mouse before restarting.\n");
    return;
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

  LogToWix("Started Mouse.\n");

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
    TR_TrackStop();
    LogToWix("Stopped mouse.");
    // wxCriticalSectionLocker enter(m_parent->m_pThreadCS);
    // delete m_parent->m_pTrackThread;
    // m_parent->m_pTrackThread = nullptr;
  } else {
    LogToWix("Track thread not running!\n");
  }
}

void cPanel::OnActiveProfile(wxCommandEvent &event) {
  int index = m_cmbProfiles->GetSelection();
  LogToFile(fmt::format("Profiles get selection: {}", index));
  std::string activeProfile((m_cmbProfiles->GetString(index)).mb_str());
  LogToFile(fmt::format("activeProfile: {}", activeProfile));
  CConfig *config = GetGlobalConfig();
  config->data.activeProfileName = activeProfile;

  m_pnlDisplayConfig->LoadDisplaySettings();
}

void cPanel::OnAddProfile(wxCommandEvent &event) {
  CConfig *config = GetGlobalConfig();
  wxTextEntryDialog dlg(this, "Add Profile",
                        "Specify a Name for the New Profile");
  dlg.SetTextValidator(wxFILTER_ALPHA);
  if (dlg.ShowModal() == wxID_OK) {
    // We can be certain that this string contains letters only.
    wxString value = dlg.GetValue();
    config->AddProfile(std::string(value.mb_str()));
    PopulateComboBoxWithProfiles(GetGlobalConfigCopy());
    unsigned int count = m_cmbProfiles->GetCount();
    // m_cmbProfiles->SetSelection(count - 1, count - 1);
  }
}

void cPanel::OnRemoveProfile(wxCommandEvent &event) {
  // int index = m_cmbProfiles->GetSelection();
  // std::string activeProfile((m_cmbProfiles->GetString(index)).mb_str());
  // CConfig *config = GetGlobalConfig();
  // config->RemoveProfile(activeProfile);
  // LogToWix(fmt::format("Profile Removed: {}", activeProfile));
  // m_removeProfile->ShowModal();
  CConfig config = GetGlobalConfigCopy();
  wxArrayString choices;
  for (auto &name : config.GetProfileNames()) {
    choices.Add(name);
  }

  wxString msg = "Delete a Profile";
  wxString msg2 = "Press OK";
  auto dlg = new wxMultiChoiceDialog(this, msg, msg2, choices, wxOK | wxCANCEL,
                                     wxDefaultPosition);
  int result = dlg->ShowModal();

}

//////////////////////////////////////////////////////////////////////
//                      Display Settings Panel                      //
//////////////////////////////////////////////////////////////////////

cPanelConfiguration::cPanelConfiguration(wxPanel *panel)
    : wxPanel(panel, wxID_ANY, wxDefaultPosition, wxDefaultSize,
              wxSIMPLE_BORDER | wxTAB_TRAVERSAL) {
  m_name =
      new wxTextCtrl(this, myID_PROFILE_NAME, "Lorem Ipsum", wxDefaultPosition,
                     wxSize(200, 20), wxTE_LEFT, wxDefaultValidator, "");
  m_profileID =
      new wxTextCtrl(this, myID_PROFILE_ID, "2201576", wxDefaultPosition,
                     wxSize(60, 20), wxTE_LEFT, wxDefaultValidator, "");
  m_useDefaultPadding = new wxCheckBox(
      this, myID_USE_DEFAULT_PADDING, "Use Default Padding", wxDefaultPosition,
      wxDefaultSize, wxCHK_2STATE, wxDefaultValidator, "");

  m_tlcMappingData = new wxDataViewListCtrl(
      this, myID_MAPPING_DATA, wxDefaultPosition, wxSize(500, 200),
      wxDV_HORIZ_RULES, wxDefaultValidator);

  m_tlcMappingData->AppendTextColumn("Display #");
  m_tlcMappingData->AppendTextColumn("Type");
  m_tlcMappingData->AppendTextColumn("Left", wxDATAVIEW_CELL_EDITABLE);
  m_tlcMappingData->AppendTextColumn("Right", wxDATAVIEW_CELL_EDITABLE);
  m_tlcMappingData->AppendTextColumn("Top", wxDATAVIEW_CELL_EDITABLE);
  m_tlcMappingData->AppendTextColumn("Bottom", wxDATAVIEW_CELL_EDITABLE);

  wxStaticText *txtPanelTitle =
      new wxStaticText(this, wxID_ANY, "Active Profile");
  wxStaticText *txtProfileName = new wxStaticText(this, wxID_ANY, "Name:   ");
  wxStaticText *txtProfileId =
      new wxStaticText(this, wxID_ANY, "TrackIR Profile ID:   ");

  wxBoxSizer *row1 = new wxBoxSizer(wxHORIZONTAL);
  row1->Add(txtProfileName, 0, wxALIGN_CENTER_VERTICAL, 0);
  row1->Add(m_name, 0, wxALIGN_CENTER_VERTICAL, 0);
  row1->Add(txtProfileId, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 10);
  row1->Add(m_profileID, 0, wxALIGN_CENTER_VERTICAL, 0);

  wxBoxSizer *topSizer = new wxBoxSizer(wxVERTICAL);
  topSizer->Add(txtPanelTitle, 0, wxALL, 5);
  topSizer->Add(row1, 0, wxALL | wxEXPAND, 5);
  topSizer->Add(m_useDefaultPadding, 0, wxALL, 5);
  topSizer->Add(m_tlcMappingData, 0, wxALL, 5);

  SetSizer(topSizer);

  // Set Tool Tip
  // myButton->SetToolTip(wxT("Some helpful tip for this button")); // button
  // is a wxButton*
}

void cPanelConfiguration::LoadDisplaySettings() {
  CConfig config = GetGlobalConfigCopy();
  auto &profile = config.GetActiveProfile();

  m_name->SetValue(profile.name);
  m_profileID->SetValue(wxString::Format("%d", profile.profile_ID));
  m_useDefaultPadding->SetValue(profile.useDefaultPadding);

  std::vector<std::string> names = {"left", "right", "top", "bottom"};
  m_tlcMappingData->DeleteAllItems();

  int displayNum = 0;
  for (auto &display : profile.bounds) {
    wxVector<wxVariant> rowBound;
    rowBound.push_back(wxVariant(wxString::Format("%d", displayNum)));
    rowBound.push_back(wxVariant("Bound"));
    for (int i = 0; i < 4; i++) {
      rowBound.push_back(
          wxVariant(wxString::Format("%7.2f", display.rotationBounds[i])));
    }

    m_tlcMappingData->AppendItem(rowBound);

    wxVector<wxVariant> rowPadding;
    rowPadding.push_back(wxVariant(wxString::Format("%d", displayNum)));
    rowPadding.push_back(wxVariant("Padding"));
    // Optional Padding Values
    for (int i = 0; i < 4; i++) {
      if (!m_useDefaultPadding->IsChecked()) {
        rowPadding.push_back(
            wxVariant(wxString::Format("%d", display.paddingBounds[i])));
      } else {
        rowPadding.push_back(wxVariant(wxString::Format("%s", "(default)")));
      }
    }
    m_tlcMappingData->AppendItem(rowPadding);

    displayNum++;
  }
}

void cPanelConfiguration::OnName(wxCommandEvent &event) {
  CConfig *config = GetGlobalConfig();
  wxString wxsPath = m_name->GetLineText(0);
  std::string name(wxsPath.mb_str());
  // config->SetProfileName(name);
  // Reload the combo box
}
void cPanelConfiguration::OnProfileID(wxCommandEvent &event) {
  CConfig *config = GetGlobalConfig();
  // config->
}
void cPanelConfiguration::OnUseDefaultPadding(wxCommandEvent &event) {
  CConfig *config = GetGlobalConfig();
  // config->
}
void cPanelConfiguration::OnTlcMappingData(wxCommandEvent &event) {
  CConfig *config = GetGlobalConfig();
  // config->
}

//////////////////////////////////////////////////////////////////////
//                           TrackThread                            //
//////////////////////////////////////////////////////////////////////

TrackThread::TrackThread(cFrame *pHandler, HWND hWnd) : wxThread() {
  m_pHandler = pHandler;
  m_hWnd = hWnd;
}

TrackThread::~TrackThread() {
  // Will need to provide locks in the future with critical sections
  // https://docs.wxwidgets.org/3.0/classwx_thread.html
  wxCriticalSectionLocker enter(m_pHandler->m_pThreadCS);
  m_pHandler->m_pTrackThread = NULL;
}

void CloseApplication() {
  wxThreadEvent *evt = new wxThreadEvent(wxEVT_THREAD);
  evt->SetInt(1);
  wxTheApp->QueueEvent(evt);
}

wxThread::ExitCode TrackThread::Entry() {
  CConfig config = GetGlobalConfigCopy();
  try {
    TR_Initialize(m_hWnd, config);

    // This is the loop function
    int result = TR_TrackStart(config);

    if (1 == result && config.data.quitOnLossOfTrackIr) {
      CloseApplication();
    }

  } catch (const Exception &ex) {
    wxLogError("Exception happened when starting track thread:\n%s", ex.what());
    return NULL;
  }

  return NULL;
}
