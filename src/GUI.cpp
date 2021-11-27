//#define _CRT_SECURE_NO_WARNINGS
/*
label other settings
label active profile
move msgs to bottom

profile box:
  default padding
  shw brdr
  move to right hand side
  set size limitations for text for inputs
*/
#include "GUI.h"

#include <wx/colour.h>
#include <wx/dataview.h>
#include <wx/filedlg.h>
#include <wx/wfstream.h>

#include "Config.h"
#include "Exceptions.h"
#include "Log.h"
#include "Track.h"

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
    // Output text normally; no errorF
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
  // type_error inherits from toml::exception
  catch (const toml::type_error &ex) {
    wxLogFatalError("Incorrect type when loading settings.\n\n%s", ex.what());
  }
  // toml::exception is base exception class
  catch (const toml::exception &ex) {
    wxLogFatalError("std::exception:\n%s", ex.what());
  } catch (const Exception &ex) {
    wxLogFatalError("My Custom Exception:\n%s", ex.what());
  } catch (...) {
    wxLogFatalError(
        "exception has gone unhandled loading and verifying settings");
  }

  // Populate GUI With Settings
  m_frame->m_panel->PopulateComboBoxWithProfiles(GetGlobalConfigCopy());
  m_frame->m_panel->LoadSettings();
  m_frame->m_panel->m_pnlDisplayConfig->LoadDisplaySettings();

  m_frame->Show();

  // Start the track IR thread if enabled
  if (config->GetBool("General/watchdog_enabled")) {
    wxCommandEvent event = {};
    m_frame->m_panel->OnTrackStart(event);
  }

  return true;
}

//////////////////////////////////////////////////////////////////////
//                            Main Frame                            //
//////////////////////////////////////////////////////////////////////

cFrame::cFrame()
    : wxFrame(nullptr, wxID_ANY, "Track IR Mouse", wxPoint(200, 200),
              wxSize(950, 600)) {
  wxMenu *menuFile = new wxMenu;
  menuFile->Append(wxID_OPEN, "&Open\tCtrl-O",
                   "Open a new settings file from disk.");
  menuFile->AppendSeparator();
  menuFile->Append(wxID_EXIT);

  wxMenu *menuHelp = new wxMenu;
  menuHelp->Append(
      myID_GEN_EXMPL, "Generate Example Settings File",
      "Use this option if the existing settings file has become corrupted");
  menuHelp->AppendSeparator();
  menuHelp->Append(wxID_ABOUT);

  wxMenuBar *menuBar = new wxMenuBar;
  menuBar->Append(menuFile, "&File");
  menuBar->Append(menuHelp, "&Help");

  SetMenuBar(menuBar);

  CreateStatusBar();
  // SetStatusText("0");

  m_panel = new cPanel(this);
  m_panel->GetSizer()->Fit(this);
}

void cFrame::OnExit(wxCommandEvent &event) { Close(true); }

