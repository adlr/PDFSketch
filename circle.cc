// Copyright...

#include "circle.h"

namespace pdfsketch {

void Circle::Serialize(pdfsketchproto::Graphic* out) const {
  Graphic::Serialize(out);
  out->set_type(pdfsketchproto::Graphic::CIRCLE);
}

void Circle::Draw(cairo_t* cr, bool selected) {
  cairo_save(cr);
  cairo_translate(cr, frame_.Left(), frame_.Top());
  cairo_scale(cr, frame_.size_.width_, frame_.size_.height_);
  cairo_arc(cr, 0.5, 0.5, 0.5, 0, 2 * M_PI);
  cairo_restore(cr);
  fill_color_.CairoSetSourceRGBA(cr);
  cairo_fill_preserve(cr);
  stroke_color_.CairoSetSourceRGBA(cr);
  cairo_set_line_width(cr, line_width_);
  cairo_stroke(cr);
}

}  // namespace pdfsketch
