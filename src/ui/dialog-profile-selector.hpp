#pragma once

#include <string>
#include <utility>
#include <vector>

#include <wx/listctrl.h>
#include <wx/wx.h>

#include "ui/dialog-utilities.hpp"

class DialogProfileIdSelector : public wxDialog
{
private:
  wxListView* p_list_view_;
  int profile_id_ = 0;
  std::vector<std::pair<std::string, std::string>> profile_id_map_;

public:
  DialogProfileIdSelector(
    wxWindow* parent,
    int current_profile_id,
    // TODO: switch to actual ordered map?
    const std::vector<std::pair<std::string, std::string>>& game_title_list)
    : wxDialog(parent,
               wxID_ANY,
               "Pick an associated game tile.",
               wxDefaultPosition,
               wxDefaultSize, //  wxSize(500, 800),
               wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
  {

    /*auto panel = new wxPanel(
      this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxBORDER_NONE);*/
    auto buttons = new PanelEndModalDialogButtons(this, this);
    buttons->AddButton("Okay", wxID_OK);
    buttons->AddButton("Cancel", wxID_CANCEL);

    profile_id_ = current_profile_id;
    profile_id_map_ = game_title_list;

    p_list_view_ = new wxListView(this,
                                  wxID_ANY,
                                  wxDefaultPosition,
                                  wxSize(325, 600),
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
      p_list_view_->InsertItem(i, title); // don't check for success?
      p_list_view_->SetItem(i, 1, id_string);
    }

    // set selection of current profile id
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

    p_list_view_->Bind(
      wxEVT_LIST_ITEM_SELECTED, &DialogProfileIdSelector::OnSelection, this);

    wxBoxSizer* top = new wxBoxSizer(wxVERTICAL);
    top->Add(p_list_view_, 1, wxEXPAND | wxALL, BORDER_SPACING);
    top->Add(
      buttons, 0, wxEXPAND | wxBOTTOM | wxLEFT | wxRIGHT, BORDER_SPACING);
    SetSizer(top);
    // Fit the frame around the size of my controls.
    top->Fit(this);

    this->CenterOnParent();
  }

  void OnSelection(wxListEvent& event)
  {
    auto idx =
      p_list_view_->GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
    // get info from underlying data
    auto pair = profile_id_map_[idx];
    profile_id_ = std::stoi(std::get<1>(pair));
    return;
  }
  int GetSelectedProfileId() { return profile_id_; }
};