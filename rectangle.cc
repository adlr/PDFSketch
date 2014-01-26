// Copyright...

#include "rectangle.h"

namespace pdfsketch {

void Rectangle::Draw(cairo_t* cr) {
  frame_.CairoRectangle(cr);
  fill_color_.CairoSetSourceRGBA(cr);
  cairo_fill_preserve(cr);
  stroke_color_.CairoSetSourceRGBA(cr);
  cairo_set_line_width(cr, line_width_);
  cairo_stroke(cr);
}

}  // namespace pdfsketch
