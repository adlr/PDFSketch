// Copyright...

#include "document_view.h"

namespace pdfsketch {

void DocumentView::DrawRect(cairo_t* cr, const Rect& rect) {
  double border = 15.0;
  cairo_set_line_width(2.0);
  cairo_set_line_join(cr, CAIRO_LINE_JOIN_MITER);
  cairo_set_line_cap(cr, CAIRO_LINE_CAP_SQUARE);
  for (int i = 0; i < 3; i++) {
    Rect page(0.0, double(i) * size_.height_ / 3.0,
              size_.width_, size_.height_ / 3.0);
    page = page.InsetBy(border + 1);
    cairo_set_source_rgb(cr, i == 0, i == 1, i == 2);
    page.CairoRectangle(cr);
    cairo_stroke(cr);
  }
}

}  // namespace pdfsketch
