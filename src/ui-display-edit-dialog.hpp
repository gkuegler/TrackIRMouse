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

class DisplayEditDialog : public wxDialog
{
private:
  wxSpinCtrl* rotation_left;
  wxSpinCtrl* rotation_right;
  wxSpinCtrl* rotation_top;
  wxSpinCtrl* rotation_bottom;

public:
  DisplayEditDialog(wxWindow* parent);
  ~DisplayEditDialog(){};
};

DisplayEditDialog::DisplayEditDialog(wxWindow* parent)
  : wxDialog(parent, wxID_ANY, "Track IR - Display Edit"
             // wxPoint(200, 200),
             // wxSize(500, 500)
    )
{
  const wxSize k_default_button_size = wxSize(110, 25);
  const wxSize k_default_button_size_2 = wxSize(150, 25);
  const constexpr int k_max_profile_length = 30;
  const wxTextValidator alphanumeric_validator(wxFILTER_ALPHANUMERIC);

  // colors used for testing
  const wxColor yellow(255, 255, 0);
  const wxColor blue(255, 181, 102);
  const wxColor pink(198, 102, 255);
  const wxColor green(142, 255, 102);
  const wxColor orange(102, 201, 255);

  // Panels
  auto panel = new wxPanel(
    this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxBORDER_NONE);
  // auto buttons =
  // new OkayApplyCancelDialogButtons(panel, this, wxID_OK | wxID_CANCEL);
  auto buttons = new OkayApplyCancelDialogButtons(panel, this);
  buttons->AddButton(wxID_OK, "Okay");
  buttons->AddButton(wxID_CANCEL, "Cancel");
  buttons->AddButton(wxID_APPLY, "Apply");

  // Logging Window
  auto label_description =
    new wxStaticText(panel, wxID_ANY, "Choose the boundaries for the display.");
  auto label_rotation =
    new wxStaticText(panel, wxID_ANY, "Display Bounds in 'Game Degrees'  ");
  auto label_left = new wxStaticText(panel, wxID_ANY, "Left:");
  auto label_right = new wxStaticText(panel, wxID_ANY, "Right:");
  auto label_top = new wxStaticText(panel, wxID_ANY, "Top:");
  auto label_bottom = new wxStaticText(panel, wxID_ANY, "Bottom:");

  auto rotation_left = new wxSpinCtrl(panel,
                                      wxID_ANY,
                                      "180",
                                      wxDefaultPosition,
                                      wxDefaultSize,
                                      wxSP_ARROW_KEYS,
                                      -180,
                                      180,
                                      60);
  auto rotation_right = new wxSpinCtrl(panel,
                                       wxID_ANY,
                                       "180",
                                       wxDefaultPosition,
                                       wxDefaultSize,
                                       wxSP_ARROW_KEYS,
                                       -180,
                                       180,
                                       60);
  auto rotation_top = new wxSpinCtrl(panel,
                                     wxID_ANY,
                                     "180",
                                     wxDefaultPosition,
                                     wxDefaultSize,
                                     wxSP_ARROW_KEYS,
                                     -180,
                                     180,
                                     60);
  auto rotation_bottom = new wxSpinCtrl(panel,
                                        wxID_ANY,
                                        "180",
                                        wxDefaultPosition,
                                        wxDefaultSize,
                                        wxSP_ARROW_KEYS,
                                        -180,
                                        180,
                                        60);

  auto static_line = new wxStaticLine(
    panel, wxID_ANY, wxDefaultPosition, wxSize(100, 2), wxLI_HORIZONTAL);

  constexpr int SMALL_SPACE = 6;
  constexpr int BIG_SPACE = 12;
  constexpr int BORDER_SPACE = 12;

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
  auto top_sizer = new wxBoxSizer(wxVERTICAL);
  // start up settings
  top_sizer->Add(label_description, 0, wxEXPAND | wxBOTTOM, BIG_SPACE);
  top_sizer->Add(label_sizer_1, 0, wxEXPAND | wxBOTTOM, BIG_SPACE);
  top_sizer->Add(rotation_sizer, 0, wxALL, SMALL_SPACE);

  auto* border_sizer = new wxBoxSizer(wxVERTICAL);
  border_sizer->Add(top_sizer, 0, wxALL | wxEXPAND, BORDER_SPACE);
  border_sizer->Add(
    buttons, 0, wxALL | wxALIGN_CENTER_HORIZONTAL, BORDER_SPACE);

  panel->SetSizer(border_sizer);

  // Fit the frame around the size of my controls.
  border_sizer->Fit(this);
}

#endif // EDIT_DISPLAY_HPP
