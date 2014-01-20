// Copyright...

#include <stdio.h>

#include <cairo.h>

#include "scroll_bar_view.h"

namespace pdfsketch {

void ScrollBarView::DrawRect(cairo_t* cr, const Rect& rect) {
  double doc_size = doc_max_ - doc_min_;
  if (doc_size <= 0.0) {
    printf("doc to small for scrollbar to draw\n");
    return;
  }

  // range [0..1]:
  double start = (show_min_ - doc_min_) / doc_size;
  double end = (show_min_ + show_size_ - doc_min_) / doc_size;

  // pixel values:
  double start_pix = start * (vertical_ ? size_.height_ : size_.width_);
  double end_pix = end * (vertical_ ? size_.height_ : size_.width_);

  start_pix = static_cast<int>(start_pix);
  end_pix = static_cast<int>(end_pix);

  cairo_set_line_width(cr, 1.0);
  cairo_set_line_join(cr, CAIRO_LINE_JOIN_BEVEL);
  if (vertical_) {
    cairo_move_to(cr, 0.5, end_pix - 0.5);
    cairo_line_to(cr, 0.5, start_pix + 0.5);
    cairo_line_to(cr, size_.width_ - 0.5, start_pix + 0.5);
  } else {
    cairo_move_to(cr, end_pix - 0.5, 0.5);
    cairo_line_to(cr, start_pix + 0.5, 0.5);
    cairo_line_to(cr, start_pix + 0.5, size_.width_ - 0.5);
  }
  cairo_stroke(cr);
}

View* ScrollBarView::OnMouseDown(const MouseInputEvent& event) {
  drag_start_pos_ = vertical_ ? event.position().y_ : event.position().x_;
  drag_start_show_min_ = show_min_;
  return this;
}

void ScrollBarView::OnMouseDrag(const MouseInputEvent& event) {
  double pos = vertical_ ? event.position().y_ : event.position().x_;
  double delta = pos - drag_start_pos_;
  show_min_ = drag_start_show_min_ + delta * (doc_max_ - doc_min_) /
      (vertical_ ? size_.height_ : size_.width_);

  // clamp show_min_ to reasonable range
  show_min_ = std::max(show_min_, doc_min_);
  if (show_min_ + show_size_ > doc_max_)
    show_min_ = doc_max_ - show_size_;

  SetNeedsDisplay();
}

void ScrollBarView::OnMouseUp(const MouseInputEvent& event) {}

}  // namespace pdfsketch
