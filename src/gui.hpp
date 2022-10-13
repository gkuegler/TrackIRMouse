#ifndef TRACKIRMOUSE_GUI_H
#define TRACKIRMOUSE_GUI_H

#include <wx/dataview.h>
#include <wx/wx.h>

#include "config.hpp"
#include "gui-control-id.hpp"
#include "gui-graphic.hpp"
#include "log.hpp"
#include "track.hpp"

// forward decl of thread classes
class TrackThread;
class ControlServerThread;

//////////////////////////////////////////////////////////////////////
//                Text Control Status Output Window                 //
//////////////////////////////////////////////////////////////////////

class cTextCtrl : public wxTextCtrl {
 public:
  cTextCtrl(wxWindow *parent, wxWindowID id, const wxString &value,
            const wxPoint &pos, const wxSize &size, int style = 0)
      : wxTextCtrl(parent, id, value, pos, size, style){};
};

//////////////////////////////////////////////////////////////////////
//                              Frame                               //
//////////////////////////////////////////////////////////////////////

// Main frame of program
class cFrame : public wxFrame {
 public:
  TrackThread *m_pTrackThread = nullptr;
  ControlServerThread *m_pServerThread = nullptr;
  wxCriticalSection m_pThreadCS;  // protects all thread pointers

  // log window
  wxStaticText *m_lbtextrich;
  cTextCtrl *m_textrich;

  cDisplayGraphic *m_displayGraphic;

  wxChoice *m_cmbProfiles;

  std::unique_ptr<config::game_title_map_t> m_titlesMap;
  wxTextCtrl *m_name;
  wxTextCtrl *m_profileID;
  wxTextCtrl *m_profileGameTitle;
  wxCheckBox *m_useDefaultPadding;
  wxDataViewListCtrl *m_tlcMappingData;

  cFrame(wxPoint, wxSize);
  void InitializeSettings();
  void UpdateGuiUsingSettings();

 public:
  void OnExit(wxCommandEvent &event);
  void OnAbout(wxCommandEvent &event);
  void OnSave(wxCommandEvent &event);
  void OnReload(wxCommandEvent &event);
  void OnSettings(wxCommandEvent &event);
  // controls handlers
  void PopulateComboBoxWithProfiles();
  void PopulateSettings();

  void OnStart(wxCommandEvent &event);
  void OnStop(wxCommandEvent &event);
  void OnShowLog(wxCommandEvent &event);
  void OnActiveProfile(wxCommandEvent &event);
  void OnAddProfile(wxCommandEvent &event);
  void OnRemoveProfile(wxCommandEvent &event);
  void OnDuplicateProfile(wxCommandEvent &event);

  // display config controls
  void LoadDisplaySettings();
  int m_ival = 0;
  void OnName(wxCommandEvent &event);
  void OnProfileID(wxCommandEvent &event);
  void OnPickTitle(wxCommandEvent &event);
  void OnUseDefaultPadding(wxCommandEvent &event);
  void OnMappingData(wxDataViewEvent &event);
  void OnAddDisplay(wxCommandEvent &event);
  void OnRemoveDisplay(wxCommandEvent &event);
  void OnMoveUp(wxCommandEvent &event);
  void OnMoveDown(wxCommandEvent &event);
};

//////////////////////////////////////////////////////////////////////
//                         Main Application                         //
//////////////////////////////////////////////////////////////////////

class cApp : public wxApp {
 public:
  cApp(){};
  ~cApp(){};

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

wxDECLARE_APP(cApp);

#endif /* TRACKIRMOUSE_GUI_H */
