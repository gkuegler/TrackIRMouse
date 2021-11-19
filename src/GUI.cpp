#define _CRT_SECURE_NO_WARNINGS

#include "GUI.h"

#include "Track.h"
#include "Config.h"
#include "Log.h"
#include "Exceptions.h"

#include <wx/dataview.h>
#include <wx/colour.h>
#include <wx/filedlg.h>
#include <wx/wfstream.h>


wxIMPLEMENT_APP(CGUIApp);


bool CGUIApp::OnInit()
{
    // Initialize global logger
    auto logger = spdlog::basic_logger_mt("mainlogger", "log-trackir.txt");
    spdlog::set_level(spdlog::level::info);

    // Construct child elements
    m_frame = new cFrame();

    // Build out this function in order to past more than
    // log messages back to my main application
    Bind(wxEVT_THREAD, [this](wxThreadEvent& event)
        {
            cTextCtrl* textrich = m_frame->m_panel->m_textrich;

            // Output message in red lettering as an error
            if (1 == event.GetExtraLong())
            {
                wxTextAttr attrExisting = textrich->GetDefaultStyle();
                textrich->SetDefaultStyle(wxTextAttr(*wxRED));
                textrich->AppendText(event.GetString());
                textrich->SetDefaultStyle(attrExisting);
            }
            // Output text normally; no errorF
            else
            {
                textrich->AppendText(event.GetString());
            }
    });

    CConfig* config = GetGlobalConfig();

    // Parse Settings File
    try
    {
       //wxLogError("press okay to continue");
       config->ParseFile("settings.toml");
    }
    catch (const toml::syntax_error& ex)
    {
       wxLogFatalError("Failed To Parse toml Settings File:\n%s", ex.what());
    }
    catch (std::runtime_error& ex)
    {
        wxLogFatalError("Failed To Parse Settings File.\n\"settings.toml\" File Likely Not Found.\n%s", ex.what());
    }
    catch (...) {
       wxLogFatalError("An unhandled exception occurred when loading toml file.");
    }

    try
    {
        config->LoadSettings();
    }
    // type_error inherits from toml::exception
    catch (const toml::type_error& ex)
    {
        wxLogFatalError("Incorrect type when loading settings.\n\n%s", ex.what());
    }
    // toml::exception is base exception class
    catch (const toml::exception& ex)
    {
        wxLogFatalError("std::exception:\n%s", ex.what());
    }
    catch (const Exception& ex)
    {
        wxLogFatalError("My Custom Exception:\n%s", ex.what());
    }
    catch (...)
    {
        wxLogFatalError("exception has gone unhandled loading and verifying settings");
    }

    // Populate GUI With Settings
    m_frame->m_panel->PopulateComboBoxWithProfiles(GetGlobalConfigCopy());
    m_frame->m_panel->LoadSettings();
    m_frame->m_panel->m_pnlDisplayConfig->LoadDisplaySettings();
    
	m_frame->Show();

    // Start the track IR thread if enabled
    if (config->GetBool("General/watchdog_enabled"))
    {
        wxCommandEvent event = {};
        m_frame->OnTrackStart(event);
    }

	return true;
}

//////////////////////////////////////////////////////////////////////
//                            Main Frame                            //
//////////////////////////////////////////////////////////////////////

cFrame::cFrame() : wxFrame(nullptr, wxID_ANY, "Example Title", wxPoint(200, 200), wxSize(1200, 800))
{

    wxMenu* menuFile = new wxMenu;
    menuFile->Append(wxID_OPEN, "&Open\tCtrl-O",
        "Open a new settings file from disk.");
    menuFile->AppendSeparator();
    menuFile->Append(wxID_EXIT);

    wxMenu* menuHelp = new wxMenu;
    menuHelp->Append(myID_GEN_EXMPL, "Generate Example Settings File", 
        "Use this option if the existing settings file has become corrupted");
    menuHelp->AppendSeparator();
    menuHelp->Append(wxID_ABOUT);

    wxMenuBar* menuBar = new wxMenuBar;
    menuBar->Append(menuFile, "&File");
    menuBar->Append(menuHelp, "&Help");

    SetMenuBar(menuBar);

    CreateStatusBar();
    //SetStatusText("0");

    m_panel = new cPanel(this);
    m_panel->GetSizer()->Fit(this);
}

