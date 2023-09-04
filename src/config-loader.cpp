#include "config-loader.hpp"

#include "config.hpp"

#include <wx/msgdlg.h>
#include <wx/string.h>

#include <string>

struct LoadResults
{
  bool success = false;
  std::string err_msg = "";
};

LoadResults
LoadFromFile(std::string filename)
{
  std::string err_msg = "lorem ipsum";
  try {
    auto config = config::Config(filename);
    // return a successfully parsed and validated config file
    config::Set(config);
    return LoadResults{ true, "" };
  } catch (const toml::syntax_error& ex) {
    err_msg = fmt::format(
      "Syntax error in toml file: \"{}\"\nSee error message below for hints "
      "on how to fix.\n{}",
      filename,
      ex.what());
  } catch (const toml::type_error& ex) {
    err_msg = fmt::format("Incorrect type when parsing toml file \"{}\".\n\n{}",
                          filename,
                          ex.what());
  } catch (const std::out_of_range& ex) {
    err_msg = fmt::format(
      "Missing data in toml file \"{}\".\n\n{}", filename, ex.what());
  } catch (const std::runtime_error& ex) {
    err_msg = fmt::format("Failed to open \"{}\"", filename);
  } catch (...) {
    err_msg = fmt::format(
      "Exception has gone unhandled loading \"{}\" and verifying values.",
      filename);
  }

  // build a default configuration object
  config::Set(config::Config());
  return LoadResults{ false, err_msg };
}

/**
 * Guard loading of configuration from a file with the user dialog, if operation
 * fails.
 */
bool
InitializeConfigurationFromFile()
{
  const std::string filename = "settings.toml";
  auto result = LoadFromFile(filename);
  if (result.success) {
    return true;
  } else {
    // press okay to keep empty settings.
    // press cancel to quit program and have user fix it manually before
    // restarting program.
    const wxString ok = "Load Empty User Settings";
    const wxString cancel = "Quit";
    const wxString instructions =
      wxString::Format("\n\nPress \"%s\" to load a default user settings "
                       "template.\nWarning: "
                       "data may be overwritten if you "
                       "continue witto quit this option and then later save.\n"
                       "Press \"%s\" to exit the program.",
                       ok,
                       cancel);
    auto dlg = wxMessageDialog(nullptr,
                               result.err_msg + instructions,
                               "Error",
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
