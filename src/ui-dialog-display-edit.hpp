/**
 * Main app frame window and GUI components.
 *
 * --License Boilerplate Placeholder--
 *
 */

#ifndef EDIT_DISPLAY_HPP
#define EDIT_DISPLAY_HPP

#include <array>
#include <string>

#include <wx/spinctrl.h>
#include <wx/statline.h>
#include <wx/wx.h>

#include "ui-dialog-utilities.hpp"
#include "utility.hpp"

class DialogDisplayEdit : public wxDialog
{
private:
  wxSpinCtrl* rotation_left;
  wxSpinCtrl* rotation_right;
  wxSpinCtrl* rotation_top;
  wxSpinCtrl* rotation_bottom;

public:
  DialogDisplayEdit(wxWindow* parent);
  ~DialogDisplayEdit(){};
};

DialogDisplayEdit::DialogDisplayEdit(wxWindow* parent)
  : wxDialog(parent, wxID_ANY, "Track IR - Display Edit")
{
  auto buttons = new PanelEndModalDialogButtons(this, this);
  buttons->AddButton("Okay", wxID_OK);
  // Future functionality.
  // buttons->AddButton("Apply", wxID_APPLY);
  buttons->AddButton("Cancel", wxID_CANCEL);

  // Logging Window
  auto label_description =
    new wxStaticText(this, wxID_ANY, "Choose the boundaries for the display.");
  auto label_rotation =
    new wxStaticText(this, wxID_ANY, "Display Bounds in 'Game Degrees'  ");
  auto label_left = new wxStaticText(this, wxID_ANY, "Left:");
  auto label_right = new wxStaticText(this, wxID_ANY, "Right:");
  auto label_top = new wxStaticText(this, wxID_ANY, "Top:");
  auto label_bottom = new wxStaticText(this, wxID_ANY, "Bottom:");

  rotation_left = new wxSpinCtrl(this,
                                 wxID_ANY,
                                 "180",
                                 wxDefaultPosition,
                                 wxDefaultSize,
                                 wxSP_ARROW_KEYS,
                                 -180,
                                 180,
                                 60);
  rotation_right = new wxSpinCtrl(this,
                                  wxID_ANY,
                                  "180",
                                  wxDefaultPosition,
                                  wxDefaultSize,
                                  wxSP_ARROW_KEYS,
                                  -180,
                                  180,
                                  60);
  rotation_top = new wxSpinCtrl(this,
                                wxID_ANY,
                                "180",
                                wxDefaultPosition,
                                wxDefaultSize,
                                wxSP_ARROW_KEYS,
                                -180,
                                180,
                                60);
  rotation_bottom = new wxSpinCtrl(this,
                                   wxID_ANY,
                                   "180",
                                   wxDefaultPosition,
                                   wxDefaultSize,
                                   wxSP_ARROW_KEYS,
                                   -180,
                                   180,
                                   60);

  auto static_line = new wxStaticLine(
    this, wxID_ANY, wxDefaultPosition, wxSize(100, 2), wxLI_HORIZONTAL);

  constexpr int SMALL_SPACE = 6;
  constexpr int BIG_SPACE = 12;

  // Static Line Titles
  auto label_sizer_1 = new wxBoxSizer(wxHORIZONTAL);
  label_sizer_1->Add(label_rotation, 0, wxALIGN_CENTER_VERTICAL, 0);
  label_sizer_1->Add(static_line, 1, wxALIGN_CENTER_VERTICAL, 0);

  // Horizontal Sub-Sizers
  auto rotation_sizer = new wxGridSizer(2, wxSize(SMALL_SPACE, SMALL_SPACE));
  rotation_sizer->Add(label_left, 0, wxALIGN_CENTER_VERTICAL, 0);
  rotation_sizer->Add(rotation_left, 0, wxALIGN_CENTER_VERTICAL, 0);
  rotation_sizer->Add(label_right, 0, wxALIGN_CENTER_VERTICAL, 0);
  rotation_sizer->Add(rotation_right, 0, wxALIGN_CENTER_VERTICAL, 0);
  rotation_sizer->Add(label_top, 0, wxALIGN_CENTER_VERTICAL, 0);
  rotation_sizer->Add(rotation_top, 0, wxALIGN_CENTER_VERTICAL, 0);
  rotation_sizer->Add(label_bottom, 0, wxALIGN_CENTER_VERTICAL, 0);
  rotation_sizer->Add(rotation_bottom, 0, wxALIGN_CENTER_VERTICAL, 0);

  // Main Layout
  auto sizer = new wxBoxSizer(wxVERTICAL);
  sizer->Add(label_description, 0, wxEXPAND | wxBOTTOM, BIG_SPACE);
  sizer->Add(label_sizer_1, 0, wxEXPAND | wxBOTTOM, BIG_SPACE);
  sizer->Add(rotation_sizer, 0, wxALL, SMALL_SPACE);

  auto* top_sizer = new wxBoxSizer(wxVERTICAL);
  top_sizer->Add(sizer, 0, wxALL | wxEXPAND, BORDER_SPACING);
  top_sizer->Add(
    buttons, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, BORDER_SPACING);

  SetSizer(top_sizer);

  // Fit the frame around the size of my controls.
  top_sizer->Fit(this);

  this->CenterOnParent();
}

#endif // EDIT_DISPLAY_HPP
