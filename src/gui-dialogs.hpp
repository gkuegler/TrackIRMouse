#ifndef TRACKIRMOUSE_GUIDIALOGS_H
#define TRACKIRMOUSE_GUIDIALOGS_H

#include <wx/listctrl.h>
#include <wx/propdlg.h>
#include <wx/textctrl.h>
#include <wx/wx.h>

#include <string>

#include "config.hpp"

class cSettingsGeneralPanel : public wxPanel {
 public:
  wxCheckBox *m_cbxEnableWatchdog;
  wxCheckBox *m_cbxTrackOnStart;
  wxCheckBox *m_cbxQuitOnLossOfTrackIR;
  wxChoice *m_cmbLogLevel;
  cSettingsGeneralPanel(wxWindow *parent, config::UserData *pUserData);

 private:
  config::UserData *m_pUserData = nullptr;
  void OnEnabledWatchdog(wxCommandEvent &event);
  void OnTrackOnStart(wxCommandEvent &event);
  void OnQuitOnLossOfTrackIr(wxCommandEvent &event);
  void OnLogLevel(wxCommandEvent &event);
};

class cSettingsAdvancedlPanel : public wxPanel {
 public:
  wxTextCtrl *m_txtTrackIrDllPath;
  cSettingsAdvancedlPanel(wxWindow *parent, config::UserData *pUserData);

 private:
  config::UserData *m_pUserData = nullptr;
  void OnTrackIrDllPath(wxCommandEvent &event);
};

class cSettingsPopup : public wxPropertySheetDialog {
 public:
  config::UserData *m_userData = nullptr;
  cSettingsPopup(wxWindow *parent, config::UserData *pUserData);

 private:
  cSettingsGeneralPanel *m_pnlGen;
  cSettingsAdvancedlPanel *m_pnlAdv;
};

class cProfileIdSelectorPanel : public wxPanel {
 public:
  wxListView *m_lctProfileIds;
  int *id = nullptr;
  std::vector<std::pair<std::string, std::string>> profileIdList;

  cProfileIdSelectorPanel(
      wxWindow *parent, int *profileId,
      const std::vector<std::pair<std::string, std::string>> &idList)
      : wxPanel(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0) {
    /**
     * Expects idList to be sorted alphabetically
     *
     */

    id = profileId;
    profileIdList = idList;

    m_lctProfileIds =
        new wxListView(this, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                       wxLC_REPORT | wxLC_SINGLE_SEL, wxDefaultValidator, "");

    // create list control columns
    m_lctProfileIds->InsertColumn(0, "Tile", wxLIST_FORMAT_LEFT, 200);
    m_lctProfileIds->InsertColumn(1, "ID", wxLIST_FORMAT_LEFT, wxLIST_AUTOSIZE);

    auto activeId = config::GetActiveProfile().profileId;
    int selection = -1;

    // populate list
    for (int i = 0; i < profileIdList.size(); i++) {
      auto pair = profileIdList[i];
      auto title = std::get<0>(pair);
      auto ids = std::get<1>(pair);
      auto idx = m_lctProfileIds->InsertItem(i, title);
      m_lctProfileIds->SetItem(idx, 1, ids);
      try {
        if (std::stoi(ids) == activeId) {
          selection = i;
        }
      } catch (std::invalid_argument) {
        spdlog::error("could not convert id: {}, to an integer.", ids);
      }
    }

    // set selection of current profile id
    if (selection > -1) {
      m_lctProfileIds->Select(selection);
    }

    wxBoxSizer *top = new wxBoxSizer(wxVERTICAL);
    top->Add(m_lctProfileIds, 1, wxEXPAND, 0);
    SetSizer(top);

    m_lctProfileIds->Bind(wxEVT_LIST_ITEM_SELECTED,
                          &cProfileIdSelectorPanel::OnSelection, this);
  }

  void OnSelection(wxListEvent &event) {
    auto idx = m_lctProfileIds->GetNextItem(-1, wxLIST_NEXT_ALL,
                                            wxLIST_STATE_SELECTED);
    // get info from underlying data
    auto pair = profileIdList[idx];
    *id = std::stoi(std::get<1>(pair));
    return;
  }
};

class cOkayCancelDlgButtons : public wxPanel {
 public:
  wxButton *okay;
  wxButton *cancel;
  wxDialog *dialog;

