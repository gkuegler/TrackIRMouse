/**
 * Main app frame window and GUI components.
 *
 * --License Boilerplate Placeholder--
 *
 */
#pragma once
#ifndef TIRMOUSE_DIALOG_UTILITIES_HPP
#define TIRMOUSE_DIALOG_UTILITIES_HPP

#include <array>
#include <string>

#include <wx/statline.h>
#include <wx/wx.h>

#include "log.hpp"
#include "settings.hpp"
#include "utility.hpp"

constexpr static const int BORDER_SPACING = 12;
static const auto DEFAULT_DIALOG_BUTTON_SIZE = wxSize(100, 25);

class PanelEndModalDialogButtons : public wxPanel
{
private:
  wxWindow* parent;
  wxDialog* dialog;
  wxBoxSizer* btn_szr;
  std::unordered_map<wxButton*, int> object_ret_code_table_;

public:
  PanelEndModalDialogButtons(wxWindow* parent_, wxDialog* dialogue_)
    : wxPanel(parent_)
  {
    parent = parent_;
    dialog = dialogue_;

    // auto static_line = new wxStaticLine(
    // this, wxID_ANY, wxDefaultPosition, wxSize(100, 2), wxLI_HORIZONTAL);

    btn_szr = new wxBoxSizer(wxHORIZONTAL);

    auto top = new wxBoxSizer(wxVERTICAL);
    // top->Add(static_line, 1, wxEXPAND | wxALL | wxALIGN_LEFT, 0);
    // top->Add(btn_szr, 0, wxTOP | wxALIGN_CENTER_HORIZONTAL, BORDER_SPACING);
    top->Add(btn_szr, 0, wxALIGN_CENTER_HORIZONTAL);

    SetSizer(top);
  }
  void AddButton(const char* label, int ret_code)
  {
#ifdef _DEBUG
    // Debugging check to make sure each button added has a different return
    // code.
    for (const auto& [key, value] : object_ret_code_table_) {
      if (ret_code == value) {
        throw std::runtime_error("Duplicate id value found for button.");
      }
    }
#endif
    auto button = new wxButton(
      this, wxID_ANY, label, wxDefaultPosition, DEFAULT_DIALOG_BUTTON_SIZE);

    object_ret_code_table_[button] = ret_code;
    btn_szr->Add(button, 0, wxALL, 0);
    button->Bind(wxEVT_BUTTON, &PanelEndModalDialogButtons::OnClick, this);
  }

  void OnClick(wxCommandEvent& event)
  {
    auto id = event.GetId();
    auto button = (wxButton*)event.GetEventObject();
    auto ret_code = object_ret_code_table_[button];
    dialog->EndModal(ret_code);
  }
};
#endif // TIRMOUSE_DIALOG_UTILITIES_HPP
