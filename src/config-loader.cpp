#include "config-loader.hpp"

#include "settings.hpp"

#include <wx/msgdlg.h>
#include <wx/string.h>

#include <string>

/**
 * Returns true if application should continue loading.
 * Returns false if application should exit.
 * Guard loading of configuration from a file with the user dialog, if operation
 * fails.
 */
bool
LoadSettingsFile()
{
  const std::string filename = "settings.json";
  auto result = settings::LoadFromFile(filename);
  if (result.success) {
    return true;
  } else {
    // press okay to keep empty settings.
    const wxString ok = "Overwrite with Default Settings";
    const wxString cancel = "Exit Program";
    const wxString prefix = "Error opening settings file.\n\n";

    auto dlg =
      wxMessageDialog(nullptr,
                      "Error opening settings file.\n\n" + result.err_msg,
                      "TrackIRMouse Error",
                      wxICON_ERROR | wxOK | wxCANCEL);
    dlg.SetOKCancelLabels(ok, cancel);

    // display reason for error to user
    // give user the chance to quit application (preventing possible data
    // loss and manually fixing the error) or load default/empty config
    if (dlg.ShowModal() == wxID_OK) {
      return true;
    } else {
      spdlog::warn("user closed app when presented with invalid settings load");
      return false;
    }
  }
}
