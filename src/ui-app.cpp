/**
 * Main app entry and GUI components.
 *
 * --License Boilerplate Placeholder--
 *
 * TODO section:
 * transform mapping values for head distance
 * use default padding overwrites user padding values
 * allow user to select custom settings file
 * upon not finding settings on load; in blank profile add number of displays
 * that are the same number of monitors detected
 * implement crtl-s save feature
 * remove generate example settings file action
 * generator an icon
 *
 *
 * profile box:
 *   set size limitations for text for inputs
 *   pick number of displays
 *   configuration window?
 *   duplicate profile
 *
 *   maintain selections to move up and down
 *   convert values to doubles in validation step of handle_ event
 *   fix internal override of handling default display padding
 */

// TODO: change the variable styling
// TODO: change the method styleing?
// TODO: change the function styling?
// TODO: add documentation?
// TODO: prune includes

// bug list
//  TODO: empty profile created by default
//  TODO: example monitors don't automatically update with the rest of the GUI
//  TODO: remove duplicate windows hardware methods

#include <wx/wx.h>

#include <string>

#include "config-loader.hpp"
#include "log.hpp"
#include "messages.hpp"
#include "mouse-modes.hpp"
#include "threads.hpp"
#include "types.hpp"
#include "ui-frame.hpp"
#include "utility.hpp"

/**
 * Center the app on the main display.
 */
wxPoint
GetOrigin(const int w, const int h)
{
  const int desktop_w = GetSystemMetrics(SM_CXMAXIMIZED);
  const int desktop_h = GetSystemMetrics(SM_CYMAXIMIZED);
  return wxPoint((desktop_w / 2) - (w / 2), (desktop_h / 2) - (h / 2));
}

class App : public wxApp
{
public:
  App(){};
  ~App(){};

  virtual bool OnInit();
  virtual int OnExit();
  virtual void OnUnhandledException();

private:
  Frame* main_window_ = nullptr;
  wxString top_app_name_;
};

wxIMPLEMENT_APP(App);

void
App::OnUnhandledException()
{
  wxLogFatalError("An unhandled exception has occurred. "
                  "Application will now terminate.");
  std::terminate();
}

bool
App::OnInit()
{
  // Initialize global default loggers
  mylogging::SetUpLogging();

  // App initialization constants
  constexpr int app_width = 1200;
  constexpr int app_height = 900;

  // Construct child elements first. The main panel_ contains a text control
  // that is a log target.
  main_window_ =
    new Frame(GetOrigin(app_width, app_height), wxSize(app_width, app_height));
  main_window_->Show();
  main_window_->StartScrollAlternateHooksAndHotkeys();

  //////////////////////////////////////////////////////////////////////
  //               Messages From Threads Outside Of GUI               //
  //////////////////////////////////////////////////////////////////////

  Bind(wxEVT_THREAD, [this](wxThreadEvent& event) {
    LogWindow* textrich = main_window_->p_text_rich_;
    switch (static_cast<msgcode>(event.GetInt())) {

      // exposes gui dependent logging from outside threads.
      // Log messages (lvl: error & critical) launch an error message dialog.
      // log messages (lvl: warning) appear as written text in the output
      // window. log messages (lvl: <=info) appear as written text in the
      // output window.
      case msgcode::log: {
        const auto& level = event.GetExtraLong();
        const auto& msg = event.GetString();
        if (spdlog::level::critical == level) {
          wxLogFatalError(msg);
        } else if (spdlog::level::err == level) {
          wxLogError(msg);
        } else if (spdlog::level::warn == level) {
          const auto existing_style = textrich->GetDefaultStyle();
          textrich->SetDefaultStyle(wxTextAttr(*wxRED));
          textrich->AppendText(msg);
          textrich->SetDefaultStyle(existing_style);
        } else {
          textrich->AppendText(msg);
        }
      } break;
      case msgcode::toggle_tracking: {
        if (main_window_->track_thread_) {
          main_window_->track_thread_->tracker_->toggle_mouse();
        }
      } break;

      // retrieve alternate mouse mode by application.
      // currently locks the mouse the different scrollbar x positions by
      // application.
      // this message is generated (using a global hook procedure) every
      // time a new window takes focus.
      case msgcode::notify_app: {
        top_app_name_ = wxString(event.GetString());
        if (main_window_->track_thread_) {
          main_window_->track_thread_->tracker_->handler_->set_alternate_mode(
            GetModeByExecutableName(top_app_name_));
        }
      } break;

      // set the alternate mouse mode for the current focused application.
      // this message is currently sent by a pipe server in a local thread.
      case msgcode::set_mode: {
        auto mode = static_cast<mouse_mode>(event.GetExtraLong());
        UpdateModesbyExecutableName(top_app_name_, mode);
        if (main_window_->track_thread_) {
          main_window_->track_thread_->tracker_->handler_->set_alternate_mode(
            mode);
        }
      } break;

      // close the application gracefully
      case msgcode::close_app:
        main_window_->Close(true);
        break;
      default:
        break;
    }
  });

  // Presents a dialog to the user if loading from file fails.
  // Initialization function returns false if the user presses cancel.
  // This should be called before accessing any settings.
  if (!LoadSettingsFile()) {
    return false;
  }

  main_window_->UpdateGuiFromSettings();

  // Start the track IR thread if enabled
  const auto settings = settings::Get();

  if (settings->track_on_start) {
    wxCommandEvent event = {}; // blank event to reuse start handler code
    main_window_->OnStart(event);
  }

  // Start the pipe server thread.
  // Pipe server is only started at first application startup.
  if (settings->pipe_server_enabled) {
    main_window_->p_server_thread_ =
      new ControlServerThread(main_window_, settings->pipe_server_name);
    if (main_window_->p_server_thread_->Run() != wxTHREAD_NO_ERROR) {
      spdlog::error("Can't run server thread.");
      delete main_window_->p_server_thread_;
      main_window_->p_server_thread_ = nullptr;
    }
  }

  return true;
}

int
App::OnExit()
{
  return 0;
}
