#define _CRT_SECURE_NO_WARNINGS

#include "GUI.h"

#include "Track.h"
#include "Config.h"
#include "Log.h"
#include "Exceptions.h"

#define FMT_HEADER_ONLY
#include <fmt/format.h>

#include <wx/dataview.h>
#include <wx/colour.h>

wxIMPLEMENT_APP(CGUIApp);


bool CGUIApp::OnInit()
{
    // Initialize global logger
    //CMyLogger* logger = new CMyLogger();
    //wxLog::SetActiveTarget(logger);
    //wxLog::SetLogLevel(wxLOG_Info);

    // Construct child elements
    m_frame = new cFrame();

    // Load Settings
    m_config = CConfig();

    try
    {
        m_config.LoadSettings();
        m_config.SetGeneralInteger("profile", 5);
        m_config.SaveSettings();
    }
    catch (std::runtime_error& e)
    {
        LogToWixError((fmt::format("runtime_error: Load Settings Failed. See TOML error above.")));
        return true;
    }
    // TODO: something in the toml library
    // is causing this error to be raised:
    // "invalid unordered_map<K, T> key"
    catch (const std::exception& ex)
    {
        LogToWixError(fmt::format("std::exception&: Load Setting Failed: ", ex.what()));
        //wxLogError("Load Setting Failed: %s", ex.what());
    }
    catch (const Exception& ex)
    {
        LogToWixError(fmt::format("Exception: Load Setting Failed: ", ex.what()));
        wxLogError("Load Setting Failed: %s", ex.what());
    }
    catch (...)
    {
        LogToWixError("An unconquered exception has gone on handle when loading settings.");
        wxLogFatalError("exception has gone unhandled");
    }

    m_frame -> m_panel -> LoadDisplayMappings(m_config);

    TrackThread* thread = new TrackThread(this, m_frame -> GetHandle(), &m_config);

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
    wxCheckBox* cbxTrackOnStart = new wxCheckBox(this, wxID_ANY, "Track On Start", wxDefaultPosition, wxDefaultSize, wxCHK_2STATE, wxDefaultValidator, "");
    wxCheckBox* cbxQuitOnLossOfTrackIR = new wxCheckBox(this, wxID_ANY, "Quit On Loss Of Track IR", wxDefaultPosition, wxDefaultSize, wxCHK_2STATE, wxDefaultValidator, "");

    wxButton* btnStartMouse = new wxButton(this, wxID_ANY, "Start Mouse", wxDefaultPosition, wxDefaultSize, 0, wxDefaultValidator, "");
    wxButton* btnStopMouse = new wxButton(this, wxID_ANY, "Stop Mouse", wxDefaultPosition, wxDefaultSize, 0, wxDefaultValidator, "");

    wxComboBox* cmbProfiles = new wxComboBox(this, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, 0, 0, wxCB_DROPDOWN, wxDefaultValidator, "");

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
    row1->Add(cbxTrackOnStart, 0, wxALL, 0);
    row1->Add(cbxQuitOnLossOfTrackIR, 0, wxALL, 0);

    wxBoxSizer* row2 = new wxBoxSizer(wxVERTICAL);
    row2->Add(btnStartMouse, 0, wxALL, 0);
    row2->Add(btnStopMouse, 0, wxALL, 0);
    row2->Add(cmbProfiles, 0, wxALL, 0);

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

void cPanel::LoadDisplayMappings(const CConfig& config)
{
    std::vector<std::string> names = { "left", "right","top", "bottom" };

    for (int i = 0; i < config.m_monitorCount; i++)
    {
        for (int j = 0; j < names.size(); j++)
        {
            wxVector<wxVariant> rotationBoundLeft;
            rotationBoundLeft.push_back(wxVariant(std::to_string(i)));
            rotationBoundLeft.push_back(wxVariant(names.at(j)));

            wxString stringnumber = wxString::Format(wxT("%d"), (int)config.bounds[i].left);

            rotationBoundLeft.push_back(wxVariant(stringnumber));
            m_tlcMappingData->AppendItem(rotationBoundLeft);
        }
    }
}

// ----------------------------------------------------------------------

cFrame::cFrame() : wxFrame(nullptr, wxID_ANY, "Example Title", wxPoint(200, 200), wxSize(1200, 800))
{
    wxMenu* menuFile = new wxMenu;

    //menuFile->Append(wxID_ANY, "&Hello...\tCtrl-H",
        //"Help string shown in status bar for this menu item");

    //menuFile->AppendSeparator();
    menuFile->Append(wxID_EXIT);

    wxMenu* menuHelp = new wxMenu;
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


// ----------------------------------------------------------------------

TrackThread::TrackThread(wxEvtHandler* parent, HWND hWnd, CConfig* pConfig) : wxThread()
{
    m_parent = parent;
    m_hWnd = hWnd;
    m_pConfig = pConfig;
}

wxThread::ExitCode TrackThread::Entry()
{

    try
    {
        CTracker Tracker(m_parent, m_hWnd, m_pConfig);
        int result = Tracker.trackStart(m_pConfig);
    }
    catch (const std::exception&)
    {
        return NULL;
    }

    return NULL;
}