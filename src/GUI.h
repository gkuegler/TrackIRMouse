#pragma once

#include <wx/wx.h>
struct strData
{
    int int1;
    int int2;
    float f1;

    wxString asString() const
    {
        return wxString::Format("{%d %d %lf}\n", int1, int2, f1);
    }
};

class MyThread : public wxThread
{
public:
    MyThread(wxEvtHandler* parent) : wxThread(), m_parent(parent) {}

    ExitCode Entry()
    {
        trackStart(m_parent);

        return NULL;
    }
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

