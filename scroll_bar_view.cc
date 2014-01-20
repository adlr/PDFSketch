// Copyright...

#include <stdio.h>

#include "scroll_bar_view.h"

namespace pdfsketch {

void ScrollBarView::DrawRect(cairo_t* cr, const Rect& rect) {
  cairo_move_to(cr, size_.width_, 0.0);
  cairo_line_to(cr, 0.0, size_.height_);
}

View* ScrollBarView::OnMouseDown(const MouseInputEvent& event) {
  printf("Down at %f %f\n", event.position().x_, event.position().y_);
  return this;
}
void ScrollBarView::OnMouseDrag(const MouseInputEvent& event) {
  printf("Drag at %f %f\n", event.position().x_, event.position().y_);
}
void ScrollBarView::OnMouseUp(const MouseInputEvent& event) {
}

}  // namespace pdfsketch
