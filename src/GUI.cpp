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
    //CMyLogger* logger = new CMyLogger();
    //wxLog::SetActiveTarget(logger);
    //wxLog::SetLogLevel(wxLOG_Info);
    /*spdlog::info("Welcome to spdlog!");*/
    auto logger = spdlog::basic_logger_mt("mainlogger", "log-trackir.txt");
    //spdlog::set_level(spdlog::level::info);

    // Construct child elements
    m_frame = new cFrame();

    // Parse Settings File
    try
    {
       //wxLogError("press okay to continue");
       g_config.ParseFile("settings.toml");
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
        g_config.LoadSettings();
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

    m_frame->m_panel->LoadDisplayMappings(g_config);
    
   
    TrackThread* thread = new TrackThread(this, m_frame -> GetHandle(), g_config);

    if (thread->Create() == wxTHREAD_NO_ERROR)
    {
        thread->Run();
    }

    // Need to build out this function in order to
    // pass more than log messages back to my main application.
    Bind(wxEVT_THREAD, [this](wxThreadEvent& event)
        {
            cTextCtrl* textrich = m_frame->m_panel->m_textrich;

            if (1 == event.GetExtraLong())
            {
                // Setting textile for future output
                wxTextAttr attrExisting = textrich->GetDefaultStyle();

                textrich->SetDefaultStyle(wxTextAttr(*wxRED));

                textrich->AppendText(event.GetString());

                textrich->SetDefaultStyle(attrExisting);

            }
            else
            {
                textrich->AppendText(event.GetString());
            }


        });

	m_frame->Show();

	return true;
}

// ----------------------------------------------------------------------

cTextCtrl::cTextCtrl(wxWindow* parent, wxWindowID id, const wxString& value,
    const wxPoint& pos, const wxSize& size, int style) : wxTextCtrl(parent, id, value, pos, size, style)
{

}

// ----------------------------------------------------------------------

cPanel::cPanel(wxFrame* frame) : wxPanel(frame)
{
    m_cbxTrackOnStart = new wxCheckBox(this, myID_TRACK_ON_START, "Track On Start", wxDefaultPosition, wxDefaultSize, wxCHK_2STATE, wxDefaultValidator, "");
    m_cbxQuitOnLossOfTrackIR = new wxCheckBox(this, myID_QUIT_ON_LOSS_OF_TRACK_IR, "Quit On Loss Of Track IR", wxDefaultPosition, wxDefaultSize, wxCHK_2STATE, wxDefaultValidator, "");

    m_btnStartMouse = new wxButton(this, wxID_ANY, "Start Mouse", wxDefaultPosition, wxDefaultSize, 0, wxDefaultValidator, "");
    m_btnStopMouse = new wxButton(this, wxID_ANY, "Stop Mouse", wxDefaultPosition, wxDefaultSize, 0, wxDefaultValidator, "");
    m_btnSaveSettings = new wxButton(this, myID_SAVE_SETTINGS, "Save Settings", wxDefaultPosition, wxDefaultSize, 0, wxDefaultValidator, "");

    m_cmbProfiles = new wxComboBox(this, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, 0, 0, wxCB_DROPDOWN, wxDefaultValidator, "");

    m_tlcMappingData = new wxDataViewListCtrl(this, wxID_ANY, wxDefaultPosition, wxSize(300, 400), wxDV_HORIZ_RULES, wxDefaultValidator);

    m_tlcMappingData->AppendTextColumn("Display #");
    m_tlcMappingData->AppendTextColumn("Parameters");
    m_tlcMappingData->AppendTextColumn("Values", wxDATAVIEW_CELL_EDITABLE);
    

    wxString start_message(fmt::format("{:-^50}\n", "MouseTrackIR Application"));
    m_textrich = new cTextCtrl(this, wxID_ANY, start_message,
        wxDefaultPosition, wxDefaultSize,
        wxTE_RICH | wxTE_MULTILINE);
    
    m_textrich -> SetFont(wxFont(12, wxFONTFAMILY_TELETYPE, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));

    wxBoxSizer* row1 = new wxBoxSizer(wxVERTICAL);
    row1->Add(m_cbxTrackOnStart, 0, wxALL, 0);
    row1->Add(m_cbxQuitOnLossOfTrackIR, 0, wxALL, 0);

    wxBoxSizer* row2 = new wxBoxSizer(wxVERTICAL);
    row2->Add(m_btnStartMouse, 0, wxALL, 0);
    row2->Add(m_btnStopMouse, 0, wxALL, 0);
    row2->Add(m_btnSaveSettings, 0, wxALL, 0);
    row2->Add(m_cmbProfiles, 0, wxALL, 0);
              
    wxBoxSizer* row3 = new wxBoxSizer(wxVERTICAL);
    //row3->Add(tcMappingData, 1, wxALL | wxEXPAND, 0);
    row3->Add(m_tlcMappingData, 0, wxALL | wxEXPAND, 0);

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

void cPanel::LoadDisplayMappings(const CConfig config)
{
    std::vector<std::string> names = { "left", "right","top", "bottom" };

    for (int i = 0; i < config.m_monitorCount; i++)
    {
        for (int j = 0; j < names.size(); j++)
        {
            wxVector<wxVariant> row;
            row.push_back(wxVariant(std::to_string(i)));
            row.push_back(wxVariant(names.at(j)));

            wxString stringnumber = wxString::Format(wxT("%d"), (int)config.m_bounds[i].rotationBounds[j]);

            row.push_back(wxVariant(stringnumber));
            m_tlcMappingData->AppendItem(row);
        }
    }
}

// ----------------------------------------------------------------------
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


// ----------------------------------------------------------------------

TrackThread::TrackThread(wxEvtHandler* parent, HWND hWnd, const CConfig config) : wxThread()
{
    m_parent = parent;
    m_hWnd = hWnd;
    m_Config = config;
}

wxThread::ExitCode TrackThread::Entry()
{

    try
    {
        CTracker Tracker(m_parent, m_hWnd, m_Config);
        int result = Tracker.trackStart(m_Config);
    }
    catch (const std::exception&)
    {
        return NULL;
    }

    return NULL;
}