void cFrame::OnExit(wxCommandEvent& event)
{
    Close(true);
}

void cFrame::OnAbout(wxCommandEvent& event)
{
    std::string msg = R"(This is a mouse control application using head tracking.
Author: George Kuegler
E-mail: georgekuegler@gmail.com)";

    wxMessageBox(msg, "About TrackIRMouse", wxOK | wxICON_NONE);
}

void cFrame::OnOpen(wxCommandEvent& event)
{
    const char lpFilename[MAX_PATH] = {0};

    DWORD result = GetModuleFileNameA(0, (LPSTR)lpFilename, MAX_PATH);

    wxString defaultFilePath;

    if (result)
    {
        wxString executablePath(lpFilename, static_cast<size_t>(result));
        defaultFilePath = executablePath.substr(0, executablePath.find_last_of("\\/"));
        // defaultFilePath = wxString(lpFilename, static_cast<size_t>(result));
        LogToWix(defaultFilePath);
    }
    else
    {
        defaultFilePath = wxEmptyString;
    }

    wxFileDialog openFileDialog(this, "Open Settings File", defaultFilePath, wxEmptyString,
                   "Toml (*.toml)|*.toml", wxFD_OPEN|wxFD_FILE_MUST_EXIST);

    if (openFileDialog.ShowModal() == wxID_CANCEL)
        return;     // the user changed their mind...
    
    // proceed loading the file chosen by the user;
    // this can be done with e.g. wxWidgets input streams:
     wxFileInputStream input_stream(openFileDialog.GetPath());
     if (!input_stream.IsOk())
     {
         wxLogError("Cannot open file '%s'.", openFileDialog.GetPath());
         return;
     }
     wxLogError("Method not implemented yet!");
}

void cFrame::OnGenerateExample(wxCommandEvent& event)
{
    wxLogError("Method not implemented yet!");
}

void cFrame::OnTrackStart(wxCommandEvent& event)
{
    // Threads run in detached mode by default.
    // It is okay to lose pointer.
    m_pTrackThread = new TrackThread(this, this->GetHandle(), GetGlobalConfigCopy());

    if (m_pTrackThread->Create() == wxTHREAD_NO_ERROR)
    {
        m_pTrackThread->Run();
    }
    else
    {
        wxLogError("Can't create the thread!");
        delete m_pTrackThread;
        m_pTrackThread = NULL;
    }
}

void cFrame::OnTrackStop(wxCommandEvent& event)
{
    // Threads run in detached mode by default.
    // It is okay to lose pointer.
    if (m_pTrackThread)
    {
        // delete T_pTrackThread;
        // m_pTrackThread = NULL;
    }
    else
    {
        LogToWix("Track thread not running!\n");
    }
}

//////////////////////////////////////////////////////////////////////
//                            Main Panel                            //
//////////////////////////////////////////////////////////////////////

cTextCtrl::cTextCtrl(wxWindow* parent, wxWindowID id, const wxString& value,
    const wxPoint& pos, const wxSize& size, int style) : wxTextCtrl(parent, id, value, pos, size, style)
{

}

