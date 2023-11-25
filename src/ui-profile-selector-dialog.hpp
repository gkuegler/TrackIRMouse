#ifndef TRACKIRMOUSE_GUIDIALOGS_H
#define TRACKIRMOUSE_GUIDIALOGS_H

#include <wx/listctrl.h>
// #include <wx/propdlg.h>
#include <wx/textctrl.h>
#include <wx/wx.h>

#include <string>

#include "config.hpp"

class cProfileIdSelectorPanel : public wxPanel
{
public:
  wxListView* p_list_view_;
  int* p_profile_id_ = nullptr;
  std::vector<std::pair<std::string, std::string>> profile_id_map_;

  /**
   * Expects id_map to be sorted alphabetically
   */
  cProfileIdSelectorPanel(
    wxWindow* parent,
    int* p_profile_id,
    const std::vector<std::pair<std::string, std::string>>& id_map)
    : wxPanel(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0)
  {
    p_profile_id_ = p_profile_id;
    profile_id_map_ = id_map;

    p_list_view_ = new wxListView(this,
                                  wxID_ANY,
                                  wxDefaultPosition,
                                  wxDefaultSize,
                                  wxLC_REPORT | wxLC_SINGLE_SEL,
                                  wxDefaultValidator,
                                  "");

    // create list control columns
    p_list_view_->InsertColumn(0, "Tile", wxLIST_FORMAT_LEFT, 200);
    p_list_view_->InsertColumn(1, "ID", wxLIST_FORMAT_LEFT, wxLIST_AUTOSIZE);

    // populate list
    for (int i = 0; i < profile_id_map_.size(); i++) {
      const auto& pair = profile_id_map_[i];
      const auto& title = std::get<0>(pair);
      const auto& id_string = std::get<1>(pair);
      p_list_view_->InsertItem(i, title); // don't check for success
      p_list_view_->SetItem(i, 1, id_string);
    }

    // don't do this, move to constructor and dep inject
    auto active_id = config::Get()->GetActiveProfile().profile_id;
    int selection = -1;

    // set selection of current profile p_profile_id_
    // try {
    //  if (std::stoi(id_string) == active_id) {
    //    selection = i;
    //  }
    //} catch (std::invalid_argument) {
    //  spdlog::error("could not convert id: {}, to an integer.", id_string);
    //}
    // if (selection > -1) {
    //  p_list_view_->Select(selection);
    //}

    wxBoxSizer* top = new wxBoxSizer(wxVERTICAL);
    top->Add(p_list_view_, 1, wxEXPAND, 0);
    SetSizer(top);

    p_list_view_->Bind(
      wxEVT_LIST_ITEM_SELECTED, &cProfileIdSelectorPanel::OnSelection, this);
  }

  void OnSelection(wxListEvent& event)
  {
    auto idx =
      p_list_view_->GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
    // get info from underlying data
    auto pair = profile_id_map_[idx];
    *p_profile_id_ = std::stoi(std::get<1>(pair));
    return;
  }
};

class cOkayCancelDlgButtons : public wxPanel
{
public:
  wxButton* okay_;
  wxButton* cancel_;
  wxDialog* dialog_;

  cOkayCancelDlgButtons(wxDialog* parent)
    : wxPanel(parent)
  {
    dialog_ = parent;
    okay_ =
      new wxButton(this, wxID_ANY, "Okay", wxDefaultPosition, wxSize(110, 25));
    cancel_ = new wxButton(
      this, wxID_ANY, "Cancel", wxDefaultPosition, wxSize(110, 25));

    auto top = new wxBoxSizer(wxHORIZONTAL);
    top->Add(okay_, 0, wxALL, 0);
    top->Add(cancel_, 0, wxALL, 0);
    SetSizer(top);

    okay_->Bind(wxEVT_BUTTON, &cOkayCancelDlgButtons::OnOkay, this);
    cancel_->Bind(wxEVT_BUTTON, &cOkayCancelDlgButtons::OnCancel, this);
  }

  void OnOkay(wxCommandEvent& event) { dialog_->EndModal(wxID_OK); }
  void OnCancel(wxCommandEvent& event) { dialog_->EndModal(wxID_CANCEL); }
};

class cProfileIdSelector : public wxDialog
{
public:
  cProfileIdSelectorPanel* panel_;
  cOkayCancelDlgButtons* buttons_;
  cProfileIdSelector(
    wxWindow* parent,
    int* p_profile_id_,
    const std::vector<std::pair<std::string, std::string>>& game_title_list)
    : wxDialog(parent,
               wxID_ANY,
               "Pick an associated game tile.",
               wxDefaultPosition,
               wxSize(500, 800),
               wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
  {
    this->CenterOnParent();

    panel_ = new cProfileIdSelectorPanel(this, p_profile_id_, game_title_list);
    buttons_ = new cOkayCancelDlgButtons(this);
    auto top = new wxBoxSizer(wxVERTICAL);
    top->Add(panel_, 1, wxEXPAND, 0);
    top->Add(buttons_, 0, wxALL | wxALIGN_CENTER_HORIZONTAL, 10);
    SetSizer(top);
  }
};

#endif /* TRACKIRMOUSE_GUIDIALOGS_H */
