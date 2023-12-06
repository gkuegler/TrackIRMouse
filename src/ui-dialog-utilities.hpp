/**
 * Main app frame window and GUI components.
 *
 * --License Boilerplate Placeholder--
 *
 */

#ifndef TRACKIRMOUSE_DIALOG_UTILITIES_HPP
#define TRACKIRMOUSE_DIALOG_UTILITIES_HPP

#include <array>
#include <string>

#include <wx/statline.h>
#include <wx/wx.h>

#include "log.hpp"
#include "settings.hpp"
#include "utility.hpp"

enum DIALOG_BUTTONS_FLAG
{
  DIALOG_BUTTON_OKAY = 1,
  DIALOG_BUTTON_APPLY = 2,
  DIALOG_BUTTON_CANCEL = 4
};

class DialogEndModalButtonsPanel : public wxPanel
{
public:
  wxWindow* parent;
  wxDialog* dialog;
  std::unordered_map<wxButton*, int> object_ret_code_table_;

  DialogEndModalButtonsPanel(wxWindow* parent_, wxDialog* dialogue_)
    : wxPanel(parent_)
  {
    parent = parent_;
    dialog = dialogue_;

    auto top = new wxBoxSizer(wxHORIZONTAL);
    SetSizer(top);
  }
  void AddButton(const char* label, int ret_code)
  {
    auto button =
      new wxButton(this, wxID_ANY, label, wxDefaultPosition, wxSize(110, 25));

#ifdef _DEBUG
    // Debugging check to make sure each button added has a different return
    // code.
    for (const auto& [key, value] : object_ret_code_table_) {
      if (ret_code == value) {
        throw std::runtime_error("Duplicate id value found for button.");
      }
    }
#endif
    object_ret_code_table_[button] = ret_code;

    auto sizer = this->GetSizer();
    sizer->Add(button, 0, wxALL, 0);

    button->Bind(wxEVT_BUTTON, &DialogEndModalButtonsPanel::OnClick, this);
  }

  void OnClick(wxCommandEvent& event)
  {
    auto id = event.GetId();
    auto button = (wxButton*)event.GetEventObject();
    auto ret_code = object_ret_code_table_[button];
    dialog->EndModal(ret_code);
  }
};
#endif // TRACKIRMOUSE_DIALOG_UTILITIES_HPP
