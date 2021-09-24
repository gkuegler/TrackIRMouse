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
        for (unsigned n = 0; n < 10; n++)
        {
            wxThreadEvent* evt = new wxThreadEvent(wxEVT_THREAD);
            strData var;
            var.int1 = n;
            var.int2 = n * n;
            var.f1 = std::sqrt(n);
            evt->SetPayload(var);
            m_parent->QueueEvent(evt);
            //May use this in the future
            //wxTheApp->QueueEvent(evt);
            this->Sleep(1000);
        }
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

