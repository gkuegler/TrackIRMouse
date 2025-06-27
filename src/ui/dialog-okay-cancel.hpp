#pragma once

#include <wx/msgdlg.h>
#include <wx/string.h>

/**
 * Launch a modal dialog for user input.
 * Custom text for the 'okay' and 'cancel' buttons.
 * Returns true if "Okay" was pressed, otherwise returns false.
 */
bool
ModalDialogOkayOrCancel(const char* msg,
                        const char* title,
                        const char* text_ok = "Okay",
                        const char* text_cancel = "Cancel")
{

  auto dlg =
    wxMessageDialog(nullptr, msg, title, wxICON_ERROR | wxOK | wxCANCEL);

  dlg.SetOKCancelLabels(text_ok, text_cancel);

  return (dlg.ShowModal() == wxID_OK);
}
