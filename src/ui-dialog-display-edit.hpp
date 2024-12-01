/**
 * Main app frame window and GUI components.
 *
 * --License Boilerplate Placeholder--
 *
 */

#pragma once

#include <array>
#include <string>

#include <wx/spinctrl.h>
#include <wx/statline.h>
#include <wx/wx.h>

#include "settings.hpp"
#include "types.hpp"
#include "ui-dialog-utilities.hpp"
#include "utility.hpp"

class DialogDisplayEdit : public wxDialog
{
private:
  wxSpinCtrl* rotation_left;
  wxSpinCtrl* rotation_right;
  wxSpinCtrl* rotation_top;
  wxSpinCtrl* rotation_bottom;
  wxSpinCtrl* padding_left;
  wxSpinCtrl* padding_right;
  wxSpinCtrl* padding_top;
  wxSpinCtrl* padding_bottom;

public:
  DialogDisplayEdit(wxWindow* parent, const settings::UserDisplay display)
    : wxDialog(parent, wxID_ANY, "Track IR - Display Edit")
  {
    auto buttons = new PanelEndModalDialogButtons(this, this);
    buttons->AddButton("Okay", wxID_OK);
    // Future functionality.
    // buttons->AddButton("Apply", wxID_APPLY);
    buttons->AddButton("Cancel", wxID_CANCEL);

    // Logging Window
    auto label_description = new wxStaticText(
      this, wxID_ANY, "Choose the boundaries for the display.");
    auto label_rotation =
      new wxStaticText(this, wxID_ANY, "Display Bounds in 'Game Degrees'  ");
    auto label_padding =
      new wxStaticText(this, wxID_ANY, "Display Bounds in 'Game Degrees'  ");
    auto label_left = new wxStaticText(this, wxID_ANY, "Left:");
    auto label_right = new wxStaticText(this, wxID_ANY, "Right:");
    auto label_top = new wxStaticText(this, wxID_ANY, "Top:");
    auto label_bottom = new wxStaticText(this, wxID_ANY, "Bottom:");
    auto label_left_2 = new wxStaticText(this, wxID_ANY, "Left:");
    auto label_right_2 = new wxStaticText(this, wxID_ANY, "Right:");
    auto label_top_2 = new wxStaticText(this, wxID_ANY, "Top:");
    auto label_bottom_2 = new wxStaticText(this, wxID_ANY, "Bottom:");

    rotation_left = new wxSpinCtrl(this,
                                   wxID_ANY,
                                   "",
                                   wxDefaultPosition,
                                   wxDefaultSize,
                                   wxSP_ARROW_KEYS,
                                   -180,
                                   180,
                                   display.rotation[LEFT_EDGE]);
    rotation_right = new wxSpinCtrl(this,
                                    wxID_ANY,
                                    "",
                                    wxDefaultPosition,
                                    wxDefaultSize,
                                    wxSP_ARROW_KEYS,
                                    -180,
                                    180,
                                    display.rotation[RIGHT_EDGE]);
    rotation_top = new wxSpinCtrl(this,
                                  wxID_ANY,
                                  "",
                                  wxDefaultPosition,
                                  wxDefaultSize,
                                  wxSP_ARROW_KEYS,
                                  -180,
                                  180,
                                  display.rotation[TOP_EDGE]);
    rotation_bottom = new wxSpinCtrl(this,
                                     wxID_ANY,
                                     "",
                                     wxDefaultPosition,
                                     wxDefaultSize,
                                     wxSP_ARROW_KEYS,
                                     -180,
                                     180,
                                     display.rotation[BOTTOM_EDGE]);

    auto static_line = new wxStaticLine(
      this, wxID_ANY, wxDefaultPosition, wxSize(100, 2), wxLI_HORIZONTAL);
    auto static_line_2 = new wxStaticLine(
      this, wxID_ANY, wxDefaultPosition, wxSize(100, 2), wxLI_HORIZONTAL);

    padding_left = new wxSpinCtrl(this,
                                  wxID_ANY,
                                  "",
                                  wxDefaultPosition,
                                  wxDefaultSize,
                                  wxSP_ARROW_KEYS,
                                  -180,
                                  180,
                                  display.padding[LEFT_EDGE]);
    padding_right = new wxSpinCtrl(this,
                                   wxID_ANY,
                                   "",
                                   wxDefaultPosition,
                                   wxDefaultSize,
                                   wxSP_ARROW_KEYS,
                                   -180,
                                   180,
                                   display.padding[RIGHT_EDGE]);
    padding_top = new wxSpinCtrl(this,
                                 wxID_ANY,
                                 "",
                                 wxDefaultPosition,
                                 wxDefaultSize,
                                 wxSP_ARROW_KEYS,
                                 -180,
                                 180,
                                 display.padding[TOP_EDGE]);
    padding_bottom = new wxSpinCtrl(this,
                                    wxID_ANY,
                                    "",
                                    wxDefaultPosition,
                                    wxDefaultSize,
                                    wxSP_ARROW_KEYS,
                                    -180,
                                    180,
                                    display.padding[BOTTOM_EDGE]);

    constexpr int SMALL_SPACE = 6;
    constexpr int BIG_SPACE = 12;

    // Static Line Titles
    auto label_sizer_1 = new wxBoxSizer(wxHORIZONTAL);
    label_sizer_1->Add(label_rotation, 0, wxALIGN_CENTER_VERTICAL, 0);
    label_sizer_1->Add(static_line, 1, wxALIGN_CENTER_VERTICAL, 0);

    auto label_sizer_2 = new wxBoxSizer(wxHORIZONTAL);
    label_sizer_2->Add(label_padding, 0, wxALIGN_CENTER_VERTICAL, 0);
    label_sizer_2->Add(static_line_2, 1, wxALIGN_CENTER_VERTICAL, 0);

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

    auto padding_sizer = new wxGridSizer(2, wxSize(SMALL_SPACE, SMALL_SPACE));
    padding_sizer->Add(label_left_2, 0, wxALIGN_CENTER_VERTICAL, 0);
    padding_sizer->Add(padding_left, 0, wxALIGN_CENTER_VERTICAL, 0);
    padding_sizer->Add(label_right_2, 0, wxALIGN_CENTER_VERTICAL, 0);
    padding_sizer->Add(padding_right, 0, wxALIGN_CENTER_VERTICAL, 0);
    padding_sizer->Add(label_top_2, 0, wxALIGN_CENTER_VERTICAL, 0);
    padding_sizer->Add(padding_top, 0, wxALIGN_CENTER_VERTICAL, 0);
    padding_sizer->Add(label_bottom_2, 0, wxALIGN_CENTER_VERTICAL, 0);
    padding_sizer->Add(padding_bottom, 0, wxALIGN_CENTER_VERTICAL, 0);

    // Main Layout
    auto sizer = new wxBoxSizer(wxVERTICAL);
    sizer->Add(label_description, 0, wxEXPAND | wxBOTTOM, BIG_SPACE);
    sizer->Add(label_sizer_1, 0, wxEXPAND | wxBOTTOM, BIG_SPACE);
    sizer->Add(rotation_sizer, 0, wxALL, SMALL_SPACE);
    sizer->Add(label_sizer_2, 0, wxEXPAND | wxTOP | wxBOTTOM, BIG_SPACE);
    sizer->Add(padding_sizer, 0, wxALL, SMALL_SPACE);

    auto top = new wxBoxSizer(wxVERTICAL);
    top->Add(sizer, 0, wxALL | wxEXPAND, BORDER_SPACING);
    top->Add(
      buttons, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, BORDER_SPACING);

    SetSizer(top);

    // Fit the frame around the size of my controls.
    top->Fit(this);

    this->CenterOnParent();
  }
  void ApplyChanges(settings::UserDisplay& display)
  {
    display.rotation = {
      static_cast<double>(rotation_left->GetValue()),
      static_cast<double>(rotation_right->GetValue()),
      static_cast<double>(rotation_top->GetValue()),
      static_cast<double>(rotation_bottom->GetValue()),
    };
    display.padding = {
      static_cast<signed int>(padding_left->GetValue()),
      static_cast<signed int>(padding_right->GetValue()),
      static_cast<signed int>(padding_top->GetValue()),
      static_cast<signed int>(padding_bottom->GetValue()),
    };
  }
};