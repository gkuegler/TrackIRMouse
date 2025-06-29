#include "ui/graphic.hpp"

#include "windows-wrapper.hpp"
#include <wx/wx.h>

#include <algorithm>
#include <vector>

#include "environment.hpp"
#include "settings.hpp"
#include "types.hpp"

PanelDisplayGraphic::PanelDisplayGraphic(wxWindow* parent, wxSize size, Settings& s)
  : settings(s)
  , wxPanel(parent, wxID_ANY, wxDefaultPosition, size, wxFULL_REPAINT_ON_RESIZE, "")
{
  p_parent_ = parent;
  Bind(wxEVT_PAINT, &PanelDisplayGraphic::PaintEvent, this);
}

// Called by the system of by wxWidgets when the panel_ needs
// to be redrawn. You can also trigger this call by
// calling Refresh()/Update().
void
PanelDisplayGraphic::PaintEvent(wxPaintEvent& evt)
{
  wxPaintDC dc(this);
  Render(dc);
}

/*
 * Alternatively, you can use a clientDC to paint on the panel_
 * at any time. Using this generally does not free you from
 * catching paint events, since it is possible that e.g. the window
 * manager throws away your drawing when the window comes to the
 * background, and expects you will redraw it when the window comes
 * back (by sending a paint event).
 *
 * In most cases, this will not be needed at all; simply handling
 * paint events and calling Refresh() when a refresh is needed
 * will do the job.
 */
// TODO: call paint now on settings change
void
PanelDisplayGraphic::PaintNow()
{
  wxClientDC dc(this);
  Render(dc);
}

/*
 * Here we do the actual rendering. I put it in a separate
 * method so that it can work no matter what type of DC
 * (e.g. wxPaintDC or wxClientDC) is user.
 */

