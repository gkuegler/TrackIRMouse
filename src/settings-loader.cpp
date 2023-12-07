/**
 * Main app entry and GUI components.
 *
 * --License Boilerplate Placeholder--
 *
 */

#include "settings-loader.hpp"

#include <wx/msgdlg.h>
#include <wx/string.h>

#include <string>

#include "constants.hpp"
#include "log.hpp"
#include "settings.hpp"
#include "utility.hpp"

/**
 * Returns true if application should continue loading.
 * Returns false if application should exit.
 * Guard loading of configuration from a file with the user dialog, if operation
 * fails.
 */
bool
LoadSettingsFile()
{
  try {

    settings::LoadFromFile(
      utility::GetAbsolutePathBasedFromExeFolder(SETTINGS_FILE_NAME));
  } catch (std::exception& ex) {
    settings::SetToDefaults();

    // press okay to keep empty settings.
    const wxString ok = "Overwrite with Default Settings";
    const wxString cancel = "Exit Program";

    auto dlg = wxMessageDialog(nullptr,
                               ex.what(),
                               "TrackIRMouse - Error opening settings file.",
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
