#ifndef TRACKIRMOUSE_GUI_H
#define TRACKIRMOUSE_GUI_H

#include "Config.h"
#include "ControlIDs.h"
#include "Exceptions.h"
#include "Log.h"
#include "Track.h"

#include <wx/dataview.h>
#include <wx/wx.h>
// #include <wx/wfstream.h>

class cFrame;

class TrackThread : public wxThread
{
  public:
    cFrame *m_pHandler = nullptr;
    HWND m_hWnd;
    CConfig m_Config;

    TrackThread(cFrame *m_pHandler, HWND hWnd, CConfig config);
    ~TrackThread();

    ExitCode Entry();

  protected:
    wxEvtHandler *m_parent;
};

//////////////////////////////////////////////////////////////////////
//                  Display Configuration SubPanel                  //
//////////////////////////////////////////////////////////////////////

class cPanelConfiguration : public wxPanel
{
  public:
    wxTextCtrl *m_name;
    wxTextCtrl *m_profileID;
    wxCheckBox *m_useDefaultPadding;

    wxDataViewListCtrl *m_tlcMappingData;

    cPanelConfiguration(wxPanel *panel);

    void LoadDisplaySettings();

  private:
    // Controls Handlers
    void OnName(wxCommandEvent &event);
    void OnProfileID(wxCommandEvent &event);
    void OnUseDefaultPadding(wxCommandEvent &event);
    void OnTlcMappingData(wxCommandEvent &event);
};

//////////////////////////////////////////////////////////////////////
//                Text Control Status Output Window                 //
//////////////////////////////////////////////////////////////////////

class cTextCtrl : public wxTextCtrl
{
  public:
    cTextCtrl(wxWindow *parent, wxWindowID id, const wxString &value, const wxPoint &pos, const wxSize &size,
              int style = 0);
};

//////////////////////////////////////////////////////////////////////
//                              Panel                               //
//////////////////////////////////////////////////////////////////////

class cPanel : public wxPanel
{
  public:
    wxCheckBox *m_cbxEnableWatchdog;
    wxCheckBox *m_cbxTrackOnStart;
    wxCheckBox *m_cbxQuitOnLossOfTrackIR;
    wxTextCtrl *m_txtTrackIrDllPath;
    wxButton *m_btnStartMouse;
    wxButton *m_btnStopMouse;
    wxButton *m_btnSaveSettings;
    wxComboBox *m_cmbProfiles;
    wxButton *m_btnAddProfile;
    wxButton *m_btnRemoveProfile;

    cPanelConfiguration *m_pnlDisplayConfig;

    cTextCtrl *m_textrich;

    cPanel(wxFrame *frame);

    void LoadSettings();
    void PopulateComboBoxWithProfiles(CConfig config)
    {
        m_cmbProfiles->Clear();

        for (auto &item : config.GetProfileNames())
        {
            m_cmbProfiles->Append(item);
        }
    }

  private:
    // Control Event Handlers
    void OnEnabledWatchdog(wxCommandEvent &event);
    void OnTrackOnStart(wxCommandEvent &event);
    void OnQuitOnLossOfTrackIr(wxCommandEvent &event);
    void OnTrackIrDllPath(wxCommandEvent &event);
    void OnSaveSettings(wxCommandEvent &event);
    void OnActiveProfile(wxCommandEvent &event);
    void OnAddProfile(wxCommandEvent &event);
    void OnRemoveProfile(wxCommandEvent &event);

    wxDECLARE_EVENT_TABLE();
};

wxBEGIN_EVENT_TABLE(cPanel, wxPanel) EVT_CHECKBOX(myID_WATCHDOG_ENABLED, cPanel::OnEnabledWatchdog)
    EVT_CHECKBOX(myID_TRACK_ON_START, cPanel::OnTrackOnStart)
        EVT_CHECKBOX(myID_QUIT_ON_LOSS_OF_TRACK_IR, cPanel::OnQuitOnLossOfTrackIr)
            EVT_TEXT_ENTER(myID_TRACK_IR_DLL_PATH, cPanel::OnTrackIrDllPath)
                EVT_BUTTON(myID_SAVE_SETTINGS, cPanel::OnSaveSettings)
                    EVT_COMBOBOX(myID_PROFILE_SELECTION, cPanel::OnActiveProfile)
                        EVT_BUTTON(myID_ADD_PROFILE, cPanel::OnAddProfile)
                            EVT_BUTTON(myID_REMOVE_PROFILE, cPanel::OnRemoveProfile) wxEND_EVENT_TABLE()

    //////////////////////////////////////////////////////////////////////
    //                              Frame                               //
    //////////////////////////////////////////////////////////////////////

    class cFrame : public wxFrame
{
  public:
    cPanel *m_panel;
    TrackThread *m_pTrackThread = nullptr;

    cFrame();

    void OnTrackStart(wxCommandEvent &event);
    void OnTrackStop(wxCommandEvent &event);

  private:
    void OnExit(wxCommandEvent &event);
    void OnAbout(wxCommandEvent &event);
    void OnOpen(wxCommandEvent &event);
    void OnGenerateExample(wxCommandEvent &event);

    wxDECLARE_EVENT_TABLE();
};

wxBEGIN_EVENT_TABLE(cFrame, wxFrame) EVT_MENU(wxID_EXIT, cFrame::OnExit) EVT_MENU(wxID_ABOUT, cFrame::OnAbout)
    EVT_MENU(wxID_OPEN, cFrame::OnOpen) EVT_BUTTON(myID_GEN_EXMPL, cFrame::OnGenerateExample)
        EVT_BUTTON(myID_START_TRACK, cFrame::OnTrackStart) EVT_BUTTON(myID_STOP_TRACK, cFrame::OnTrackStop)
            wxEND_EVENT_TABLE()

    //////////////////////////////////////////////////////////////////////
    //                         Main Application                         //
    //////////////////////////////////////////////////////////////////////

    class CGUIApp : public wxApp
{
  public:
    CGUIApp(){};
    ~CGUIApp(){};

    virtual bool OnInit();
    virtual void OnUnhandledException()
    {
        wxLogFatalError("An unhandled exception has occurred.");
        std::terminate();
    }

  private:
    cFrame *m_frame = nullptr;
};

wxDECLARE_APP(CGUIApp);

#endif /* TRACKIRMOUSE_GUI_H */
