// Copyright

#include <stdio.h>

#include "view.h"

namespace pdfsketch {

Rect Rect::Intersect(const Rect& that) const {
  // Based on http://svn.gna.org/svn/gnustep/libs/java/trunk/Java/gnu/gnustep/base/NSRect.java
  double maxX1 = origin_.x_ + size_.width_;
  double maxY1 = origin_.y_ + size_.height_;
  double maxX2 = that.origin_.x_ + that.size_.width_;
  double maxY2 = that.origin_.y_ + that.size_.height_;

  if ((maxX1 <= that.origin_.x_) || (maxX2 <= origin_.x_) || (maxY1 <= that.origin_.y_) 
      || (maxY2 <= origin_.y_)) {
    return Rect();
  } else {
    float newX, newY, newWidth, newHeight;

    if (that.origin_.x_ <= origin_.x_)
      newX = origin_.x_;
    else
      newX = that.origin_.x_;
    
    if (that.origin_.y_ <= origin_.y_)
      newY = origin_.y_;
    else
      newY = that.origin_.y_;
    
    if (maxX2 >= maxX1)
      newWidth = size_.width_;
    else
      newWidth = maxX2 - newX;
    
    if (maxY2 >= maxY1)
      newHeight = size_.height_;
    else
      newHeight = maxY2 - newY;
    
    return Rect(newX, newY, newWidth, newHeight);
  }
}
bool Rect::Intersects(const Rect& that) const {
  // todo: optimize
  Rect intersect = Intersect(that);
  return intersect.size_.width_ > 0 && intersect.size_.height_ > 0;
}

bool Rect::Contains(const Point& point) const {
  return origin_.x_ <= point.x_ && point.x_ < (origin_.x_ + size_.width_) &&
      origin_.y_ <= point.y_ && point.y_ < (origin_.y_ + size_.height_);
}

void MouseInputEvent::UpdateToSubview(View* subview) {
  position_ = subview->Superview()->ConvertPointToSubview(*subview, position_);
}

void MouseInputEvent::UpdateFromSubview(View* subview) {
  position_ = subview->Superview()->ConvertPointFromSubview(*subview, position_);
}

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
  printf("View::Draw\n");
  for (View* child = bottom_child_; child; child = child->upper_sibling_) {
    Rect intersect_parent = child->Frame().Intersect(rect);
    printf("checking child for draw\n");
    if (!intersect_parent.size_.width_ || !intersect_parent.size_.height_) {
      printf("no intersection\n");
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
}

void View::Resize(const Size& size) {
  double x_delta = size.width_ - size_.width_;
  double y_delta = size.height_ - size_.height_;
  size_ = size;

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

View* View::OnMouseDown(const MouseInputEvent& event) {
  // send to subviews
  for (View* child = top_child_; child; child = child->lower_sibling_) {
    View* ret = NULL;
    if (child->Frame().Contains(event.position())) {
      MouseInputEvent child_evt(event);
      child_evt.UpdateToSubview(child);
      ret = child->OnMouseDown(child_evt);
    }
    if (ret)
      return ret;
  }
  return NULL;
}

Point View::ConvertPointFromSubview(const View& subview, const Point& point) const {
  return Point(point.x_ * subview.scale_, point.y_ * subview.scale_).
      TranslatedBy(subview.origin_.x_, subview.origin_.y_);
}
Size View::ConvertSizeFromSubview(const View& subview, const Size& size) const {
  return size.ScaledBy(subview.scale_);
}

Point View::ConvertPointToSubview(const View& subview, Point point) const {
  if (subview.Superview() != this) {
    if (!subview.Superview()) {
      printf("Missing superview\n");
      return Point();
    }
    point = subview.Superview()->ConvertPointToSubview(subview, point);
  }
  Point temp = point.TranslatedBy(-subview.origin_.x_, -subview.origin_.y_);
  return Point(temp.x_ / subview.scale_, temp.y_ / subview.scale_);
}

}  // namespace pdfsketch
