#pragma once
#ifndef TRACKIRMOUSE_GUI_H
#define TRACKIRMOUSE_GUI_H

#include "Config.h"
#include "Track.h"

#include "Log.h"
#include "ControlIDs.h"

#include <wx/wx.h>
#include <wx/dataview.h>



class TrackThread : public wxThread
{
public:
    HWND m_hWnd;
    CConfig* m_pConfig;

    TrackThread(wxEvtHandler* parent, HWND hWnd, CConfig* config);
    ExitCode Entry();

protected:
    wxEvtHandler* m_parent;
};

//------------------------------------------------------------------------------

class cTextCtrl : public wxTextCtrl
{
public:
    cTextCtrl(wxWindow* parent, wxWindowID id, const wxString& value,
        const wxPoint& pos, const wxSize& size, int style = 0);
};

//------------------------------------------------------------------------------

class cPanel : public wxPanel
{
public:
    cTextCtrl* m_textrich;

    cPanel(wxFrame* frame);

    void LoadDisplayMappings(const CConfig& config);

private:
    wxDataViewListCtrl* m_tlcMappingData;

    void SaveCheckbox(wxCommandEvent& event)
    {
        LogToWix("checkbox pressed: ");
        switch (event.GetId())
        {
        case myID_TRACK_ON_START:
            //SetValueInTable?
            //where to declare function?
            LogToWix("myID_TRACK_ON_START pressed\n");
            break;

        case myID_QUIT_ON_LOSS_OF_TRACK_IR:
            LogToWix("myID_QUIT_ON_LOSS_OF_TRACK_IR pressed\n");
            break;
        }
    }

    wxDECLARE_EVENT_TABLE();
};

wxBEGIN_EVENT_TABLE(cPanel, wxPanel)
    EVT_CHECKBOX(myID_TRACK_ON_START, cPanel::SaveCheckbox)
    EVT_CHECKBOX(myID_QUIT_ON_LOSS_OF_TRACK_IR, cPanel::SaveCheckbox)
wxEND_EVENT_TABLE()

//------------------------------------------------------------------------------

class cFrame : public wxFrame
{
public:
    cPanel* m_panel;

    cFrame();

private:
    void OnExit(wxCommandEvent& event);
    void OnAbout(wxCommandEvent& event);

    wxDECLARE_EVENT_TABLE();

};

wxBEGIN_EVENT_TABLE(cFrame, wxFrame)
    EVT_MENU(wxID_EXIT, cFrame::OnExit)
    EVT_MENU(wxID_ABOUT, cFrame::OnAbout)
wxEND_EVENT_TABLE()

//------------------------------------------------------------------------------

class CGUIApp : public wxApp
{
public:
    CConfig m_config;

    CGUIApp() {};
    ~CGUIApp() {};

	virtual bool OnInit();
private:
	cFrame* m_frame = nullptr;
};

wxDECLARE_APP(CGUIApp);

#endif /* TRACKIRMOUSE_GUI_H */