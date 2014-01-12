// Copyright

#include <stdio.h>

#include "view.h"

namespace pdfsketch {

void View::AddSubview(View* subview) {
  if (subview->parent_) {
    printf("%s: Subview has parent already!\n", __func__);
    return;
  }
  subview->parent_ = this;
  subview->upper_sibling_ = NULL;
  subview->lower_sibling_ = top_child_;
  if (top_child_)
    top_child_->upper_sibling_ = subview;
  top_child_ = subview;
  if (!bottom_child_)
    bottom_child_ = top_child_;
}

void View::SetNeedsDisplayInRect(const Rect& rect) {
  if (!parent_) {
    printf("%s: Missing parent!\n", __func__);
    return;
  }
  parent_->SetNeedsDisplayInRect(parent_->ConvertRectFromSubview(*this, rect));
}

void View::DrawRect(cairo_t* ctx, const Rect& rect) {
  // Draw each child
  /*
  for (View* child = bottom_child_; child; child = child->upper_sibling_) {
    Rect intersect_parent = child->Frame().Intersect(rect);
    if (!intersect.size_.width_ || !intersect.size_.height_) {
      continue;  // No intersection
    }
    cairo_save(ctx);
    cairo_translate(ctx, child->origin_.x_, child->origin_.y_);
    cairo_scale(ctx, child->scale_, child->scale_);
    Rect intersect_child =
        intersect_parent.TranslatedBy(-child->origin_.x_,
                                      -child->origin_.y_).
        ScaledBy(1 / child->scale_);
    child->DrawRect(ctx, intersect_child);
    cairo_restore(ctx);
  }
  */
}

void View::Resize(const Size& size) {
  double x_delta = size.width_ - size_.width_;
  double y_delta = size.height_ - size_.height_;

  // Update subview frames
  for (View* child = top_child_; child; child = child->lower_sibling_) {
    Rect frame = child->Frame();
    if (!child->top_fixed_to_top_) {
      // Set top of rect w/o changing bottom
      frame.origin_.y_ += y_delta;
      frame.size_.height_ -= y_delta;
    }
    if (!child->bot_fixed_to_top_) {
      frame.size_.height_ += y_delta;
    }
    if (!child->left_fixed_to_left_) {
      // Set left of rect w/o changing right
      frame.origin_.x_ += x_delta;
      frame.size_.width_ -= x_delta;
    }
    if (!child->right_fixed_to_left_) {
      frame.size_.width_ += x_delta;
    }
    child->SetFrame(frame);
  }
}

void View::SetFrame(const Rect& frame) {
  origin_ = frame.origin_;
  Size new_size = frame.size_.ScaledBy(1 / scale_);
  if (new_size != size_)
    Resize(new_size);
}

Point View::ConvertPointFromSubview(const View& subview, const Point& point) {
  return Point(point.x_ * subview.scale_, point.y_ * subview.scale_).
      TranslatedBy(subview.origin_.x_, subview.origin_.y_);
}
Size View::ConvertSizeFromSubview(const View& subview, const Size& size) {
  return size.ScaledBy(subview.scale_);
}


}  // namespace pdfsketch