cPanel::cPanel(wxFrame* frame) : wxPanel(frame)
{
    m_cbxEnableWatchdog = new wxCheckBox(this, myID_WATCHDOG_ENABLED, "Watchdog Enabled", wxDefaultPosition, wxDefaultSize, wxCHK_2STATE, wxDefaultValidator, "");
    m_cbxTrackOnStart = new wxCheckBox(this, myID_TRACK_ON_START, "Track On Start", wxDefaultPosition, wxDefaultSize, wxCHK_2STATE, wxDefaultValidator, "");
    m_cbxQuitOnLossOfTrackIR = new wxCheckBox(this, myID_QUIT_ON_LOSS_OF_TRACK_IR, "Quit On Loss Of Track IR", wxDefaultPosition, wxDefaultSize, wxCHK_2STATE, wxDefaultValidator, "");

    wxStaticText* txtTrackLocation1 = new wxStaticText(this, wxID_ANY, "Path of 'NPClient64.dll'");
    m_txtTrackIrDllPath = new cTextCtrl(this, myID_TRACK_IR_DLL_PATH, "(default)", wxDefaultPosition, wxSize(300, 20), wxTE_LEFT);
    wxStaticText* txtTrackLocation2 = new wxStaticText(this, wxID_ANY, "Note: a value of 'default' will get from install location.");

    m_btnStartMouse = new wxButton(this, myID_START_TRACK, "Start Mouse", wxDefaultPosition, wxDefaultSize, 0, wxDefaultValidator, "");
    m_btnStopMouse = new wxButton(this, myID_STOP_TRACK, "Stop Mouse", wxDefaultPosition, wxDefaultSize, 0, wxDefaultValidator, "");
    m_btnSaveSettings = new wxButton(this, myID_SAVE_SETTINGS, "Save Settings", wxDefaultPosition, wxDefaultSize, 0, wxDefaultValidator, "");

    m_cmbProfiles = new wxComboBox(this, myID_PROFILE_SELECTION, "", wxDefaultPosition, wxDefaultSize, 0, 0, wxCB_DROPDOWN, wxDefaultValidator, "");

    // m_tlcMappingData = new wxDataViewListCtrl(this, wxID_ANY, wxDefaultPosition, wxSize(300, 400), wxDV_HORIZ_RULES, wxDefaultValidator);

    // m_tlcMappingData->AppendTextColumn("Display #");
    // m_tlcMappingData->AppendTextColumn("Parameters");
    // m_tlcMappingData->AppendTextColumn("Values", wxDATAVIEW_CELL_EDITABLE);
    
    m_pnlDisplayConfig = new cPanelConfiguration(this);

    wxString start_message(fmt::format("{:-^50}\n", "MouseTrackIR Application"));
    m_textrich = new cTextCtrl(this, wxID_ANY, start_message,
        wxDefaultPosition, wxDefaultSize,
        wxTE_RICH | wxTE_MULTILINE);
    
    m_textrich -> SetFont(wxFont(12, wxFONTFAMILY_TELETYPE, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));

    //wxBoxSizer* rowA = new wxBoxSizer(wxHORIZONTAL);


    wxBoxSizer* row1 = new wxBoxSizer(wxVERTICAL);
    row1->Add(m_cbxEnableWatchdog, 0, wxALL, 0);
    row1->Add(m_cbxTrackOnStart, 0, wxALL, 0);
    row1->Add(m_cbxQuitOnLossOfTrackIR, 0, wxALL, 0);
    row1->Add(txtTrackLocation1, 0, wxALL, 0);
    row1->Add(m_txtTrackIrDllPath, 0, wxALL, 0);
    row1->Add(txtTrackLocation2, 0, wxALL, 0);

    wxBoxSizer* row2 = new wxBoxSizer(wxVERTICAL);
    row2->Add(m_btnStartMouse, 0, wxALL, 0);
    row2->Add(m_btnStopMouse, 0, wxALL, 0);
    row2->Add(m_btnSaveSettings, 0, wxALL, 0);
    row2->Add(m_cmbProfiles, 0, wxALL, 0);
              
    wxBoxSizer* row3 = new wxBoxSizer(wxVERTICAL);
    //row3->Add(tcMappingData, 0, wxALL | wxEXPAND, 0);
    row3->Add(m_pnlDisplayConfig, 0, wxALL | wxEXPAND, 0);

    wxBoxSizer* row4 = new wxBoxSizer(wxHORIZONTAL);
    row4->Add(m_textrich, 1, wxALL | wxEXPAND, 5);

    wxBoxSizer* leftColumn = new wxBoxSizer(wxVERTICAL);
    leftColumn->Add(row1, 0, wxALL, 10);
    leftColumn->Add(row2, 0, wxALL, 10);
    leftColumn->Add(row3, 0, wxALL, 10);

    wxBoxSizer* topSizer = new wxBoxSizer(wxHORIZONTAL);
    topSizer->Add(leftColumn, 0, wxALL | wxEXPAND, 10);
    topSizer->Add(row4, 1, wxALL | wxEXPAND, 10);

    SetSizer(topSizer);
}
void cPanel::LoadSettings()
{
    CConfig config = GetGlobalConfigCopy();

    m_cbxTrackOnStart->SetValue(config.GetBool("General/track_on_start"));
    m_cbxEnableWatchdog->SetValue(config.GetBool("General/watchdog_enabled"));
    m_cbxQuitOnLossOfTrackIR->SetValue(config.GetBool("General/quit_on_loss_of_track_ir"));

    wxString activeProfile = config.GetString("General/active_profile");
    LogToWix(fmt::format("activeProfile: {}", activeProfile));

    int index = m_cmbProfiles->FindString(activeProfile);
    if (wxNOT_FOUND == index)
    {
        wxLogError("active profile not found in available loaded profiles");
    }
    else
    {
        m_cmbProfiles->SetSelection(index);
    }

}

