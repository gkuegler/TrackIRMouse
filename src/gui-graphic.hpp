#ifndef TRACKIRMOUSE_DISPLAYGRAPHIC_H
#define TRACKIRMOUSE_DISPLAYGRAPHIC_H

#include <Windows.h>
#include <wx/wx.h>

#include <vector>
class cDisplayGraphic : public wxPanel {
 public:
  cDisplayGraphic(wxWindow* parent, wxSize size);
  wxWindow* m_parent;

  void PaintEvent(wxPaintEvent& evt);
  void PaintNow();  // user method to force redraw
  void Render(wxDC& dc);

 private:
  int width = 200;
  int height = 100;
  int gap = 20;
};

#endif /* TRACKIRMOUSE_DISPLAYGRAPHIC_H */
