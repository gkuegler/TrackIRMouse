#ifndef TRACKIRMOUSE_UI_FRAME_H
#define TRACKIRMOUSE_UI_FRAME_H

#include <wx/dataview.h>
#include <wx/wx.h>

#include "game-titles.hpp"
#include "hooks.hpp"
#include "hotkey.hpp"
#include "settings.hpp"
#include "ui-graphic.hpp"

// forward decl of thread classes
class ThreadHeadTracking;
class ThreadPipeServer;

//////////////////////////////////////////////////////////////////////
//                Text Control Status Output Window                 //
//////////////////////////////////////////////////////////////////////

class LogOutputControl : public wxTextCtrl
{
public:
  LogOutputControl(wxWindow* parent,
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
class MainWindow : public wxFrame
{
public:
  std::unique_ptr<game_title_map_t> p_titles_map_;
  GameTitleVector titles_;

  // threads
  ThreadHeadTracking* track_thread_ = nullptr;
  ThreadPipeServer* pipe_server_thread_ = nullptr;
  wxCriticalSection cs_track_thread_; // protects track thread
  wxCriticalSection cs_pipe_thread_;  // protects pipe server thread

  // components
  PanelDisplayGraphic* p_display_graphic_;
  LogOutputControl* p_text_rich_;

  // controls
  wxStaticText* label_text_rich_;
  wxChoice* p_combo_profiles_;
  wxTextCtrl* p_text_name_;
  wxTextCtrl* p_text_profile_id_;
  wxTextCtrl* p_text_profile_game_title_;
  wxCheckBox* p_check_use_default_padding_;
  wxDataViewListCtrl* p_view_mapping_data_;

  // Global hotkey has to be wrapped in a smart pointer to avoid a bug
  // where my same object would be deleted following the initialization of my
  // frame.
  std::unique_ptr<GlobalHotkey> hotkey_alternate_mode_;
  std::unique_ptr<HookWindowChanged> hook_window_changed_;

public:
  MainWindow(wxPoint, wxSize);
  ~MainWindow();

  void StartScrollAlternateHooksAndHotkeys();
  void StartPipeServer();
  void StopPipeServer();
  void RemoveHooks();
  void UpdateGuiFromSettings();

  // void StopTrackThread();
  //  menu handlers
  void OnExit(wxCommandEvent& event);
  void OnAbout(wxCommandEvent& event);
  void OnSave(wxCommandEvent& event);
  void OnScrollAlternateHotkey(wxKeyEvent& event);
  void OnReload(wxCommandEvent& event);
  void OnSettings(wxCommandEvent& event);
  void OnLogFile(wxCommandEvent& event);
  // control handlers
  void OnStart(wxCommandEvent& event);
  void OnStop(wxCommandEvent& event);
  void OnActiveProfile(wxCommandEvent& event);
  void OnAddProfile(wxCommandEvent& event);
  void OnRemoveProfile(wxCommandEvent& event);
  void OnDuplicateProfile(wxCommandEvent& event);
  // display config controls
  void UpdateProfilePanelFromSettings();
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
  void OnDisplayEdit(wxCommandEvent& event);
  // helpers
  void PopulateComboBoxWithProfiles();
  void PopulateSettings();
};

#endif /* TRACKIRMOUSE_UI_FRAME_H */
