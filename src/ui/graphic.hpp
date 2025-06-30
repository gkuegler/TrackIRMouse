#pragma once

#include "settings.hpp"
#include <wx/wx.h>

/* A graphic showing the current arrangement of display monitors and associated info.*/
class PanelDisplayGraphic : public wxPanel
{
public:
  wxWindow* p_parent;

  PanelDisplayGraphic(wxWindow* parent, Settings& s);
  void OnPaint(wxPaintEvent& evt);

private:
  Settings& settings;

  void Render(wxDC& dc);
};
