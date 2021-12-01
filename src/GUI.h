#ifndef TRACKIRMOUSE_GUI_H
#define TRACKIRMOUSE_GUI_H

#include <wx/dataview.h>
#include <wx/propdlg.h>
#include <wx/wx.h>
#include <wx/choicdlg.h>

#include "Config.h"
#include "ControlIDs.h"
#include "Exceptions.h"
#include "Log.h"
#include "Track.h"
// #include <wx/wfstream.h>

class cFrame;

class TrackThread : public wxThread {
 public:
  cFrame *m_pHandler = nullptr;
  HWND m_hWnd;

  TrackThread(cFrame *m_pHandler, HWND hWnd);
  ~TrackThread();

  ExitCode Entry();

 protected:
  wxEvtHandler *m_parent;
};

//////////////////////////////////////////////////////////////////////
//                  Display Configuration SubPanel                  //
//////////////////////////////////////////////////////////////////////

class cPanelConfiguration : public wxPanel {
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

  wxDECLARE_EVENT_TABLE();
};
// clang-format off
wxBEGIN_EVENT_TABLE(cPanelConfiguration, wxPanel)
    EVT_TEXT_ENTER(myID_PROFILE_NAME, cPanelConfiguration::OnName)
    EVT_TEXT_ENTER(myID_PROFILE_ID, cPanelConfiguration::OnProfileID)
    EVT_CHECKBOX(myID_USE_DEFAULT_PADDING, cPanelConfiguration::OnUseDefaultPadding)
wxEND_EVENT_TABLE()
    // clang-format on

    //////////////////////////////////////////////////////////////////////
    //                Text Control Status Output Window                 //
    //////////////////////////////////////////////////////////////////////

    class cTextCtrl : public wxTextCtrl {
 public:
  cTextCtrl(wxWindow *parent, wxWindowID id, const wxString &value,
            const wxPoint &pos, const wxSize &size, int style = 0);
};

//////////////////////////////////////////////////////////////////////
//                              Panel                               //
//////////////////////////////////////////////////////////////////////

class cPanel : public wxPanel {
 public:
  wxButton *m_btnStartMouse;
  wxButton *m_btnStopMouse;
  wxChoice *m_cmbProfiles;
  wxButton *m_btnAddProfile;
  wxButton *m_btnRemoveProfile;

  wxArrayString m_choices;

  cPanelConfiguration *m_pnlDisplayConfig;

  cTextCtrl *m_textrich;

  cPanel(cFrame *frame);

  void PopulateComboBoxWithProfiles(CConfig config);
  void PopulateSettings();

  void OnTrackStart(wxCommandEvent &event);
  void OnTrackStop(wxCommandEvent &event);

 private:
  cFrame *m_parent;
  wxMultiChoiceDialog* m_removeProfile;

  void OnActiveProfile(wxCommandEvent &event);
  void OnAddProfile(wxCommandEvent &event);
  void OnRemoveProfile(wxCommandEvent &event);

  wxDECLARE_EVENT_TABLE();
};

// clang-format off
wxBEGIN_EVENT_TABLE(cPanel, wxPanel)
  EVT_BUTTON(myID_START_TRACK, cPanel::OnTrackStart)
  EVT_BUTTON(myID_STOP_TRACK, cPanel::OnTrackStop)
  EVT_CHOICE(myID_PROFILE_SELECTION, cPanel::OnActiveProfile)
  EVT_BUTTON(myID_ADD_PROFILE, cPanel::OnAddProfile)
  EVT_BUTTON(myID_REMOVE_PROFILE, cPanel::OnRemoveProfile)
wxEND_EVENT_TABLE()
// clang-format on

class cSettingsGeneralPanel : public wxPanel {
 public:
  wxCheckBox *m_cbxEnableWatchdog;
  wxCheckBox *m_cbxTrackOnStart;
  wxCheckBox *m_cbxQuitOnLossOfTrackIR;
  cSettingsGeneralPanel(wxWindow* parent);
  void UpdateControls(SData *userData);