void cFrame::OnAbout(wxCommandEvent &event) {
  std::string msg = R"(This is a mouse control application using head tracking.
Author: George Kuegler
E-mail: georgekuegler@gmail.com)";

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

  wxStaticText *txtGenStgTitle =
      new wxStaticText(this, wxID_ANY, "General Settings:");
  m_cbxEnableWatchdog = new wxCheckBox(
      this, myID_WATCHDOG_ENABLED, "Watchdog Enabled", wxDefaultPosition,
      wxDefaultSize, wxCHK_2STATE, wxDefaultValidator, "");
  m_cbxTrackOnStart = new wxCheckBox(
      this, myID_TRACK_ON_START, "Track On Start", wxDefaultPosition,
      wxDefaultSize, wxCHK_2STATE, wxDefaultValidator, "");
  m_cbxQuitOnLossOfTrackIR = new wxCheckBox(
      this, myID_QUIT_ON_LOSS_OF_TRACK_IR, "Quit On Loss Of Track IR",
      wxDefaultPosition, wxDefaultSize, wxCHK_2STATE, wxDefaultValidator, "");

  wxStaticText *txtTrackLocation1 =
      new wxStaticText(this, wxID_ANY, "Path of 'NPClient64.dll':");
  m_txtTrackIrDllPath =
      new cTextCtrl(this, myID_TRACK_IR_DLL_PATH, "(default)",
                    wxDefaultPosition, wxDefaultSize, wxTE_LEFT);
  wxStaticText *txtTrackLocation2 = new wxStaticText(
      this, wxID_ANY,
      "Note: a value of 'default' will get from install location.");

  m_btnStartMouse =
      new wxButton(this, myID_START_TRACK, "Start Mouse", wxDefaultPosition,
                   kDefaultButtonSize, 0, wxDefaultValidator, "");
  m_btnStopMouse =
      new wxButton(this, myID_STOP_TRACK, "Stop Mouse", wxDefaultPosition,
                   kDefaultButtonSize, 0, wxDefaultValidator, "");
  m_btnSaveSettings =
      new wxButton(this, myID_SAVE_SETTINGS, "Save Settings", wxDefaultPosition,
                   kDefaultButtonSize, 0, wxDefaultValidator, "");

  wxStaticText *txtProfiles =
      new wxStaticText(this, wxID_ANY, "Active Profile:   ");

  m_cmbProfiles = new wxComboBox(this, myID_PROFILE_SELECTION, "",
                                 wxDefaultPosition, wxDefaultSize, 0, 0,
                                 wxCB_DROPDOWN, wxDefaultValidator, "");
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

  // General Settings Box
  wxBoxSizer *zrGenStg = new wxBoxSizer(wxVERTICAL);
  zrGenStg->Add(txtGenStgTitle, 0, wxBOTTOM, 5);
  zrGenStg->Add(m_cbxEnableWatchdog, 0, wxLEFT, 0);
  zrGenStg->Add(m_cbxTrackOnStart, 0, wxLEFT, 0);
  zrGenStg->Add(m_cbxQuitOnLossOfTrackIR, 0, wxLEFT, 0);
  zrGenStg->Add(txtTrackLocation1, 0, wxTOP, 5);
  zrGenStg->Add(m_txtTrackIrDllPath, 1, wxALL | wxEXPAND, 0);
  zrGenStg->Add(txtTrackLocation2, 0, wxALL, 0);

  wxBoxSizer *zrTrackCmds = new wxBoxSizer(wxHORIZONTAL);
  zrTrackCmds->Add(m_btnStartMouse, 0, wxALL, 0);
  zrTrackCmds->Add(m_btnStopMouse, 0, wxALL, 0);
  zrTrackCmds->Add(m_btnSaveSettings, 0, wxALL, 0);

  wxBoxSizer *zrProfCmds = new wxBoxSizer(wxHORIZONTAL);
  zrProfCmds->Add(txtProfiles, 0, wxALIGN_CENTER_VERTICAL, 0);
  zrProfCmds->Add(m_cmbProfiles, 0, wxALIGN_CENTER_VERTICAL, 0);
  zrProfCmds->Add(m_btnAddProfile, 0, wxALL, 0);
  zrProfCmds->Add(m_btnRemoveProfile, 0, wxALL, 0);

  wxBoxSizer *zrBlock1Left = new wxBoxSizer(wxVERTICAL);
  zrBlock1Left->Add(zrTrackCmds, 0, wxBOTTOM, 10);
  zrBlock1Left->Add(zrGenStg, 0, wxALL, 0);

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
}
void cPanel::LoadSettings() {
  CConfig config = GetGlobalConfigCopy();

  m_cbxTrackOnStart->SetValue(config.GetBool("General/track_on_start"));
  m_cbxEnableWatchdog->SetValue(config.GetBool("General/watchdog_enabled"));
  m_cbxQuitOnLossOfTrackIR->SetValue(
      config.GetBool("General/quit_on_loss_of_track_ir"));

  wxString activeProfile = config.GetString("General/active_profile");
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

  LogToWix("Started Mouse.\n")

  // after the call to wxThread::Run(), the m_pThread pointer is "unsafe":
  // at any moment the thread may cease to exist (because it completes its
  // work). To avoid dangling pointers ~MyThread() will set m_pThread to nullptr
  // when the thread dies.
}

void cPanel::OnTrackStop(wxCommandEvent &event) {
  // Threads run in detached mode by default.
  // Right is responsible for setting m_pTrackThread to nullptr when it finishes
  // and destroys itself.
  if (m_parent->m_pTrackThread) {
    TR_TrackStop();
    LogToWix("Stopped mouse.")
    // wxCriticalSectionLocker enter(m_parent->m_pThreadCS);
    // delete m_parent->m_pTrackThread;
    // m_parent->m_pTrackThread = nullptr;
  } else {
    LogToWix("Track thread not running!\n");
  }
}

