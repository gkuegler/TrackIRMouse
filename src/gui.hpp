#ifndef TRACKIRMOUSE_GUI_H
#define TRACKIRMOUSE_GUI_H

#include <wx/dataview.h>
#include <wx/wx.h>

#include "config.hpp"
#include "gui-control-id.hpp"
#include "gui-graphic.hpp"
#include "hotkey.hpp"
#include "log.hpp"
#include "trackers.hpp"

// forward decl of thread classes
class TrackThread;
class ControlServerThread;

//////////////////////////////////////////////////////////////////////
//                Text Control Status Output Window                 //
//////////////////////////////////////////////////////////////////////

class LogWindow : public wxTextCtrl
{
public:
  LogWindow(wxWindow* parent,
            wxWindowID p_profile_id_,
            const wxString& value,
            const wxPoint& pos,
            const wxSize& size,
            int style = 0)
    : wxTextCtrl(parent, p_profile_id_, value, pos, size, style){};
};

//////////////////////////////////////////////////////////////////////
//                              Frame                               //
//////////////////////////////////////////////////////////////////////

// Main frame of program
class Frame : public wxFrame
{
public:
  TrackThread* p_track_thread_ = nullptr;
  ControlServerThread* p_server_thread_ = nullptr;
  wxCriticalSection p_thread_cs; // protects all thread pointers

  // log window
  wxStaticText* label_text_rich_;
  LogWindow* p_text_rich_;

  cDisplayGraphic* p_display_graphic_;

  wxChoice* p_combo_profiles_;

  std::unique_ptr<config::game_title_map_t> p_titles_map_;
  wxTextCtrl* p_text_name_;
  wxTextCtrl* p_text_profile_id_;
  wxTextCtrl* p_text_profile_game_title_;
  wxCheckBox* p_check_use_default_padding_;
  wxDataViewListCtrl* p_view_mapping_data_;
  // global hotkey has to be wrapped in a smart pointer to avoid a bug
  // where my same object would be deleted following the initialization of my
  // frame
  std::unique_ptr<GlobalHotkey> hotkey_alternate_mode_;

  Frame(wxPoint, wxSize);
  ~Frame(){};
  void InitializeSettings();
  void UpdateGuiUsingSettings();

public:
  // menu handlers
  void OnExit(wxCommandEvent& event);
  void OnAbout(wxCommandEvent& event);
  void OnSave(wxCommandEvent& event);
  void OnGlobalHotkey(wxKeyEvent& event);
  void OnReload(wxCommandEvent& event);
  void OnSettings(wxCommandEvent& event);
  // controls handlers
  void PopulateComboBoxWithProfiles();
  void PopulateSettings();

  void OnStart(wxCommandEvent& event);
  void OnStop(wxCommandEvent& event);
  void OnShowLog(wxCommandEvent& event);
  void OnActiveProfile(wxCommandEvent& event);
  void OnAddProfile(wxCommandEvent& event);
  void OnRemoveProfile(wxCommandEvent& event);
  void OnDuplicateProfile(wxCommandEvent& event);

  // display config controls
  void LoadDisplaySettings();
  int m_ival = 0;
  void OnName(wxCommandEvent& event);
  void OnProfileID(wxCommandEvent& event);
  void OnPickTitle(wxCommandEvent& event);
  void OnUseDefaultPadding(wxCommandEvent& event);
  void OnMappingData(wxDataViewEvent& event);
  void OnAddDisplay(wxCommandEvent& event);
  void OnRemoveDisplay(wxCommandEvent& event);
  void OnMoveUp(wxCommandEvent& event);
  void OnMoveDown(wxCommandEvent& event);
};

//////////////////////////////////////////////////////////////////////
//                         Main Application                         //
//////////////////////////////////////////////////////////////////////

class App : public wxApp
{
public:
  App(){};
  ~App(){};

  virtual bool OnInit();
  virtual int OnExit();
  virtual void OnUnhandledException()
  {
    wxLogFatalError("An unhandled exception has occurred. "
                    "Application will now terminate.");
    std::terminate();
  }

private:
  Frame* p_frame_ = nullptr;
};

wxDECLARE_APP(App);

#endif /* TRACKIRMOUSE_GUI_H */
