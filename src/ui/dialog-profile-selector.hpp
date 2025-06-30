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
  wxListView* p_list_view;
  NpTitleList titles;
  int id = 0;

public:
  DialogProfileIdSelector(wxWindow* parent, int current_id, const NpTitleList& titles)
    : id(current_id)
    , titles(titles)
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

    p_list_view = new wxListView(this,
                                 wxID_ANY,
                                 wxDefaultPosition,
                                 wxSize(325, 600),
                                 wxLC_REPORT | wxLC_SINGLE_SEL,
                                 wxDefaultValidator,
                                 "");

    // Create list control columns.
    p_list_view->InsertColumn(0, "Tile", wxLIST_FORMAT_LEFT, 200);
    p_list_view->InsertColumn(1, "ID", wxLIST_FORMAT_LEFT, wxLIST_AUTOSIZE);

    // Populate list.
    for (int i = 0; i < titles.size(); i++) {
      const auto& title = titles[i];
      auto idx = p_list_view->InsertItem(i, title.name);
      p_list_view->SetItem(idx, 1, title.id);
    }

    // Need to select item before selection event is bound, because this method generates a
    // selection event.
    const auto id_str = std::to_string(id);
    for (size_t i = 0; i < titles.size(); i++) {
      if (titles[i].id == id_str) {
        p_list_view->Select(i);
      }
    }

    p_list_view->Bind(wxEVT_LIST_ITEM_SELECTED, &DialogProfileIdSelector::OnSelection, this);

    wxBoxSizer* top = new wxBoxSizer(wxVERTICAL);
    top->Add(p_list_view, 1, wxEXPAND | wxALL, BORDER_SPACING);
    top->Add(buttons, 0, wxEXPAND | wxBOTTOM | wxLEFT | wxRIGHT, BORDER_SPACING);
    SetSizer(top);

    // Fit the frame around the size of my controls.
    top->Fit(this);

    this->CenterOnParent();
  }

  void OnSelection(wxListEvent& event)
  {
    auto idx = p_list_view->GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
    id = std::stoi(titles[idx].id);
  }
  int GetSelectedProfileId() { return id; }
};
