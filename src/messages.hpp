#pragma once
#ifndef TIRMOUSE_MESSAGES_HPP
#define TIRMOUSE_MESSAGES_HPP

#include <string>

enum class msgcode
{
  normal,
  log,
  log_normal_text,
  log_red_text,
  toggle_tracking,
  notify_app,
  set_mode,
  close_app
};

void
SendThreadMessage(msgcode code, std::string msg = "");

void
SendThreadMessage(msgcode code, std::string msg, long);

#endif /* TIRMOUSE_MESSAGES_HPP */