  cOkayCancelDlgButtons(wxDialog *parent) : wxPanel(parent) {
    dialog = parent;
    okay = new wxButton(this, wxID_ANY, "Okay", wxDefaultPosition,
                        wxSize(110, 25));
    cancel = new wxButton(this, wxID_ANY, "Cancel", wxDefaultPosition,
                          wxSize(110, 25));

    auto top = new wxBoxSizer(wxHORIZONTAL);
    top->Add(okay, 0, wxALL, 0);
    top->Add(cancel, 0, wxALL, 0);
    SetSizer(top);

    okay->Bind(wxEVT_BUTTON, &cOkayCancelDlgButtons::OnOkay, this);
    cancel->Bind(wxEVT_BUTTON, &cOkayCancelDlgButtons::OnCancel, this);
  }

  void OnOkay(wxCommandEvent &event) { dialog->EndModal(wxID_OK); }
  void OnCancel(wxCommandEvent &event) { dialog->EndModal(wxID_CANCEL); }
};

class cProfileIdSelector : public wxDialog {
 public:
  cProfileIdSelectorPanel *panel;
  cOkayCancelDlgButtons *buttons;
  cProfileIdSelector(
      wxWindow *parent, int *id,
      const std::vector<std::pair<std::string, std::string>> &gameTitleList)
      : wxDialog(parent, wxID_ANY, "Pick an associated game tile.",
                 wxDefaultPosition, wxSize(500, 800),
                 wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER) {
    this->CenterOnParent();

    panel = new cProfileIdSelectorPanel(this, id, gameTitleList);
    buttons = new cOkayCancelDlgButtons(this);
    auto top = new wxBoxSizer(wxVERTICAL);
    top->Add(panel, 1, wxEXPAND, 0);
    top->Add(buttons, 0, wxALL | wxALIGN_CENTER_HORIZONTAL, 10);
    SetSizer(top);
  }
};

#include <type_traits>

/**
 * Instantiate a modal dialog used to capture input.
 * Has "Ok" and "Cancel" options.
 * Instantiate with user supplied panel and appropriate handlers within said
 * panel.
 *
 * T = wxPanel used for displaying user defined widgets
 * D = data object to be manupulated by reference by my panel
 * A = argument object supplied to T constructor
 *
 *
 */
template <class T, class D, class A>
class cModalInputDialog : public wxDialog {
 public:
  wxPanel *panel;
  wxPanel *buttons;
  cModalInputDialog(wxWindow *parent, wxString title, wxSize size, D &data,
                    A args)
      : wxDialog(parent, wxID_ANY, title, wxDefaultPosition, size,
                 wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER) {
    static_assert(std::is_base_of<wxPanel, T>::value,
                  "T must inherit from wxPanel");
    panel = new T(this, data, args);
    buttons = new cOkayCancelDlgButtons(this);
    auto top = new wxBoxSizer(wxVERTICAL);
    top->Add(panel, 1, wxEXPAND, 0);
    top->Add(buttons, 0, wxALL | wxALIGN_CENTER_HORIZONTAL, 10);
    SetSizer(top);
  }
  cModalInputDialog(wxWindow *parent, wxString title, wxSize size, D &data,
                    A &&args)
      : wxDialog(parent, wxID_ANY, title, wxDefaultPosition, size,
                 wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER) {
    static_assert(std::is_base_of<wxPanel, T>::value,
                  "T must inherit from wxPanel");
    panel = new T(this, data, std::forward<A>(args));
    buttons = new cOkayCancelDlgButtons(this);
    auto top = new wxBoxSizer(wxVERTICAL);
    top->Add(panel, 1, wxEXPAND, 0);
    top->Add(buttons, 0, wxALL | wxALIGN_CENTER_HORIZONTAL, 10);
    SetSizer(top);
  }
};

// ------- Example Usage -------
// // Define the data object to be manipulated
// typedef struct foo_data {
//   int a = 0;
//   int b = 0;
// } foo_data;

// // Define a struct to contain the arguments.
// // alternatively use a single type.
//  typedef struct foo_args {
//    int arg1 = 0;
//    string arg2 = "This is a string.";
//  } foo_args_;

// // Define the panel to be shown in the dialog box
// class foo {
// public:
//  foo(foo_data& d, foo_args& a){
//    t = a;
//    d.a = 5;
//    // Bind event handlers
//   }
//  // Declare event handlers
// };

// // Implementation in the handler that is launching the dialog
// foo_data data{2, 3};
// foo_args args;
// baz<foo, foo_data, foo_args> dialog("title", data, "hello");
//
// int result = dialogjShowModal();
// if (wxID_OK == result){
//   // do something with the manipulated data
// } else {
//   // generally do nothing here
// }

#endif /* TRACKIRMOUSE_GUIDIALOGS_H */
