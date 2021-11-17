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

class cFrame;

class TrackThread : public wxThread
{
public:
    cFrame* m_pHandler = nullptr;
    HWND m_hWnd;
    CConfig m_Config;

    TrackThread(cFrame* m_pHandler, HWND hWnd, CConfig config);
    ~TrackThread();

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
    wxTextCtrl* m_name;
    wxTextCtrl* m_profileID;
    wxCheckBox* m_useDefaultPadding;

    wxDataViewListCtrl* m_tlcMappingData;

    cPanelConfiguration(wxPanel* panel);
    
    void LoadDisplaySettings();

private:
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
    wxCheckBox* m_cbxEnableWatchdog;
    wxCheckBox* m_cbxTrackOnStart;
    wxCheckBox* m_cbxQuitOnLossOfTrackIR;
    wxTextCtrl* m_txtTrackIrDllPath;
    wxButton* m_btnStartMouse;
    wxButton* m_btnStopMouse;
    wxButton * m_btnSaveSettings;
    wxComboBox* m_cmbProfiles;

    cPanelConfiguration* m_pnlDisplayConfig;

    cTextCtrl* m_textrich;

    cPanel(wxFrame* frame);
    
    void PopulateComboBoxWithProfiles(CConfig config)
    {
        for (auto& item : config.m_profileNames)
        {
            m_cmbProfiles->Append(item);
        }
    }

private:
    wxDataViewListCtrl* m_tlcMappingData;

    // Control Event Handlers
    void OnEnabledWatchdog(wxCommandEvent& event);
    void OnTrackOnStart(wxCommandEvent& event);
    void OnQuitOnLossOfTrackIr(wxCommandEvent& event);
    void OnTrackIrDllPath(wxCommandEvent& event);
    void OnSaveSettings(wxCommandEvent& event);
    void OnActiveProfile(wxCommandEvent& event);

    wxDECLARE_EVENT_TABLE();
};

wxBEGIN_EVENT_TABLE(cPanel, wxPanel)
    EVT_CHECKBOX(myID_WATCHDOG_ENABLED, cPanel::OnEnabledWatchdog)
    EVT_CHECKBOX(myID_TRACK_ON_START, cPanel::OnTrackOnStart)
    EVT_CHECKBOX(myID_QUIT_ON_LOSS_OF_TRACK_IR, cPanel::OnQuitOnLossOfTrackIr)
    EVT_TEXT_ENTER(myID_TRACK_IR_DLL_PATH, cPanel::OnTrackIrDllPath)
    EVT_BUTTON(myID_SAVE_SETTINGS, cPanel::OnSaveSettings)
    EVT_COMBOBOX(myID_PROFILE_SELECTION, cPanel::OnActiveProfile)
wxEND_EVENT_TABLE()

//////////////////////////////////////////////////////////////////////
//                              Frame                               //
//////////////////////////////////////////////////////////////////////

class cFrame : public wxFrame
{
public:
    cPanel* m_panel;
    TrackThread* m_pTrackThread = nullptr;

    cFrame();

    void OnTrackStart(wxCommandEvent& event);

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
    EVT_BUTTON(myID_GEN_EXMPL, cFrame::OnGenerateExample)
    EVT_BUTTON(myID_TRACK_START, cFrame::OnGenerateExample)
wxEND_EVENT_TABLE()

//////////////////////////////////////////////////////////////////////
//                         Main Application                         //
//////////////////////////////////////////////////////////////////////

class CGUIApp : public wxApp
{
public:
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