#pragma once

#include <wx/wx.h>
#include "Track.h"

class MyThread : public wxThread
{
public:
    HWND m_hWnd;

    MyThread(wxEvtHandler* parent, HWND hWnd);
    ExitCode Entry();

protected:
    wxEvtHandler* m_parent;
};

class cTextCtrl : public wxTextCtrl
{
public:
    cTextCtrl(wxWindow* parent, wxWindowID id, const wxString& value,
        const wxPoint& pos, const wxSize& size, int style = 0);
};

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