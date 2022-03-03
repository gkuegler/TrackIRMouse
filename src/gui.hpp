#ifndef TRACKIRMOUSE_GUI_H
#define TRACKIRMOUSE_GUI_H

#include <wx/dataview.h>
#include <wx/wx.h>

#include "config.hpp"
#include "exceptions.hpp"
#include "gui-control-id.hpp"
#include "gui-graphic.hpp"
#include "log.hpp"
#include "track.hpp"

// forward decl of thread classes
class TrackThread;
class WatchdogThread;
class cPanel;
class cFrame;

//////////////////////////////////////////////////////////////////////
//                  Display Configuration SubPanel                  //
//////////////////////////////////////////////////////////////////////

class cPanelConfiguration : public wxPanel {
 public:
  wxTextCtrl *m_name;
  wxComboBox *m_profileID;
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
  int m_ival = 0;
  void OnName(wxCommandEvent &event);
  void OnProfileID(wxCommandEvent &event);
  void OnUseDefaultPadding(wxCommandEvent &event);
  void OnMappingData(wxDataViewEvent &event);
  void OnAddDisplay(wxCommandEvent &event);
  void OnRemoveDisplay(wxCommandEvent &event);
  void OnMoveUp(wxCommandEvent &event);
  void OnMoveDown(wxCommandEvent &event);
};

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
  cDisplayGraphic *m_displayGraphic;

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
};

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
};

//////////////////////////////////////////////////////////////////////
//                         Main Application                         //
//////////////////////////////////////////////////////////////////////

class CGUIApp : public wxApp {
 public:
  CGUIApp(){};
  ~CGUIApp(){};

  virtual bool OnInit();
  virtual int OnExit();
  virtual void OnUnhandledException() {
    wxLogFatalError(
        "An unhandled exception has occurred. "
        "Application will now terminate.");
    std::terminate();
  }

 private:
  cFrame *m_frame = nullptr;
};

wxDECLARE_APP(CGUIApp);

#endif /* TRACKIRMOUSE_GUI_H */
