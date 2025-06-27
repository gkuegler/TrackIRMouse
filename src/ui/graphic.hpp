#pragma once

#include "settings.hpp"
#include <wx/wx.h>

class PanelDisplayGraphic : public wxPanel
{
public:
  PanelDisplayGraphic(wxWindow* parent, wxSize size, Settings& s);
  wxWindow* p_parent_;

  void PaintEvent(wxPaintEvent& evt);
  void PaintNow(); // user method to force redraw
  void Render(wxDC& dc);

private:
  Settings& settings;
  int width_ = 200;
  int height_ = 100;
  int gap_ = 20;
};