void
PanelDisplayGraphic::Render(wxDC& dc)
{
  // set canvas space or querry space?
  // get monitor rectangles
  // normalize sizes?
  // find center of virt desktop?
  // draw rectangle
  // find correct bound in draw Pixels
  // multiply by multiplier or normalize to std size?
  // draw text inside each rectangle

  // get rect
  // normalize rect to height_
  // calc imaginary top left and top right of norm
  // get fitting coefficient
  // offset rec from imaginary top left
  // fit panel_ to imaginary and fitted height_ and width_
  // draw rectangles to panel_
  // draw text to panel_
  // make sure panel_ in centered?

  dc.DestroyClippingRegion();

  static constexpr int pad = 5;
  static const auto dark_blue = wxColor(71, 127, 255);
  static const auto light_blue = wxColor(173, 198, 255);
  static const auto dark_gray = wxColor(80, 80, 80);
  static const auto light_gray = wxColor(200, 200, 200);
  const auto text_heigt = dc.GetTextExtent("example").GetHeight();
  const auto half_text_height = text_heigt / 2;

  int cwidth = 0;
  int cheight = 0;
  this->GetClientSize(&cwidth, &cheight);
  // spdlog::info("height_, width_ -> {}, {}", cheight, cwidth);

  const double area_x = cwidth - 50; // drawing area width_
  const double area_y = cheight - 50;

  // get array of monitor bounds
  const auto hdi = WinDisplayInfo();
  const auto usrDisplays = settings.GetActiveProfileRef().displays;

  // offset all rectangles so that 0,0 as top left most value
  std::vector<RectPixels> bounds_offset;
  {
    int l{ 0 }, t{ 0 };
    for (auto& d : hdi.rectangles) {
      l = (d.left < l) ? d.left : l; // get leftmost value
      t = (d.top < t) ? d.top : t;   // get topmost value
    }
    for (auto& d : hdi.rectangles) {
      // int x = (dwidth / 2) + l;
      // int y = (dheight / 2) + t;
      bounds_offset.push_back({ d.top - l, d.right - l, d.top - t, d.bottom - t });
    }
  }

  // scale rectangle so they fit in the drawing area, taking up all space
  // available
  std::vector<Rect<double>> bounds_norm;
  {
    const double xratio = area_x / hdi.desktop_width;
    const double yratio = area_y / hdi.desktop_height;
    const double ratio = std::min<double>(xratio, yratio);
    for (auto& d : bounds_offset) {
      bounds_norm.push_back({ static_cast<double>(d.left) * ratio,
                              static_cast<double>(d.right) * ratio,
                              static_cast<double>(d.top) * ratio,
                              static_cast<double>(d.bottom) * ratio });
    }
  }

  // for (auto& d : bounds_norm) {
  //   spdlog::info("scaled to dwg area -> {}, {}, {}, {}", d.left, d.right, d.top,
  //                d.bottom);
  // }

  // reget max height_ and width_
  double swidth = 0;
  double sheight = 0;
  {
    double l{ 0 }, r{ 0 }, t{ 0 }, b{ 0 };
    for (auto& d : bounds_norm) {
      // TODO: use std::min here
      l = (d.left < l) ? d.left : l;
      r = (d.right > r) ? d.right : r;
      t = (d.top < t) ? d.top : t;
      b = (d.bottom > b) ? d.bottom : b;
    }
    swidth = r - l;
    sheight = b - t;
  }
  const double x_offset = (area_x / 2) - (swidth / 2);
  const double y_offset = (area_y / 2) - (sheight / 2);

  {
    for (auto& d : bounds_norm) {
      d.left += x_offset;
      d.right += x_offset;
      d.top += y_offset;
      d.bottom += y_offset;
    }
  }

  // for (auto& d : bounds_norm) {
  //   spdlog::info("centered to dwg area -> {}, {}, {}, {}", d.left, d.right, d.top,
  //                d.7);
  // }

  for (int i = 0; i < bounds_norm.size(); i++) {
    // Draw the rectangle
    auto r = wxRect();
    r.SetLeft(bounds_norm[i].left);
    r.SetRight(bounds_norm[i].right);
    r.SetTop(bounds_norm[i].top);
    r.SetBottom(bounds_norm[i].bottom);
    dc.SetBrush(light_gray);        // fill color
    dc.SetPen(wxPen(dark_gray, 3)); // outline
    dc.DrawRectangle(r);

    const bool userSpecifiedRotationAvailable = (usrDisplays.size() > i);

    // Draw text labels
    const auto text_left = userSpecifiedRotationAvailable
                             ? wxString::Format(wxT("%0.2f"), usrDisplays[i].rotation.left)
                             : wxString("?");
    const auto text_right = userSpecifiedRotationAvailable
                              ? wxString::Format(wxT("%0.2f"), usrDisplays[i].rotation.right)
                              : wxString("?");
    const auto text_top = userSpecifiedRotationAvailable
                            ? wxString::Format(wxT("%0.2f"), usrDisplays[i].rotation.top)
                            : wxString("?");
    const auto text_bottom = userSpecifiedRotationAvailable
                               ? wxString::Format(wxT("%0.2f"), usrDisplays[i].rotation.bottom)
                               : wxString("?");

    // clang-format off
    const auto middleX = r.x + (r.GetWidth() / 2);
    const auto middleY = r.y + (r.height / 2);

    const int text_left_x = r.x + pad;
    const int text_left_y = middleY - half_text_height;

    const int text_right_x = r.GetRight() - pad - dc.GetTextExtent(text_right).GetWidth();
    const int text_right_y = middleY - half_text_height;

    const int text_top_x = middleX - (dc.GetTextExtent(text_top).GetWidth() / 2);
    const int text_top_y = r.y + pad;

    const int text_bottom_x = middleX - (dc.GetTextExtent(text_bottom).GetWidth() / 2);
    const int text_bottom_y = r.GetBottom() - pad - text_heigt;

    const auto text_center = wxString::Format(wxT("%d"), i);
    const int text_center_x = middleX - (dc.GetTextExtent(text_center).GetWidth() / 2);
    const int text_center_y = middleY - text_heigt - 1;

    const auto resolution = wxString::Format(wxT("%dx%d"), 1920, 1080);
    const int resolution_x = middleX - (dc.GetTextExtent(resolution).GetWidth() / 2);
    const int resolution_y = middleY + 1;
    

    dc.DrawText(text_left, text_left_x, text_left_y);
    dc.DrawText(text_right, text_right_x, text_right_y);
    dc.DrawText(text_top, text_top_x, text_top_y);
    dc.DrawText(text_bottom, text_bottom_x, text_bottom_y);
    dc.DrawText(text_center, text_center_x, text_center_y);
    dc.DrawText(resolution, resolution_x, resolution_y);
    // clang-format on
  }
}
