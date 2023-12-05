/**
 * Interface to start tracking with TrackIR.
 *
 * The release version is designed to run as an administrator
 * UAC Execution Level /level=requireAdministrator
 * The debug version leaves UAC alone and runs as a user.
 *
 * I'm building to x64 primarily because fmt library
 * only 64-bit.
 */

#pragma warning(disable : 4996)
#include "trackers.hpp"

#include "log.hpp"
#include "np-client.h"
#include "registry-access.h"
#include "types.hpp"

// 7 is a magic number I found through experimentation.
// It means a hWnd is already registered with NPTrackIR.
// Calling un-register window handle will unregistered the
// window handle regardless who the hWnd belongs to.
#define NP_ERR_HWND_ALREADY_REGISTERED 7

// Uncomment this line for testing to prevent program
// from attaching to NPTrackIR and supersede control
// #define TEST_NO_TRACK
//
// TODO: make track client polymorphic class by providing a base class?
// TODO: check out opentrack for other interfaces?

namespace trackers {

// SendInput with absolute mouse movement flag takes a short int
constexpr auto USHORT_MAX_VAL = 65535;

TrackIR::TrackIR(handlers::MouseHandler* handler)
  : handler_(handler)
  , tracking_allowed_to_run_(true)
  , pause_mouse_(false)
{
  logger_ = mylogging::GetClonedLogger("trackir");
}

void
TrackIR::initialize(HWND hWnd,
                    bool auto_find_dll,
                    std::string user_dll_folder,
                    int profile_id)

{

  logger_->trace("Starting Initialization Of TrackIR");

  //////////////////////////////////////////////////////////////////////
  //                  Finding NPTrackIR DLL Location                  //
  //////////////////////////////////////////////////////////////////////

  std::string dll_path =
    auto_find_dll ? GetTrackIRDllFolderFromRegistry() : user_dll_folder;

  // ensure path has a slash at the end before appending filename
  if (dll_path.back() != '\\') {
    dll_path.push_back('\\');
  }

// Match to the correct bitness of this application
#if defined(_WIN64) || defined(__amd64__)
  dll_path.append("NPClient64.dll");
#else
  dll_path.append("NPClient.dll");
#endif

  spdlog::debug("NPTrackIR DLL Path: {}", dll_path);

  // Find and load TrackIR DLL
#ifdef UNICODE
  TCHAR sDll[MAX_PATH];

  int conversion_result =
    MultiByteToWideChar(CP_UTF8,
                        // MB_ERR_INVALID_CHARS, // I feel like this should be
                        // the smart choice, but this causes an error.
                        MB_COMPOSITE,
                        dll_path.c_str(),
                        MAX_PATH,
                        sDll,
                        MAX_PATH);

  if (0 == conversion_result) {
    throw std::runtime_error(
      std::format("Windows Error: failed to convert track dll location to "
                  "wchar_t* with error code: {}",
                  GetLastError()));
  }
#else
  TCHAR sDLL = dll_path.c_str()
#endif

  // Load the DLL and resolved dll function pointers
  NP_InitializeClient(sDll);

  // NP software needs a window handle_ to know when it should
  // stop sending data frames if window is closed.
  // TEST_NO_TRACK defined so that this program doesn't boot control of NP
  // software from my local MouseTrackIR instance while developing.
#ifndef TEST_NO_TRACK

  switch (NP_RegisterWindowHandle(hWnd)) {
    case NP_OK:
      break;
    case NP_ERR_DEVICE_NOT_PRESENT:
      throw std::runtime_error("Device Not Present.");
      break;
    case NP_ERR_HWND_ALREADY_REGISTERED:
      NP_UnregisterWindowHandle();
      spdlog::warn("NP Booting control of previous mousetracker instance.");
      Sleep(2);
      if (NP_OK != NP_RegisterWindowHandle(hWnd)) {
        throw std::runtime_error("Failed to re-register window handle.");
      }
      break;
    default:
      throw std::runtime_error("Failed to register window handle.");
      break;
  }

  // I'm skipping 'query the software version'. I don't think its necessary
  // there's no info in the sdk on how to handle different software versions of
  // NPTrackIR.

  // Request roll, pitch. See NPClient.h
  if (NP_OK != NP_RequestData(NPPitch | NPYaw | NPX | NPY | NPZ)) {
    throw std::runtime_error("NP Request Data failed.");
  }
  if (NP_OK != NP_RegisterProgramProfileID(profile_id)) {
    throw std::runtime_error("NP Register Profile ID failed.");
  }
#endif
  spdlog::info("NPTrackIR Initialization Successful");
}

TrackIR::~TrackIR()
{
  disconnect_from_np_trackir();
}

retcode
TrackIR::start()
{
#ifndef TEST_NO_TRACK
  // Skipping this api call. I think this is for legacy games.
  // NP_StopCursor();

  if (NP_OK == NP_StartDataTransmission())
    logger_->debug("NP Started data transmission.");
  else {
    logger_->error("NP Start Data Transmission failed");
    return retcode::fail;
  }

#endif

  NPRESULT gdf;
  tagTrackIRData *pTIRData, TIRData;
  pTIRData = &TIRData;

  unsigned short last_frame = 0;
  // note: dropped frames rare and not real world relevant
  // I decided not to handle_ them.
  while (tracking_allowed_to_run_) {
    gdf = NP_GetData(pTIRData);
    if (NP_OK == gdf) {
      // unsigned short status = (*pTIRData).wNPStatus;
      unsigned short framesig = (*pTIRData).wPFrameSignature;
      // TODO: apply negative sign on startup to avoid extra operation here
      // yaw and pitch come reversed relative to GUI program for some reason
      // from trackIR

      // head down is pitch increasing
      // head left is yaw increaseing
      double yaw = (*pTIRData).fNPYaw * (180.0 / 16383);
      double pitch = (*pTIRData).fNPPitch * (180.0 / 16383);

      // Don't move the mouse when TrackIR paused.
      if (framesig == last_frame) {
        // TrackIR 5 supposedly operates at 120hz.
        // 8ms is approximately 1 frame.
        Sleep(8);
        continue;
      }

      // Pause mouse move calls without stopping NPTrackIR data transmission or
      // disconnecting. This is  primarily used during testing/debugging so that
      // my test instance can latch on to active tracking data without
      // re-registering a window handle and disrupting my normal copy. The pause
      // tracking method is called from the named pipe server. I use build
      // scripts to send commands to my working copy's pipe server to pause
      // mouse moving before and enabling mouse moving after.
      if (pause_mouse_ == false) {
        handler_->handle_input(yaw, pitch);
      }

      last_frame = framesig;
    } else if (NP_ERR_DEVICE_NOT_PRESENT == gdf) {
      logger_->warn("device not present, tracking stopped");
      return retcode::track_ir_loss;
    } else {
      throw std::runtime_error(
        std::format("tracking loop exit due to unknown problem.\ncode returned "
                    "from NP dll: {}",
                    static_cast<int>(gdf)));
    }
    Sleep(8);
  }

  return retcode::graceful_exit;
}
// Tracking loop uses this to check if it should break, return to thread, then
// have thread auto clean up
void
TrackIR::connect_to_np_track_ir()
{
}

void
TrackIR::disconnect_from_np_trackir()
{
#ifndef TEST_NO_TRACK
  NP_StopDataTransmission();
  NP_UnregisterWindowHandle();
#endif
}

void
TrackIR::toggle_mouse()
{
  pause_mouse_ = !pause_mouse_;
  if (pause_mouse_) {
    logger_->warn("mouse paused");
  } else {
    logger_->warn("mouse resumed");
  }
}

void
TrackIR::stop()
{
  logger_->trace("stop tracking called into");
  tracking_allowed_to_run_ = false;
}

} // namespace trackers
