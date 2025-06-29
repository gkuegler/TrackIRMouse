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
  int id_ = 0;
  GameTitleList titles_;

public:
  DialogProfileIdSelector(wxWindow* parent, int id, const GameTitleList& titles)
    : id_(id)
    , titles_(titles)
    , wxDialog(parent,
               wxID_ANY,
               "Pick an associated game tile.",
               wxDefaultPosition,
               wxDefaultSize,
               wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
  {
    auto buttons = new PanelEndModalDialogButtons(this, this);
    buttons->AddButton("Okay", wxID_OK);
    buttons->AddButton("Cancel", wxID_CANCEL);

    p_list_view_ = new wxListView(this,
                                  wxID_ANY,
                                  wxDefaultPosition,
                                  wxSize(325, 600),
                                  wxLC_REPORT | wxLC_SINGLE_SEL,
                                  wxDefaultValidator,
                                  "");

    // Create list control columns.
    p_list_view_->InsertColumn(0, "Tile", wxLIST_FORMAT_LEFT, 200);
    p_list_view_->InsertColumn(1, "ID", wxLIST_FORMAT_LEFT, wxLIST_AUTOSIZE);

    // Populate list.
    for (int i = 0; i < titles_.size(); i++) {
      const auto& title = titles_[i];
      p_list_view_->InsertItem(i, title.name); // don't check for success?
      p_list_view_->SetItem(i, 1, title.id);
    }

    // Need to select item before selection event is bound, because this method generates a
    // selection event.
    auto ids = std::to_string(id_);
    for (size_t i = 0; i < titles.size(); i++) {
      if (titles[i].id == ids) {
        p_list_view_->Select(i);
      }
    }

    p_list_view_->Bind(wxEVT_LIST_ITEM_SELECTED, &DialogProfileIdSelector::OnSelection, this);

    wxBoxSizer* top = new wxBoxSizer(wxVERTICAL);
    top->Add(p_list_view_, 1, wxEXPAND | wxALL, BORDER_SPACING);
    top->Add(buttons, 0, wxEXPAND | wxBOTTOM | wxLEFT | wxRIGHT, BORDER_SPACING);
    SetSizer(top);

    // Fit the frame around the size of my controls.
    top->Fit(this);

    this->CenterOnParent();
  }

  void OnSelection(wxListEvent& event)
  {
    auto idx = p_list_view_->GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
    id_ = std::stoi(titles_[idx].id);
  }
  int GetSelectedProfileId() { return id_; }
};
