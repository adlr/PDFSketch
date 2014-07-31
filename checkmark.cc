// Copyright...

#include "checkmark.h"

namespace pdfsketch {

void Checkmark::Serialize(pdfsketchproto::Graphic* out) const {
  Graphic::Serialize(out);
  out->set_type(pdfsketchproto::Graphic::CHECKMARK);
}

void Checkmark::Place(int page, const Point& location) {
  page_ = page;
  frame_.size_ = Size(9.0, 9.0);
  PlaceUpdate(location);
}

void Checkmark::PlaceUpdate(const Point& location) {
  SetNeedsDisplay(false);
  frame_.SetCenter(location);
  SetNeedsDisplay(false);
}

void Checkmark::Draw(cairo_t* cr, bool selected) {
  stroke_color_.CairoSetSourceRGBA(cr);
  cairo_set_line_width(cr, line_width_);
  frame_.UpperLeft().CairoMoveTo(cr);
  frame_.LowerRight().CairoLineTo(cr);
  frame_.UpperRight().CairoMoveTo(cr);
  frame_.LowerLeft().CairoLineTo(cr);
  cairo_stroke(cr);
}

}  // namespace pdfsketch
