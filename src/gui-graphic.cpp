#include "gui-graphic.hpp"

#include <Windows.h>
#include <spdlog/spdlog.h>
#include <wx/wx.h>

#include <algorithm>
#include <vector>

#include "config.hpp"
#include "environment.hpp"

BEGIN_EVENT_TABLE(cDisplayGraphic, wxPanel)
EVT_PAINT(cDisplayGraphic::PaintEvent)
END_EVENT_TABLE()

// not used anywhere, may ba cleaned up later
// using RectInt = std::vector<std::vector<int>>;
// using RectDouble = std::vector<std::vector<double>>;

// RectFloat NormalizeRect(RectInt bounds) {
//  find total desktop length and height
//  convert positions from 0-1; divide positions by lenght and width
//}

//// find top left normalized point as an origin point to draw from
// wxPoint GetNormTopLeft(RectFloat normalized);
//
//// get coefficient to fot overall drawing to space
// float fittingCoefficient = GetFittingCoefficient(normalized);
//
//// map from 0,0 to virtual top left
// auto offsetRect = OffsetNormalizedRect(normalized);
//
//// multiply by coefficient and truncate to pixel values
// auto fittedRect = FitRect(offsetRect);

cDisplayGraphic::cDisplayGraphic(wxWindow* parent, wxSize size)
    : wxPanel(parent, wxID_ANY, wxDefaultPosition, size,
              wxFULL_REPAINT_ON_RESIZE, "") {
  m_parent = parent;
}

// Called by the system of by wxWidgets when the panel needs
// to be redrawn. You can also trigger this call by
// calling Refresh()/Update().
void cDisplayGraphic::PaintEvent(wxPaintEvent& evt) {
  wxPaintDC dc(this);
  Render(dc);
}

/*
 * Alternatively, you can use a clientDC to paint on the panel
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
void cDisplayGraphic::PaintNow() {
  wxClientDC dc(this);
  Render(dc);
}

/*
 * Here we do the actual rendering. I put it in a separate
 * method so that it can work no matter what type of DC
 * (e.g. wxPaintDC or wxClientDC) is user.
 */

