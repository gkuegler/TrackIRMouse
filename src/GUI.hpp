#ifndef TRACKIRMOUSE_GUI_H
#define TRACKIRMOUSE_GUI_H

#include <wx/dataview.h>
#include <wx/wx.h>

#include "config.hpp"
#include "exceptions.hpp"
#include "gui-control-id.hpp"
#include "log.hpp"
#include "track.hpp"

class cFrame;
class cPanel;

class TrackThread : public wxThread {
 public:
  cFrame *m_pHandler = nullptr;
  HWND m_hWnd;

  TrackThread(cFrame *m_pHandler, HWND hWnd);
  ~TrackThread();

  ExitCode Entry();
};

class WatchdogThread : public wxThread {
 public:
  cFrame *m_pHandler = nullptr;
  HANDLE m_hPipe = INVALID_HANDLE_VALUE;

  WatchdogThread(cFrame *pHandler);
  ~WatchdogThread();

  ExitCode Entry();
};

//////////////////////////////////////////////////////////////////////
//                  Display Configuration SubPanel                  //
//////////////////////////////////////////////////////////////////////

class cPanelConfiguration : public wxPanel {
 public:
  wxTextCtrl *m_name;
  wxTextCtrl *m_profileID;
  wxCheckBox *m_useDefaultPadding;
  wxButton *m_btnAddDisplay;
  wxButton *m_btnRemoveDisplay;
  wxButton *m_btnMoveUp;
  wxButton *m_btnMoveDown;
  wxDataViewListCtrl *m_tlcMappingData;

  cPanelConfiguration(cPanel *parent);

  void LoadDisplaySettings();

 private:
  cPanel *m_parent;
  void OnName(wxCommandEvent &event);
  void OnProfileID(wxCommandEvent &event);
  void OnUseDefaultPadding(wxCommandEvent &event);
  void OnMappingData(wxDataViewEvent &event);
  void OnAddDisplay(wxCommandEvent &event);
  void OnRemoveDisplay(wxCommandEvent &event);
  void OnMoveUp(wxCommandEvent &event);
  void OnMoveDown(wxCommandEvent &event);

  wxDECLARE_EVENT_TABLE();
};

// clang-format off
wxBEGIN_EVENT_TABLE(cPanelConfiguration, wxPanel)
    EVT_TEXT(myID_PROFILE_NAME, cPanelConfiguration::OnName)
    EVT_TEXT(myID_PROFILE_ID, cPanelConfiguration::OnProfileID)
    EVT_CHECKBOX(myID_USE_DEFAULT_PADDING, cPanelConfiguration::OnUseDefaultPadding)
    // EVT_BUTTON(myID_MANAGE_DISPLAYS, cPanelConfiguration::OnManageDisplays)
    EVT_DATAVIEW_ITEM_EDITING_DONE(myID_MAPPING_DATA, cPanelConfiguration::OnMappingData)
    EVT_BUTTON(myID_ADD_DISPLAY, cPanelConfiguration::OnAddDisplay)
    EVT_BUTTON(myID_REMOVE_DISPLAY, cPanelConfiguration::OnRemoveDisplay)
    EVT_BUTTON(myID_MOVE_UP, cPanelConfiguration::OnMoveUp)
    EVT_BUTTON(myID_MOVE_DOWN, cPanelConfiguration::OnMoveDown)
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
  wxButton *m_btnDuplicateProfile;
  cTextCtrl *m_textrich;
  cPanelConfiguration *m_pnlDisplayConfig;

  cPanel(cFrame *parent);

  void PopulateComboBoxWithProfiles();
  void PopulateSettings();

  void OnTrackStart(wxCommandEvent &event);
  void OnTrackStop(wxCommandEvent &event);

 private:
  cFrame *m_parent;

  void OnActiveProfile(wxCommandEvent &event);
  void OnAddProfile(wxCommandEvent &event);
  void OnRemoveProfile(wxCommandEvent &event);
  void OnDuplicateProfile(wxCommandEvent &event);

  wxDECLARE_EVENT_TABLE();
};

//clang-format off
wxBEGIN_EVENT_TABLE(cPanel, wxPanel)
    EVT_BUTTON(myID_START_TRACK, cPanel::OnTrackStart)
        EVT_BUTTON(myID_STOP_TRACK, cPanel::OnTrackStop)
            EVT_CHOICE(myID_PROFILE_SELECTION, cPanel::OnActiveProfile)
                EVT_BUTTON(myID_ADD_PROFILE, cPanel::OnAddProfile)
                    EVT_BUTTON(myID_REMOVE_PROFILE, cPanel::OnRemoveProfile)
                        EVT_BUTTON(myID_DUPLICATE_PROFILE,
                                   cPanel::OnDuplicateProfile)
                            wxEND_EVENT_TABLE()
    // clang-format on

    //////////////////////////////////////////////////////////////////////
    //                              Frame                               //
    //////////////////////////////////////////////////////////////////////

    // Main frame of program
    class cFrame : public wxFrame {
 public:
  cPanel *m_panel;
  TrackThread *m_pTrackThread = nullptr;
  WatchdogThread *m_pWatchdogThread = nullptr;
  wxCriticalSection m_pThreadCS;  // protects all thread pointers

  cFrame(wxPoint, wxSize);
  void LoadSettingsFromFile();
  void UpdateGuiFromSettings();

 private:
  void OnExit(wxCommandEvent &event);
  void OnAbout(wxCommandEvent &event);
  void OnOpen(wxCommandEvent &event);
  void OnSave(wxCommandEvent &event);
  void OnReload(wxCommandEvent &event);
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
  EVT_MENU(wxID_RELOAD, cFrame::OnReload)
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
