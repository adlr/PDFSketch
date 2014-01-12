// Copyright

#include <math.h>
#include <stdio.h>

#include "root_view.h"

namespace pdfsketch {

void RootView::DrawRect(cairo_t* cr, const Rect& rect) {
  View::DrawRect(cr, rect);
  for (int i = 0; i < 4; i++) {
    double x_pos = 0.0, y_pos = 0.0;
    switch (i) {
      case 1:
        x_pos = size_.width_;
        break;
      case 2:
        x_pos = size_.width_;
        y_pos = size_.height_;
        break;
      case 3:
        y_pos = size_.height_;
    }
    cairo_move_to(cr, x_pos, y_pos);
    cairo_arc(cr, x_pos, y_pos, 10.0, 0.0, 2 * M_PI);
  }
  cairo_stroke(cr);
}

void RootView::SetNeedsDisplayInRect(const Rect& rect) {
  if (!delegate_) {
    printf("%s: can't draw, no delegate\n", __func__);
    return;
  }
  if (!draw_requested_) {
    draw_requested_ = true;
    pp::MessageLoop::GetCurrent().PostWork(
        callback_factory_.NewCallback(&RootView::HandleDrawRequest));
  }
}

void RootView::HandleDrawRequest(int32_t result) {
  draw_requested_ = false;
  cairo_t* cr = delegate_->AllocateCairo();
  DrawRect(cr, Bounds());
  delegate_->FlushCairo();
}

void RootView::Resize(const Size& size) {
  View::Resize(size);
  SetNeedsDisplay();
}

}  // namespace pdfsketch