void cDisplayGraphic::Render(wxDC& dc) {
  // set canvas space or querry space?
  // get monitor rectangles
  // normalize sizes?
  // find center of virt desktop?
  // draw rectangle
  // find correct bound in draw pixels
  // multiply by multiplier or normalize to std size?
  // draw text inside each rectangle

  // get rect
  // normalize rect to height
  // calc imaginary top left and top right of norm
  // get fitting coefficient
  // offset rec from imaginary top left
  // fit panel to imaginary and fitted height and width
  // draw rectangles to panel
  // draw text to panel
  // make sure panel in centered?

  dc.DestroyClippingRegion();

  constexpr int pad = 5;
  const auto darkBlue = wxColor(71, 127, 255);
  const auto lightBlue = wxColor(173, 198, 255);
  const auto textHeight = dc.GetTextExtent("example").GetHeight();
  const auto halfTextHeight = textHeight / 2;

  // brush = fill
  // pen = outline
  dc.SetBrush(lightBlue);         // fill color
  dc.SetPen(wxPen(darkBlue, 5));  // 5-pixels-thick blue outline

  // test data
  std::vector<int> padding = {3, 3, 0, 0};

  int cwidth = 0;
  int cheight = 0;

  this->GetClientSize(&cwidth, &cheight);
  spdlog::info("height, width -> {}, {}", cheight, cwidth);

  double area_x = cwidth - 50;  // drawing area width
  double area_y = cheight - 50;

  // get array of monitor bounds
  auto hdi = env::GetHardwareDisplayInfo();
  auto usr = config::GetActiveProfile().displays;

  // normalize virtual desktop rect for each monitor where 1 = total width in
  // pixels values can be negative
  // auto normalized = NormalizeRect(bounds);
  int dwidth = hdi.width;
  int dheight = hdi.height;
  spdlog::info("dwidth -> {}", dwidth);
  spdlog::info("dheight -> {}", dheight);

  // offset all rectangles so that 0,0 as top left most value
  env::HardwareDisplays bounds_offset;
  {
    int l{0}, t{0};
    for (auto& d : hdi.displays) {
      l = (d[0] < l) ? d[0] : l;  // get leftmost value
      t = (d[2] < t) ? d[2] : t;  // get topmost value
    }
    for (auto& d : hdi.displays) {
      // int x = (dwidth / 2) + l;
      // int y = (dheight / 2) + t;
      bounds_offset.push_back({d[0] - l, d[1] - l, d[2] - t, d[3] - t});
    }
  }

  for (auto& d : bounds_offset) {
    spdlog::info("offset -> {}, {}, {}, {}", d[0], d[1], d[2], d[3]);
  }

  // scale rectangle so they fit in the drawing area, taking up all space
  // available
  std::vector<std::vector<double>> bounds_norm;
  {
    double xratio = area_x / dwidth;
    double yratio = area_y / dheight;
    double ratio = std::min<double>(xratio, yratio);
    for (auto& d : bounds_offset) {
      bounds_norm.push_back({static_cast<double>(d[0]) * ratio,
                             static_cast<double>(d[1]) * ratio,
                             static_cast<double>(d[2]) * ratio,
                             static_cast<double>(d[3]) * ratio});
    }
  }

  for (auto& d : bounds_norm) {
    spdlog::info("scaled to dwg area -> {}, {}, {}, {}", d[0], d[1], d[2],
                 d[3]);
  }

  // reget max height and width
  double swidth = 0;
  double sheight = 0;
  {
    double l{0}, r{0}, t{0}, b{0};
    for (auto& d : bounds_norm) {
      l = (d[0] < l) ? d[0] : l;
      r = (d[1] > r) ? d[1] : r;
      t = (d[2] < t) ? d[2] : t;
      b = (d[3] > b) ? d[3] : b;
    }
    swidth = r - l;
    sheight = b - t;
  }
  double x_offset = (area_x / 2) - (swidth / 2);
  double y_offset = (area_y / 2) - (sheight / 2);

  // get coefficient to fot overall drawing to space
  // float fittingCoefficient = GetFittingCoefficient(normalized);
  // TODO: need to find single scale factor to keep aspect ratio

  {
    for (auto& d : bounds_norm) {
      d[0] += x_offset;
      d[1] += x_offset;
      d[2] += y_offset;
      d[3] += y_offset;
    }
  }

  for (auto& d : bounds_norm) {
    spdlog::info("centered to dwg area -> {}, {}, {}, {}", d[0], d[1], d[2],
                 d[3]);
  }

  // map from 0,0 to virtual top left
  // auto offsetRect = OffsetNormalizedRect(normalized);

  // multiply by coefficient and truncate to pixel values
  // auto fittedRect = FitRect(offsetRect);

  // draw rectangles
  //
  // [in] desktop width and height
  // [in] pixel rect
  // [in] panel size or draw area with (width, height)
  // [in] rotational rect

  // draw a refernce rectangle
  // dc.DrawRectangle(wxRect(0, 0, area_x, area_y));

  for (int i = 0; i < bounds_norm.size(); i++) {
    // auto r = wxRect(xoffset, 10, width, height);

    auto r = wxRect();
    r.SetLeft(bounds_norm[i][0]);
    r.SetRight(bounds_norm[i][1]);
    r.SetTop(bounds_norm[i][2]);
    r.SetBottom(bounds_norm[i][3]);

    auto middleX = r.x + (r.GetWidth() / 2);
    auto middleY = r.y + (r.height / 2);

    wxString text_left = wxString::Format(wxT("%0.2f"), usr[i].rotation[0]);
    int text_left_x = r.x + pad;
    int text_left_y = middleY - halfTextHeight;

    wxString text_right = wxString::Format(wxT("%0.2f"), usr[i].rotation[1]);
    int text_right_x =
        r.GetRight() - pad - dc.GetTextExtent(text_right).GetWidth();
    int text_right_y = middleY - halfTextHeight;

    wxString text_top = wxString::Format(wxT("%0.2f"), usr[i].rotation[2]);
    int text_top_x = middleX - (dc.GetTextExtent(text_top).GetWidth() / 2);
    int text_top_y = r.y + pad;

    wxString text_bottom = wxString::Format(wxT("%0.2f"), usr[i].rotation[3]);
    int text_bottom_x =
        middleX - (dc.GetTextExtent(text_bottom).GetWidth() / 2);
    int text_bottom_y = r.GetBottom() - pad - textHeight;

    wxString text_center = wxString::Format(wxT("%d"), i);
    int text_center_x =
        middleX - (dc.GetTextExtent(text_center).GetWidth() / 2);
    int text_center_y = middleY - halfTextHeight;

    int xoffset = i * (width + pad);

    // Draw Objects
    dc.DrawRectangle(r);
    dc.DrawText(text_left, text_left_x, text_left_y);
    dc.DrawText(text_right, text_right_x, text_right_y);
    dc.DrawText(text_top, text_top_x, text_top_y);
    dc.DrawText(text_bottom, text_bottom_x, text_bottom_y);
    dc.DrawText(text_center, text_center_x, text_center_y);
  }

  // draw a line
  // dc.SetPen(wxPen(wxColor(0, 0, 0), 3)); // black line, 3 pixels thick
  // dc.DrawLine(300, 100, 700, 300); // draw line across the rectangle

  // Look at the wxDC docs to learn how to draw other stuff
}
