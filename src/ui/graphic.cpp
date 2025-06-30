#include "ui/graphic.hpp"

#include "windows-wrapper.hpp"
#include <wx/wx.h>

#include <algorithm>
#include <vector>

#include "environment.hpp"
#include "settings.hpp"
#include "types.hpp"

PanelDisplayGraphic::PanelDisplayGraphic(wxWindow* parent, Settings& s)
  : p_parent(parent)
  , settings(s)
  , wxPanel(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxFULL_REPAINT_ON_RESIZE, "")
{
  Bind(wxEVT_PAINT, &PanelDisplayGraphic::OnPaint, this);
}

void
PanelDisplayGraphic::OnPaint(wxPaintEvent& evt)
{
  wxPaintDC dc(this);
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

  int client_width = 0;
  int client_height = 0;

  // Determine available space for graphic.
  wxSize area = this->GetClientSize();
  spdlog::debug("graphic: client height, client width -> {}x{}", client_height, client_width);

  // get array of monitor bounds
  const auto hdi = WinDisplayInfo();
  const auto usrDisplays = settings.GetActiveProfileRef().displays;

  /* Note: I opt to use std::transform instead of modifying my container in place simply for the
   * ease of debugging.
   */

  // Normalize all coordinates to 0,0 as the top-left corner.
  std::vector<RectPixels> normalized = hdi.Normalize();

  const double xratio = static_cast<double>(area.GetWidth()) / hdi.desktop_width;
  const double yratio = static_cast<double>(area.GetHeight()) / hdi.desktop_height;
  const double ratio = std::min(xratio, yratio);

  // Scale rectangle so they fit in the drawing area, taking up all space
  // available.
  std::vector<RectPixels> scaled(normalized.size());

  std::transform(normalized.begin(), normalized.end(), scaled.begin(), [ratio](RectPixels r) {
    return RectPixels{ static_cast<Pixels>(r.left * ratio),
                       static_cast<Pixels>(r.right * ratio),
                       static_cast<Pixels>(r.top * ratio),
                       static_cast<Pixels>(r.bottom * ratio) };
  });

  for (int i = 0; i < scaled.size(); i++) {
    // Draw rectangle.
    auto r = wxRect();
    r.SetLeft(scaled[i].left);
    r.SetRight(scaled[i].right);
    r.SetTop(scaled[i].top);
    r.SetBottom(scaled[i].bottom);
    dc.SetBrush(light_gray);        // fill color
    dc.SetPen(wxPen(dark_gray, 3)); // outline
    dc.DrawRectangle(r);

    const bool userSpecifiedRotationAvailable = (i < usrDisplays.size());

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