 private:
  SData *m_userData = nullptr;
  void OnEnabledWatchdog(wxCommandEvent &event);
  void OnTrackOnStart(wxCommandEvent &event);
  void OnQuitOnLossOfTrackIr(wxCommandEvent &event);
  wxDECLARE_EVENT_TABLE();
};

// clang-format off
BEGIN_EVENT_TABLE(cSettingsGeneralPanel, wxPanel)
  EVT_CHECKBOX(myID_WATCHDOG_ENABLED, cSettingsGeneralPanel::OnEnabledWatchdog)
  EVT_CHECKBOX(myID_TRACK_ON_START, cSettingsGeneralPanel::OnTrackOnStart)
  EVT_CHECKBOX(myID_QUIT_ON_LOSS_OF_TRACK_IR, cSettingsGeneralPanel::OnQuitOnLossOfTrackIr)
END_EVENT_TABLE()
// clang-format on

class cSettingsAdvancedlPanel : public wxPanel {
 public:
  wxTextCtrl* m_txtTrackIrDllPath;
  cSettingsAdvancedlPanel(wxWindow *parent);
  void UpdateControls(SData *userData);

 private:
  SData *m_userData = nullptr;
  void OnTrackIrDllPath(wxCommandEvent &event);
  wxDECLARE_EVENT_TABLE();
};

// clang-format off
BEGIN_EVENT_TABLE(cSettingsAdvancedlPanel, wxPanel)
  EVT_TEXT(myID_TRACK_IR_DLL_PATH, cSettingsAdvancedlPanel::OnTrackIrDllPath)
END_EVENT_TABLE()
// clang-format on

class cSettingsPopup : public wxPropertySheetDialog {
 public:
  SData *m_userData = nullptr;
  cSettingsPopup(cFrame *m_parent);
  void LoadUserData(SData *userData);

 private:
  cFrame *m_parent = nullptr;
  cSettingsGeneralPanel *m_pnlGen;
  cSettingsAdvancedlPanel *m_pnlAdv;
};

//class cRemoveProfile : public wxMultiChoiceDialog {
//public:
//};

//////////////////////////////////////////////////////////////////////
//                              Frame                               //
//////////////////////////////////////////////////////////////////////

class cFrame : public wxFrame {
 public:
  cPanel *m_panel;
  TrackThread *m_pTrackThread = nullptr;
  wxCriticalSection m_pThreadCS;  // protects the m_pThread pointer

  cFrame();

 private:
  cSettingsPopup *m_settingsPopup;

  void OnExit(wxCommandEvent &event);
  void OnAbout(wxCommandEvent &event);
  void OnOpen(wxCommandEvent &event);
  void OnSave(wxCommandEvent &event);
  void OnSettings(wxCommandEvent &event);
  void OnGenerateExample(wxCommandEvent &event);

  wxDECLARE_EVENT_TABLE();
};

// clang-format off
wxBEGIN_EVENT_TABLE(cFrame, wxFrame)
    EVT_MENU(wxID_EXIT, cFrame::OnExit)
    EVT_MENU(wxID_ABOUT, cFrame::OnAbout)
    EVT_MENU(wxID_OPEN, cFrame::OnOpen)
    EVT_MENU(wxID_SAVE, cFrame::OnSave)
    EVT_MENU(myID_SETTINGS, cFrame::OnSettings)
    EVT_BUTTON(myID_GEN_EXMPL, cFrame::OnGenerateExample)
wxEND_EVENT_TABLE()
    // clang-format on

    //////////////////////////////////////////////////////////////////////
    //                         Main Application                         //
    //////////////////////////////////////////////////////////////////////

    class CGUIApp : public wxApp {
 public:
  CGUIApp(){};
  ~CGUIApp(){};

  virtual bool OnInit();
  virtual void OnUnhandledException() {
    wxLogFatalError("An unhandled exception has occurred.");
    std::terminate();
  }

 private:
  cFrame *m_frame = nullptr;
};

wxDECLARE_APP(CGUIApp);

#endif /* TRACKIRMOUSE_GUI_H */
