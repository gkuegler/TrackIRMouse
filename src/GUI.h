#pragma once
#ifndef TRACKIRMOUSE_GUI_H
#define TRACKIRMOUSE_GUI_H

#include <wx/wx.h>
// #include "Track.h"

class TrackThread : public wxThread
{
public:
    HWND m_hWnd;

    TrackThread(wxEvtHandler* parent, HWND hWnd);
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
    cPanel(wxFrame* frame);

    cTextCtrl* m_textrich;
};

class cFrame : public wxFrame
{
public:
    cFrame();

    cPanel* m_panel;

};

//------------------------------------------------------------------------------

class CGUIApp : public wxApp
{
public:
    CGUIApp();
	~CGUIApp();

	virtual bool OnInit();
private:
	cFrame* m_frame = nullptr;
};

wxDECLARE_APP(CGUIApp);

#endif /* TRACKIRMOUSE_GUI_H */