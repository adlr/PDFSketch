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

  cairo_save(cr);
  cairo_set_line_width(cr, 1.0);
  cairo_set_line_join(cr, CAIRO_LINE_JOIN_MITER);
  cairo_set_line_cap(cr, CAIRO_LINE_CAP_SQUARE);
  double width = size_.width_;
  if (!vertical_) {
    // swap x and y axes when drawing
    cairo_matrix_t swap = {
      0.0, 1.0, 1.0, 0.0, 0.0, 0.0
    };
    cairo_transform(cr, &swap);
    width = size_.height_;
  }

  cairo_set_source_rgb(cr, 0.5, 0.5, 0.5);
  cairo_move_to(cr, 0.5, end_pix - 0.5);
  cairo_line_to(cr, width - 0.5, end_pix - 0.5);
  cairo_line_to(cr, width - 0.5, start_pix + 0.5);
  cairo_stroke(cr);

  cairo_set_source_rgb(cr, 0.2, 0.2, 0.2);
  cairo_move_to(cr, 0.5, end_pix - 0.5);
  cairo_line_to(cr, 0.5, start_pix + 0.5);
  cairo_line_to(cr, width - 0.5, start_pix + 0.5);
  cairo_stroke(cr);
  cairo_restore(cr);
}

View* ScrollBarView::OnMouseDown(const MouseInputEvent& event) {
  drag_start_pos_ = vertical_ ? event.position().y_ : event.position().x_;
  drag_start_show_min_ = show_min_;
  return this;
}

void ScrollBarView::OnMouseDrag(const MouseInputEvent& event) {
  double pos = vertical_ ? event.position().y_ : event.position().x_;
  double delta = pos - drag_start_pos_;
  double new_show_min = drag_start_show_min_ + delta * (doc_max_ - doc_min_) /
      (vertical_ ? size_.height_ : size_.width_);
  ScrollBy(new_show_min - show_min_);
}

void ScrollBarView::ScrollBy(double delta) {
  if (delta == 0.0)
    return;
  show_min_ += delta;

  // clamp show_min_ to reasonable range
  show_min_ = std::max(show_min_, doc_min_);
  if (show_min_ + show_size_ > doc_max_) {
    show_min_ = doc_max_ - show_size_;
  }

  if (delegate_)
    delegate_->ScrollBarMovedTo(this, show_min_);

  SetNeedsDisplay();
}

void ScrollBarView::CenterDocValue(double value) {
  double new_min = value - show_size_ / 2.0;
  ScrollBy(new_min - show_min_);
}

void ScrollBarView::OnMouseUp(const MouseInputEvent& event) {}

const double ScrollBarView::kThickness = 15.0;

}  // namespace pdfsketch