void cPanel::OnEnabledWatchdog(wxCommandEvent &event) {
  CConfig *config = GetGlobalConfig();
  config->SetValue("General/watchdog_enabled",
                   m_cbxEnableWatchdog->IsChecked());
}

void cPanel::OnTrackOnStart(wxCommandEvent &event) {
  CConfig *config = GetGlobalConfig();
  config->SetValue("General/track_on_start", m_cbxTrackOnStart->IsChecked());
}

void cPanel::OnQuitOnLossOfTrackIr(wxCommandEvent &event) {
  CConfig *config = GetGlobalConfig();
  config->SetValue("General/quit_on_loss_of_track_ir",
                   m_cbxQuitOnLossOfTrackIR->IsChecked());
}

void cPanel::OnTrackIrDllPath(wxCommandEvent &event) {
  CConfig *config = GetGlobalConfig();
  wxString wxsPath = m_txtTrackIrDllPath->GetLineText(0);
  std::string path(wxsPath.mb_str());
  config->SetValue("General/trackir_dll_dIirectory", path);
}

void cPanel::OnActiveProfile(wxCommandEvent &event) {
  int index = m_cmbProfiles->GetSelection();
  CConfig *config = GetGlobalConfig();
  std::string activeProfile((m_cmbProfiles->GetString(index)).mb_str());
  config->LoadActiveProfile(activeProfile);
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
    PopulateComboBoxWithProfiles(*config);
    unsigned int count = m_cmbProfiles->GetCount();
    m_cmbProfiles->SetSelection(count - 1, count - 1);
  }
}

void cPanel::OnRemoveProfile(wxCommandEvent &event) {
  CConfig *config = GetGlobalConfig();
  CProfile profile = GetGlobalConfig()->GetActiveProfile();
  config->RemoveProfile(profile.m_name);
  LogToWix(fmt::format("Profile Removed: {}", profile.m_name));
}

void cPanel::OnSaveSettings(wxCommandEvent &event) {
  CConfig *config = GetGlobalConfig();
  config->SaveSettings();
}

//////////////////////////////////////////////////////////////////////
//                      Display Settings Panel                      //
//////////////////////////////////////////////////////////////////////

cPanelConfiguration::cPanelConfiguration(wxPanel *panel)
    : wxPanel(panel, wxID_ANY, wxDefaultPosition, wxDefaultSize,
              wxSIMPLE_BORDER | wxTAB_TRAVERSAL) {
  m_name = new wxTextCtrl(this, wxID_ANY, "Lorem Ipsum", wxDefaultPosition,
                          wxSize(200, 20), wxTE_LEFT, wxDefaultValidator, "");
  m_profileID =
      new wxTextCtrl(this, wxID_ANY, "2201576", wxDefaultPosition,
                     wxSize(60, 20), wxTE_LEFT, wxDefaultValidator, "");
  m_useDefaultPadding =
      new wxCheckBox(this, wxID_ANY, "Use Default Padding", wxDefaultPosition,
                     wxDefaultSize, wxCHK_2STATE, wxDefaultValidator, "");

  m_tlcMappingData = new wxDataViewListCtrl(this, wxID_ANY, wxDefaultPosition,
                                            wxSize(500, 200), wxDV_HORIZ_RULES,
                                            wxDefaultValidator);

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
}

void cPanelConfiguration::LoadDisplaySettings() {
  CConfig config = GetGlobalConfigCopy();
  auto &dpConfig = config.m_activeProfile;

  m_name->SetValue(dpConfig.m_name);
  m_profileID->SetValue(wxString::Format("%d", dpConfig.m_profile_ID));
  m_useDefaultPadding->SetValue(dpConfig.m_useDefaultPadding);

  std::vector<std::string> names = {"left", "right", "top", "bottom"};
  m_tlcMappingData->DeleteAllItems();

  int displayNum = 0;
  for (auto &display : dpConfig.m_bounds) {
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

    if (1 == result && config.GetBool("General/quit_on_loss_of_track_ir")) {
      CloseApplication();
    }

  } catch (const Exception &ex) {
    wxLogError("Exception happened when starting track thread:\n%s", ex.what());
    return NULL;
  }

  return NULL;
}
