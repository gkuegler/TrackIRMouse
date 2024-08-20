#pragma once
#ifndef TIRMOUSE_DISPLAYGRAPHIC_H
#define TIRMOUSE_DISPLAYGRAPHIC_H

#include <wx/wx.h>

class PanelDisplayGraphic : public wxPanel
{
public:
  PanelDisplayGraphic(wxWindow* parent, wxSize size);
  wxWindow* p_parent_;

  void PaintEvent(wxPaintEvent& evt);
  void PaintNow(); // user method to force redraw
  void Render(wxDC& dc);

private:
  int width_ = 200;
  int height_ = 100;
  int gap_ = 20;
};

#endif /* TIRMOUSE_DISPLAYGRAPHIC_H */
