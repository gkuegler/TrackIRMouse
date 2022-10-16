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
#include "types.hpp"

// Uncomment this line for testing to prevent program
// from attaching to NPTrackIR and supersede control
// #define TEST_NO_TRACK
//
// TODO: make track client polymorphic class
// TODO: check out opentrack for other interfaces

namespace trackers {

// SendInput with absolute mouse movement flag takes a short int
constexpr auto USHORT_MAX_VAL = 65535;

TrackIR::TrackIR(HWND hWnd, std::string dllpath, int profile_id,
                 handlers::MouseHandler* handler)
    : m_hWnd(hWnd),
      m_dllpath(dllpath),
      m_profile_id(profile_id),
      m_handler(handler),
      m_tracking_allowed_to_run(true),
      m_pause_tracking(false) {
  m_logger = mylogging::MakeLoggerFromStd("trackir");

  m_logger->trace("Starting Initialization Of TrackIR");

  connect_to_np_track_ir();
}

TrackIR::~TrackIR() { disconnect_from_np_trackir(); }

retcode TrackIR::start() {
#ifndef TEST_NO_TRACK
  // Skipping this api call. I think this is for legacy games.
  // NP_StopCursor();

  NPRESULT rslt = NP_StartDataTransmission();
  if (NP_OK == rslt)
    m_logger->debug("NP Started data transmission.");
  else {
    m_logger->error("NP Start Data Transmission failed with code: {}\n", rslt);
    return retcode::fail;
  }

  spdlog::info("NPTrackIR Started");

#endif

  NPRESULT gdf;
  tagTrackIRData *pTIRData, TIRData;
  pTIRData = &TIRData;

  unsigned short last_frame = 0;
  // note: dropped frames rare and not real world relevant
  // I decided not to handle them.
  while (m_tracking_allowed_to_run) {
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

      // Watchdog enables software to be controlled via a named pipe. This is
      // primarily used during testing so that my test instance can latch on
      // to active tracking data without re-registering a window handle. A
      // disbale msg is sent before my test instance launches, then my normal
      // instance is enables after as part of my build script.
      if (m_pause_tracking == false) {
        m_handler->handle_input(yaw, pitch);
      }

      last_frame = framesig;
    }

    else if (NP_ERR_DEVICE_NOT_PRESENT == gdf) {
      m_logger->warn("device not present, tracking stopped");
      return retcode::fail;
    }

    Sleep(8);
  }

  return retcode::success;
}
// Tracking loop uses this to check if it should break, return to thread, then
// have thread auto clean up
void TrackIR::connect_to_np_track_ir() {
  // start watchdog thread
  // load trackir dll and getproc addresses
  // connect to trackir and request data type
  //
  // Find and load TrackIR DLL
#ifdef UNICODE
  TCHAR sDll[MAX_PATH];

  int conversion_result = MultiByteToWideChar(
      CP_UTF8,
      // MB_ERR_INVALID_CHARS, // I feel like this should be
      // the smart choice, but this causes an error.
      MB_COMPOSITE, m_dllpath.c_str(), MAX_PATH, sDll, MAX_PATH);

  if (0 == conversion_result) {
    throw std::runtime_error(std::format(
        "failed to convert track dll location to wchart* with error code: {}",
        GetLastError()));
  }
#else
  TCHAR sDLL = dllpath.c_str()
#endif

  // Load trackir dll and resolve function addresses
  if (NP_OK != NPClient_Init(sDll)) {
    throw std::runtime_error("couldn't initialize track ir");
  }

  // NP software needs a window handle to know when it should
  // stop sending data frames if window is closed.
  // TEST_NO_TRACK defined so that this program doesn't boot control of NP
  // software from my local MouseTrackIR instance while developing.
#ifndef TEST_NO_TRACK

  switch (NP_RegisterWindowHandle(m_hWnd)) {
    case NP_OK:
      break;
    case 7:
      // 7 is a magic number I found through experimentation.
      // It means another program/instance has its window handle registered
      // already.
      NP_UnregisterWindowHandle();
      spdlog::warn("NP Booting control of previous mousetracker instance.");
      Sleep(2);
      if (NP_OK != NP_RegisterWindowHandle(m_hWnd)) {
        throw std::runtime_error("Failed to re-register window handle");
      }
      break;
    default:
      throw std::runtime_error("Failed to register window handle.");
      break;
  }

  // I'm skipping query the software version, I don't think its necessary
  // theres no info in the sdk on how to handle different software versions

  // Request roll, pitch. See NPClient.h
  if (NP_OK != NP_RequestData(NPPitch | NPYaw)) {
    throw std::runtime_error("NP Request Data failed.");
  }
  if (NP_OK != NP_RegisterProgramProfileID(m_profile_id)) {
    throw std::runtime_error("NP Register Profile ID failed.");
  }
  spdlog::info("NPTrackIR Initialization Successful");
#endif
}

void TrackIR::disconnect_from_np_trackir() {
#ifndef TEST_NO_TRACK
  NP_StopDataTransmission();
  NP_UnregisterWindowHandle();
#endif
}

// TODO: there is a risk of the thread closing between the check and the set
void TrackIR::toggle() {
  m_pause_tracking = !m_pause_tracking;
  if (m_pause_tracking) {
    m_logger->warn("tracking paused");
  } else {
    m_logger->warn("tracking resumed");
  }
}

// TODO: there is a risk of the thread closing between the check and the set
void TrackIR::stop() {
  m_logger->trace("Stop called into.");
  m_tracking_allowed_to_run = false;
}

}  // namespace trackers
