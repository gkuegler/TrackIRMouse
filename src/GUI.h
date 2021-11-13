#ifndef TRACKIRMOUSE_GUI_H
#define TRACKIRMOUSE_GUI_H

#include "Config.h"
#include "Track.h"
#include "Log.h"
#include "ControlIDs.h"
#include "Exceptions.h"

#include <wx/wx.h>
#include <wx/dataview.h>
// #include <wx/wfstream.h>

class TrackThread : public wxThread
{
public:
    HWND m_hWnd;
    CConfig m_Config;

    TrackThread(wxEvtHandler* parent, HWND hWnd, CConfig config);
    ExitCode Entry();

protected:
    wxEvtHandler* m_parent;
};

//////////////////////////////////////////////////////////////////////
//                  Display Configuration SubPanel                  //
//////////////////////////////////////////////////////////////////////

class cPanelConfiguration : public wxPanel
{
public:
    cPanelConfiguration(wxPanel* panel);
    //LoadDisplaySettings(CConfig config);

    wxTextCtrl* m_name;
    wxTextCtrl* m_profileID;
    wxCheckBox* m_useDefaultPadding;

    wxDataViewListCtrl* m_tlcMappingData;


};

//////////////////////////////////////////////////////////////////////
//                Text Control Status Output Window                 //
//////////////////////////////////////////////////////////////////////

class cTextCtrl : public wxTextCtrl
{
public:
    cTextCtrl(wxWindow* parent, wxWindowID id, const wxString& value,
        const wxPoint& pos, const wxSize& size, int style = 0);
};

//////////////////////////////////////////////////////////////////////
//                              Panel                               //
//////////////////////////////////////////////////////////////////////

class cPanel : public wxPanel
{
public:
    wxCheckBox* m_cbxTrackOnStart;
    wxCheckBox* m_cbxQuitOnLossOfTrackIR;
    wxButton* m_btnStartMouse;
    wxButton* m_btnStopMouse;
    wxButton * m_btnSaveSettings;
    wxComboBox* m_cmbProfiles;

    wxPanel* m_pnlDisplayConfiguration;

    cTextCtrl* m_textrich;

    CConfig* m_pconfig;

    cPanel(wxFrame* frame);
    
    void LoadDisplayMappings(const CConfig config);
    void PopulateComboBoxWithProfiles(CConfig config)
    {
        for (auto& item : config.m_profileNumbers)
        {
            m_cmbProfiles->Append(item);
        }
    }

    void SetConfiguration(CConfig* config)
    {
        m_pconfig = config;
    }

private:
    wxDataViewListCtrl* m_tlcMappingData;

    void OnTrackOnStart(wxCommandEvent& event)
    {
        m_pconfig->SetValue("General/track_on_start", m_cbxTrackOnStart->IsChecked());
    }
    
    void OnQuitOnLossOfTrackIr(wxCommandEvent& event)
    {
        m_pconfig->SetValue("General/quit_on_loss_of_track_ir", m_cbxQuitOnLossOfTrackIR->IsChecked());
    }

    void OnSaveSettings(wxCommandEvent& event)
    {
        m_pconfig->SaveSettings();
    }

    wxDECLARE_EVENT_TABLE();
};

wxBEGIN_EVENT_TABLE(cPanel, wxPanel)
    EVT_CHECKBOX(myID_TRACK_ON_START, cPanel::OnTrackOnStart)
    EVT_CHECKBOX(myID_QUIT_ON_LOSS_OF_TRACK_IR, cPanel::OnQuitOnLossOfTrackIr)
    EVT_BUTTON(myID_SAVE_SETTINGS, cPanel::OnSaveSettings)
wxEND_EVENT_TABLE()

//////////////////////////////////////////////////////////////////////
//                              Frame                               //
//////////////////////////////////////////////////////////////////////

class cFrame : public wxFrame
{
public:
    cPanel* m_panel;

    cFrame();

private:
    void OnExit(wxCommandEvent& event);
    void OnAbout(wxCommandEvent& event);
    void OnOpen(wxCommandEvent& event);
    void OnGenerateExample(wxCommandEvent& event);

    wxDECLARE_EVENT_TABLE();

};

wxBEGIN_EVENT_TABLE(cFrame, wxFrame)
    EVT_MENU(wxID_EXIT, cFrame::OnExit)
    EVT_MENU(wxID_ABOUT, cFrame::OnAbout)
    EVT_MENU(wxID_OPEN, cFrame::OnOpen)
    EVT_MENU(myID_GEN_EXMPL, cFrame::OnGenerateExample)
wxEND_EVENT_TABLE()

//////////////////////////////////////////////////////////////////////
//                         Main Application                         //
//////////////////////////////////////////////////////////////////////

class CGUIApp : public wxApp
{
public:
    CConfig g_config;

    CGUIApp() {};
    ~CGUIApp() {};

	virtual bool OnInit();
    virtual void OnUnhandledException()
    {
        wxLogFatalError("An unhandled exception has occurred.");
        std::terminate();
    }
private:
	cFrame* m_frame = nullptr;
};

wxDECLARE_APP(CGUIApp);

#endif /* TRACKIRMOUSE_GUI_H */