void cPanel::OnEnabledWatchdog(wxCommandEvent& event)
{
    CConfig* config = GetGlobalConfig();
    config->SetValue("General/watchdog_enabled", m_cbxEnableWatchdog->IsChecked());
}

void cPanel::OnTrackOnStart(wxCommandEvent& event)
{
    CConfig* config = GetGlobalConfig();
    config->SetValue("General/track_on_start", m_cbxTrackOnStart->IsChecked());
}

void cPanel::OnQuitOnLossOfTrackIr(wxCommandEvent& event)
{
    CConfig* config = GetGlobalConfig();
    config->SetValue("General/quit_on_loss_of_track_ir", m_cbxQuitOnLossOfTrackIR->IsChecked());
}

void cPanel::OnTrackIrDllPath(wxCommandEvent& event)
{
    CConfig* config = GetGlobalConfig();
    wxString wxsPath = m_txtTrackIrDllPath->GetLineText(0);
    std::string path(wxsPath.mb_str());
    config->SetValue("General/trackir_dll_dIirectory", path);
}

void cPanel::OnActiveProfile(wxCommandEvent& event)
{
    int index = m_cmbProfiles->GetSelection();
    CConfig* config = GetGlobalConfig();
    std::string activeProfile((m_cmbProfiles->GetString(index)).mb_str());
    config->LoadActiveDisplay(activeProfile);
    m_pnlDisplayConfig->LoadDisplaySettings();
}

void cPanel::OnSaveSettings(wxCommandEvent& event)
{
    CConfig* config = GetGlobalConfig();
    config->SaveSettings();
}

//////////////////////////////////////////////////////////////////////
//                      Display Settings Panel                      //
//////////////////////////////////////////////////////////////////////

