#define _CRT_SECURE_NO_WARNINGS

#include "GUI.h"

#include "Track.h"
#include "Config.h"
#include "Log.h"

#define FMT_HEADER_ONLY
#include <fmt/format.h>

wxIMPLEMENT_APP(CGUIApp);


bool CGUIApp::OnInit()
{
    // Initialize global logger
    CMyLogger* logger = new CMyLogger();
    wxLog::SetActiveTarget(logger);
    wxLog::SetLogLevel(wxLOG_Info);

    // Construct child elements
    m_frame = new cFrame();

    // Load Settings
    m_config = CConfig();

    try
    {
        m_config.LoadSettings();
    }
    catch (std::runtime_error e)
    {
        m_frame->m_panel->m_textrich->AppendText((fmt::format("Load Settings Failed. See TOML error above.")));
    }
    catch (const std::exception& ex)
    {
        printf("%s", ex.what());
    }
    catch (...)
    {
        wxLogError("exception has been uncocked");
    }

    TrackThread* thread = new TrackThread(this, m_frame -> GetHandle(), &m_config);

    if (thread->Create() == wxTHREAD_NO_ERROR)
    {
        thread->Run();
    }

    // Need to build out this function in order to
    // pass more than log messages back to my main application.
    Bind(wxEVT_THREAD, [this](wxThreadEvent& ev)
        {
            m_frame -> m_panel->m_textrich->AppendText(ev.GetString());
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
    wxCheckBox* cbxTrackOnStart = new wxCheckBox(this, wxID_ANY, wxString("Track On Start"), wxDefaultPosition, wxDefaultSize, wxCHK_2STATE, wxDefaultValidator, wxString("Default Checkbox Name Park"));

    wxString start_message(fmt::format("{:-^50}\n", "MouseTrackIR Application"));
    m_textrich = new cTextCtrl(this, wxID_ANY, start_message,
        wxDefaultPosition, wxDefaultSize,
        wxTE_RICH | wxTE_MULTILINE);
    
    m_textrich -> SetFont(wxFont(12, wxFONTFAMILY_TELETYPE, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));
   
    // Setting text style of specific characters
    //m_textrich->SetStyle(0, 10, *wxRED);
    //m_textrich->SetStyle(30, 40,
        //wxTextAttr(*wxGREEN, wxNullColour, *wxITALIC_FONT));

    // Setting textile for future output
    //m_textrich->SetDefaultStyle(wxTextAttr()); // default text?
    //m_textrich->SetDefaultStyle(wxTextAttr(*wxCYAN, *wxBLUE)); // cyan on blue
    //m_textrich->AppendText("blah blah blah blah\n");


    // Setting Advanced Text Attributes
    //wxTextAttr attr = m_textrich->GetDefaultStyle();
    //attr.SetFontUnderlined(true);
    //m_textrich->SetDefaultStyle6(attr);
    //m_textrich->AppendText("\nAnd there");

    // lay out the controls
    //wxBoxSizer* column1 = new wxBoxSizer(wxVERTICAL);
    //column1->Add(m_textrich, 1, wxALL | wxEXPAND, 5);
    //column1->Add(m_password, 0, wxALL | wxEXPAND, 10);
    //column1->Add(m_readonly, 0, wxALL, 10);
    //column1->Add(m_limited, 0, wxALL, 10);
    //column1->Add(upperOnly, 0, wxALL, 10);
    //column1->Add(m_horizontal, 1, wxALL | wxEXPAND, 10);

    //wxBoxSizer* column2 = new wxBoxSizer(wxVERTICAL);
    //column2->Add(m_multitext, 1, wxALL | wxEXPAND, 10);
    //column2->Add(m_tab, 0, wxALL | wxEXPAND, 10);
    //column2->Add(m_enter, 1, wxALL | wxEXPAND, 10);

    wxBoxSizer* row1 = new wxBoxSizer(wxHORIZONTAL);
    row1->Add(cbxTrackOnStart, 1, wxALL | wxEXPAND, 5);

    wxBoxSizer* row2 = new wxBoxSizer(wxHORIZONTAL);
    row2->Add(m_textrich, 1, wxALL | wxEXPAND, 5);
    //row1->Add(column2, 1, wxALL | wxEXPAND, 10);
    //row1->Add(m_textrich, 1, wxALL | wxEXPAND, 10);

    wxBoxSizer* topSizer = new wxBoxSizer(wxVERTICAL);
    topSizer->Add(row1, 0, wxALL | wxEXPAND, 10);
    topSizer->Add(row2, 1, wxALL | wxEXPAND, 10);

    SetSizer(topSizer);
}

// ----------------------------------------------------------------------

cFrame::cFrame() : wxFrame(nullptr, wxID_ANY, "Example Title", wxPoint(200, 200), wxSize(700, 800))
{
    m_panel = new cPanel(this);
    //m_panel = new MyPanel(this, 10, 10, 300, 100);
    m_panel->GetSizer()->Fit(this);
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