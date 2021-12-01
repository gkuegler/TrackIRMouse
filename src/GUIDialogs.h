#ifndef TRACKIRMOUSE_GUIDIALOGS_H
#define TRACKIRMOUSE_GUIDIALOGS_H

#include "Config.h"
#include "ControlIDs.h"

#include <wx/bookctrl.h>
#include <wx/propdlg.h>
#include <wx/choicdlg.h>
#include <wx/textctrl.h>

class cSettingsGeneralPanel : public wxPanel {
public:
	wxCheckBox* m_cbxEnableWatchdog;
	wxCheckBox* m_cbxTrackOnStart;
	wxCheckBox* m_cbxQuitOnLossOfTrackIR;
	cSettingsGeneralPanel(wxWindow* parent, SData* pUserData);

private:
	SData* m_pUserData = nullptr;
	void OnEnabledWatchdog(wxCommandEvent& event);
	void OnTrackOnStart(wxCommandEvent& event);
	void OnQuitOnLossOfTrackIr(wxCommandEvent& event);
	wxDECLARE_EVENT_TABLE();
};

class cSettingsAdvancedlPanel : public wxPanel {
public:
	wxTextCtrl* m_txtTrackIrDllPath;
	cSettingsAdvancedlPanel(wxWindow* parent, SData* pUserData);

private:
	SData* m_pUserData = nullptr;
	void OnTrackIrDllPath(wxCommandEvent& event);
	wxDECLARE_EVENT_TABLE();
};

class cSettingsPopup : public wxPropertySheetDialog {
public:
	SData* m_userData = nullptr;
	cSettingsPopup(wxWindow* parent, SData* pUserData);

private:
	cSettingsGeneralPanel* m_pnlGen;
	cSettingsAdvancedlPanel* m_pnlAdv;
};

//class cRemoveProfile : public wxMultiChoiceDialog {
//public:
//};

#endif /* TRACKIRMOUSE_GUIDIALOGS_H */