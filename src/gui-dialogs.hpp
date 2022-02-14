#ifndef TRACKIRMOUSE_GUIDIALOGS_H
#define TRACKIRMOUSE_GUIDIALOGS_H

#include <wx/propdlg.h>
#include <wx/textctrl.h>
#include <wx/wx.h>

#include "config.hpp"

class cSettingsGeneralPanel : public wxPanel {
 public:
  wxCheckBox* m_cbxEnableWatchdog;
  wxCheckBox* m_cbxTrackOnStart;
  wxCheckBox* m_cbxQuitOnLossOfTrackIR;
  cSettingsGeneralPanel(wxWindow* parent, config::UserData* pUserData);

 private:
  config::UserData* m_pUserData = nullptr;
  void OnEnabledWatchdog(wxCommandEvent& event);
  void OnTrackOnStart(wxCommandEvent& event);
  void OnQuitOnLossOfTrackIr(wxCommandEvent& event);
  wxDECLARE_EVENT_TABLE();
};

class cSettingsAdvancedlPanel : public wxPanel {
 public:
  wxTextCtrl* m_txtTrackIrDllPath;
  cSettingsAdvancedlPanel(wxWindow* parent, config::UserData* pUserData);

 private:
  config::UserData* m_pUserData = nullptr;
  void OnTrackIrDllPath(wxCommandEvent& event);
  wxDECLARE_EVENT_TABLE();
};

class cSettingsPopup : public wxPropertySheetDialog {
 public:
  config::UserData* m_userData = nullptr;
  cSettingsPopup(wxWindow* parent, config::UserData* pUserData);

 private:
  cSettingsGeneralPanel* m_pnlGen;
  cSettingsAdvancedlPanel* m_pnlAdv;
};

#endif /* TRACKIRMOUSE_GUIDIALOGS_H */
