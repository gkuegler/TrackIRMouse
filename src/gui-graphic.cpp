#include "gui-graphic.hpp"

#include <Windows.h>
#include <spdlog/spdlog.h>
#include <wx/wx.h>

#include <algorithm>
#include <vector>

#include "config.hpp"
#include "environment.hpp"

cDisplayGraphic::cDisplayGraphic(wxWindow* parent, wxSize size)
  : wxPanel(parent,
            wxID_ANY,
            wxDefaultPosition,
            size,
            wxFULL_REPAINT_ON_RESIZE,
            "")
{
  p_parent_ = parent;
  // TODO: initialize logger_ here
  Bind(wxEVT_PAINT, &cDisplayGraphic::PaintEvent, this);
}

// Called by the system of by wxWidgets when the panel_ needs
// to be redrawn. You can also trigger this call by
// calling Refresh()/Update().
void
cDisplayGraphic::PaintEvent(wxPaintEvent& evt)
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
cDisplayGraphic::PaintNow()
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
cDisplayGraphic::Render(wxDC& dc)
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

  // test data
  std::vector<int> padding = { 3, 3, 0, 0 };

  int cwidth = 0;
  int cheight = 0;
  this->GetClientSize(&cwidth, &cheight);
  // spdlog::info("height_, width_ -> {}, {}", cheight, cwidth);

  const double area_x = cwidth - 50; // drawing area width_
  const double area_y = cheight - 50;

  // get array of monitor bounds
  const auto hdi = env::GetHardwareDisplayInfo();
  const auto usrDisplays = config::Get()->GetActiveProfile().displays;

  // normalize virtual desktop rect for each monitor where 1 = total width_ in
  // Pixels values can be negative
  // auto normalized = NormalizeRect(bounds);
  const int dwidth = hdi.width;
  const int dheight = hdi.height;
  // spdlog::info("dwidth -> {}", dwidth);
  // spdlog::info("dheight -> {}", dheight);

  // offset all rectangles so that 0,0 as top left most value
  env::HardwareDisplays bounds_offset;
  {
    int l{ 0 }, t{ 0 };
    for (auto& d : hdi.displays) {
      l = (d[0] < l) ? d[0] : l; // get leftmost value
      t = (d[2] < t) ? d[2] : t; // get topmost value
    }
    for (auto& d : hdi.displays) {
      // int x = (dwidth / 2) + l;
      // int y = (dheight / 2) + t;
      bounds_offset.push_back({ d[0] - l, d[1] - l, d[2] - t, d[3] - t });
    }
  }

  // for (auto& d : bounds_offset) {
  //   spdlog::info("offset -> {}, {}, {}, {}", d[0], d[1], d[2], d[3]);
  // }

  // scale rectangle so they fit in the drawing area, taking up all space
  // available
  std::vector<std::vector<double>> bounds_norm;
  {
    const double xratio = area_x / dwidth;
    const double yratio = area_y / dheight;
    const double ratio = std::min<double>(xratio, yratio);
    for (auto& d : bounds_offset) {
      bounds_norm.push_back({ static_cast<double>(d[0]) * ratio,
                              static_cast<double>(d[1]) * ratio,
                              static_cast<double>(d[2]) * ratio,
                              static_cast<double>(d[3]) * ratio });
    }
  }

  // for (auto& d : bounds_norm) {
  //   spdlog::info("scaled to dwg area -> {}, {}, {}, {}", d[0], d[1], d[2],
  //                d[3]);
  // }

  // reget max height_ and width_
  double swidth = 0;
  double sheight = 0;
  {
    double l{ 0 }, r{ 0 }, t{ 0 }, b{ 0 };
    for (auto& d : bounds_norm) {
      l = (d[0] < l) ? d[0] : l;
      r = (d[1] > r) ? d[1] : r;
      t = (d[2] < t) ? d[2] : t;
      b = (d[3] > b) ? d[3] : b;
    }
    swidth = r - l;
    sheight = b - t;
  }
  const double x_offset = (area_x / 2) - (swidth / 2);
  const double y_offset = (area_y / 2) - (sheight / 2);

  {
    for (auto& d : bounds_norm) {
      d[0] += x_offset;
      d[1] += x_offset;
      d[2] += y_offset;
      d[3] += y_offset;
    }
  }

  // for (auto& d : bounds_norm) {
  //   spdlog::info("centered to dwg area -> {}, {}, {}, {}", d[0], d[1], d[2],
  //                d[3]);
  // }

  for (int i = 0; i < bounds_norm.size(); i++) {
    // Draw the rectangle
    auto r = wxRect();
    r.SetLeft(bounds_norm[i][0]);
    r.SetRight(bounds_norm[i][1]);
    r.SetTop(bounds_norm[i][2]);
    r.SetBottom(bounds_norm[i][3]);
    dc.SetBrush(light_gray);        // fill color
    dc.SetPen(wxPen(dark_gray, 3)); // outline
    dc.DrawRectangle(r);

    const bool userSpecifiedRotationAvailable = (usrDisplays.size() > i);

    // Draw text labels
    const auto text_left =
      userSpecifiedRotationAvailable
        ? wxString::Format(wxT("%0.2f"), usrDisplays[i].rotation[0])
        : wxString("?");
    const auto text_right =
      userSpecifiedRotationAvailable
        ? wxString::Format(wxT("%0.2f"), usrDisplays[i].rotation[1])
        : wxString("?");
    const auto text_top =
      userSpecifiedRotationAvailable
        ? wxString::Format(wxT("%0.2f"), usrDisplays[i].rotation[2])
        : wxString("?");
    const auto text_bottom =
      userSpecifiedRotationAvailable
        ? wxString::Format(wxT("%0.2f"), usrDisplays[i].rotation[3])
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