cPanelConfiguration::cPanelConfiguration(wxPanel* panel) : wxPanel(panel)
{
    m_name = new wxTextCtrl(this, wxID_ANY, "Desktop", wxDefaultPosition, wxSize(150, 20), wxTE_LEFT, wxDefaultValidator, "");
    m_profileID = new wxTextCtrl(this, wxID_ANY, "2201576", wxDefaultPosition, wxDefaultSize, wxTE_LEFT, wxDefaultValidator, "");
    m_useDefaultPadding = new wxCheckBox(this, wxID_ANY, "Use Default Padding", wxDefaultPosition, wxDefaultSize, wxCHK_2STATE, wxDefaultValidator, "");

    m_tlcMappingData = new wxDataViewListCtrl(this, wxID_ANY, wxDefaultPosition, wxSize(300, 400), wxDV_HORIZ_RULES, wxDefaultValidator);

    m_tlcMappingData->AppendTextColumn("Display #");
    m_tlcMappingData->AppendTextColumn("Parameters");
    m_tlcMappingData->AppendTextColumn("Values", wxDATAVIEW_CELL_EDITABLE);

    wxStaticText* txtProfileName = new wxStaticText(this, wxID_ANY, "Profile Name:   ");
    wxStaticText* txtProfileId = new wxStaticText(this, wxID_ANY, "TrackIR Profile ID:   ");

    wxBoxSizer* row1 = new wxBoxSizer(wxHORIZONTAL);
    row1->Add(txtProfileName, 0, 0, 0);
    row1->Add(m_name, 0, 0, 0);

    wxBoxSizer* row2 = new wxBoxSizer(wxHORIZONTAL);
    row2->Add(txtProfileId, 0, 0, 0);
    row2->Add(m_profileID, 0, 0, 0);

    wxBoxSizer* topSizer = new wxBoxSizer(wxVERTICAL);
    topSizer->Add(row1, 0, wxALL, 5);
    topSizer->Add(row2, 0, wxALL, 5);
    topSizer->Add(m_useDefaultPadding, 0, wxALL, 5);
    topSizer->Add(m_tlcMappingData, 0, wxALL, 5);

    SetSizer(topSizer);
}

void cPanelConfiguration::LoadDisplaySettings()
{
    CConfig config = GetGlobalConfigCopy();
    auto& dpConfig = config.m_activeDisplayConfiguration;

    m_name->SetValue(dpConfig.m_name);
    m_profileID->SetValue(wxString::Format("%d", dpConfig.m_profile_ID));
    m_useDefaultPadding->SetValue(dpConfig.m_useDefaultPadding);

    std::vector<std::string> names = { "left", "right","top", "bottom" };
    m_tlcMappingData->DeleteAllItems();
    
    int displayNum = 0;
    for (auto& display : dpConfig.m_bounds)
    {
        for (int i = 0; i < 4; i++)
        {
            wxVector<wxVariant> row;
            row.push_back(wxVariant(wxString::Format("%d", displayNum)));
            row.push_back(wxVariant(names.at(i)));
            row.push_back(wxVariant(wxString::Format("%7.2f", display.rotationBounds[i])));
            m_tlcMappingData->AppendItem(row);

            // Optional Padding Values
            if(!m_useDefaultPadding->IsChecked())
            {
                wxVector<wxVariant> row;
                row.push_back(wxVariant(wxString::Format("%d", displayNum)));
                row.push_back(wxVariant("pad" + names.at(i)));
                row.push_back(wxVariant(wxString::Format("%d", display.paddingBounds[i])));
                row.push_back(wxVariant(wxString::Format("%s", "(default)")));
                m_tlcMappingData->AppendItem(row);
            }
        }
        displayNum++;
    }
}
//////////////////////////////////////////////////////////////////////
//                           TrackThread                            //
//////////////////////////////////////////////////////////////////////

TrackThread::TrackThread(cFrame* pHandler, HWND hWnd, CConfig config) : wxThread()
{
    m_pHandler = pHandler;
    m_hWnd = hWnd;
    m_Config = config;
}

TrackThread::~TrackThread()
{
    // Will need to provide locks in the future with critical sections
    https://docs.wxwidgets.org/3.0/classwx_thread.html
    m_pHandler->m_pTrackThread = NULL;
}

wxThread::ExitCode TrackThread::Entry()
{

    try
    {
        TR_Initialize(m_hWnd, m_Config);
        TR_TrackStart(m_Config);

    }
    catch (const std::exception& ex)
    {
        wxLogError("Exception happened when starting track thread:\n%s", ex.what());
        return NULL;
    }

    return NULL;
}