#include "hooks.hpp"
#include "handlers.hpp"
#include "log.hpp"
#include "messages.hpp"
#include "types.hpp"

#include <filesystem>
#include <psapi.h>

namespace other_global {
static handlers::MouseHandler* g_handler = nullptr;
}

void
SetHandler(handlers::MouseHandler* handler)
{
  other_global::g_handler = handler;
}

/**
 * Out-of-Context Hook Function
 * https://learn.microsoft.com/en-us/windows/win32/winauto/out-of-context-hook-functions?redirectedfrom=MSDN
 * Note: Out-of-context hook functions are noticeably slower than in-context
 * hook functions due to marshaling.
 */
void CALLBACK
WindowChangeHook(HWINEVENTHOOK hWinEventHook,
                 DWORD event,
                 HWND hwnd,
                 LONG idObject,
                 LONG idChild,
                 DWORD dwEventThread,
                 DWORD dwmsEventTime)
{
  // retreive and use the title of hwnd as needed...
  // SPDLOG_LEVEL_TRACE("hwnd: {}", static_cast<void*>(hwnd));
  CHAR buffer[MAX_PATH] = { 0 };
  DWORD dwProcId = 0;

  GetWindowThreadProcessId(hwnd, &dwProcId);

  HANDLE hProcess =
    OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, dwProcId);
  if (NULL != hProcess) {
    if (0 != GetModuleFileNameExA(hProcess, NULL, buffer, MAX_PATH)) {
      // SPDLOG_LEVEL_TRACE("process filename: {}", buffer);

      auto path = std::filesystem::path(buffer);
      auto name = path.filename();
      SendThreadMessage(msgcode::notify_app, name.string());

    } else {
      spdlog::debug("GetModuleFileNameXxA failed. GLE={}", GetLastError());
    }
    CloseHandle(hProcess);
  } else {
    spdlog::debug("failed to open process. GLE={}", GetLastError());
  }
}
// enum mouse_mode
// ------------------
// move_mouse
// scrollbar_left_small
// scrollbar_left_mini_map
// scrollbar_right_small
// scrollbar_right_mini_map
// scrollbar_hold_x
// autocad_zoom

void CALLBACK
ScrollHook(HWINEVENTHOOK hWinEventHook,
           DWORD event,
           HWND hwnd,
           LONG idObject,
           LONG idChild,
           DWORD dwEventThread,
           DWORD dwmsEventTime)
{
  // not registering scrolling start events
  if (EVENT_SYSTEM_SCROLLINGSTART == event) {
    spdlog::debug("scroll start called: {} -> {}", event, idObject);
  } else if (EVENT_SYSTEM_SCROLLINGEND == event) {
    spdlog::debug("scroll end called: {} -> {}", event, idObject);
  }
  // no applications use this default
  // scrollbar ids are different for every application
  // if (OBJID_VSCROLL == idObject)
}

WindowChangedHook::WindowChangedHook()
{
  h_hook_window_change =
    SetWinEventHook(EVENT_OBJECT_FOCUS,
                    EVENT_OBJECT_FOCUS,
                    NULL,
                    &WindowChangeHook,
                    0,
                    0,
                    WINEVENT_OUTOFCONTEXT | WINEVENT_SKIPOWNPROCESS);
  // h_scroll = SetWinEventHook(EVENT_SYSTEM_SCROLLINGSTART,
  //                            EVENT_SYSTEM_SCROLLINGEND,
  //                            NULL,
  //                            &ScrollHook,
  //                            0,
  //                            0,
  //                            WINEVENT_OUTOFCONTEXT |
  //                            WINEVENT_SKIPOWNPROCESS);
}

WindowChangedHook::~WindowChangedHook()
{
  UnhookWinEvent(h_hook_window_change);
  // UnhookWinEvent(h_scroll);
}
