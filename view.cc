// Copyright

#include "view.h"

#include <stdio.h>

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

// Algorithm from https://github.com/adlr/formulatepro/blob/master/FPGraphic.m
bool Rect::SetBottomAbs(double bottom) {
  bool flip = bottom < origin_.y_;
  if (flip) {
    size_.height_ = origin_.y_ - bottom;
    origin_.y_ = bottom;
  } else {
    size_.height_ = bottom - origin_.y_;
  }
  return flip;
}
bool Rect::SetRightAbs(double right) {
  bool flip = right < origin_.x_;
  if (flip) {
    size_.width_ = origin_.x_ - right;
    origin_.x_ = right;
  } else {
    size_.width_ = right - origin_.x_;
  }
  return flip;
}
bool Rect::SetTopAbs(double top) {
  bool flip = top > (origin_.y_ + size_.height_);
  if (flip) {
    origin_.y_ += size_.height_;
    size_.height_ = top - origin_.y_;
  } else {
    size_.height_ += (origin_.y_ - top);
    origin_.y_ = top;
  }
  return flip;
}
bool Rect::SetLeftAbs(double left) {
  bool flip = left > (origin_.x_ + size_.width_);
  if (flip) {
    origin_.x_ += size_.width_;
    size_.width_ = left - origin_.x_;
  } else {
    size_.width_ += (origin_.x_ - left);
    origin_.x_ = left;
  }
  return flip;
}

void MouseInputEvent::UpdateToSubview(View* subview,
                                      View* from_superview) {
  position_ =
      from_superview->ConvertPointToSubview(*subview, position_);
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

void View::RemoveSubview(View* subview) {
  if (this != subview->parent_) {
    printf("Removing view from non-parent!\n");
    return;
  }
  View* prev_upper = subview->upper_sibling_;
  View* prev_lower = subview->lower_sibling_;
  subview->parent_ = NULL;
  if (prev_upper)
    prev_upper->lower_sibling_ = prev_lower;
  if (prev_lower)
    prev_lower->upper_sibling_ = prev_upper;
  if (top_child_ == subview)
    top_child_ = prev_lower;
  if (bottom_child_ == subview)
    bottom_child_ = prev_upper;
  subview->upper_sibling_ = subview->lower_sibling_ = NULL;
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
  for (View* child = bottom_child_; child; child = child->upper_sibling_) {
    Rect intersect_parent = child->Frame().Intersect(rect);
    if (!intersect_parent.size_.width_ || !intersect_parent.size_.height_) {
      continue;  // No intersection
    }
    cairo_save(ctx);
    intersect_parent.CairoRectangle(ctx);
    cairo_clip(ctx);
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

Rect View::VisibleSubrect() const {
  if (!parent_)
    return Rect(size_);
  // TODO(adlr): fill this in
  Rect parent_visible = parent_->ConvertRectToSubview(*this,
                                                      parent_->VisibleSubrect());
  return parent_visible.Intersect(Rect(size_));
}

void View::Resize(const Size& size) {
    printf("%s:%d\n", __FILE__, __LINE__);
  double x_delta = size.width_ - size_.width_;
    printf("%s:%d\n", __FILE__, __LINE__);
  double y_delta = size.height_ - size_.height_;
    printf("%s:%d\n", __FILE__, __LINE__);
  SetSize(size);
    printf("%s:%d\n", __FILE__, __LINE__);

  // Update subview frames
  for (View* child = top_child_; child; child = child->lower_sibling_) {
    printf("%s:%d\n", __FILE__, __LINE__);
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
    printf("%s:%d\n", __FILE__, __LINE__);
    child->SetFrame(frame);
    printf("%s:%d\n", __FILE__, __LINE__);
  }
    printf("%s:%d\n", __FILE__, __LINE__);
}

void View::SetFrame(const Rect& frame) {
  origin_ = frame.origin_;
  Size new_size = frame.size_.ScaledBy(1 / scale_);
  if (new_size != size_)
    Resize(new_size);
  if (delegate_)
    delegate_->ViewFrameChanged(this, frame);
}

void View::SetSize(const Size& size) {
  printf("%s:%d\n", __FILE__, __LINE__);
  size_ = size;
  printf("%s:%d\n", __FILE__, __LINE__);
  if (delegate_) {
    printf("%s:%d\n", __FILE__, __LINE__);
    printf("this: 0x%08zx del: 0x%08zx\n",
           (size_t)this, (size_t)delegate_);
    delegate_->ViewFrameChanged(this, Frame());
    printf("%s:%d\n", __FILE__, __LINE__);
  }
  printf("%s:%d\n", __FILE__, __LINE__);
}

void View::SetScale(double scale) {
  scale_ = scale;
  if (delegate_)
    delegate_->ViewFrameChanged(this, Frame());
}

View* View::OnMouseDown(const MouseInputEvent& event) {
  // send to subviews
  for (View* child = top_child_; child; child = child->lower_sibling_) {
    View* ret = NULL;
    if (child->Frame().Contains(event.position())) {
      MouseInputEvent child_evt(event);
      child_evt.UpdateToSubview(child, this);
      ret = child->OnMouseDown(child_evt);
    }
    if (ret)
      return ret;
  }
  return NULL;
}

bool View::OnKeyText(const KeyboardInputEvent& event) {
  for (View* child = top_child_; child; child = child->lower_sibling_)
    if (child->OnKeyText(event))
      return true;
  return false;
}
bool View::OnKeyDown(const KeyboardInputEvent& event) {
  for (View* child = top_child_; child; child = child->lower_sibling_)
    if (child->OnKeyDown(event))
      return true;
  return false;
}
bool View::OnKeyUp(const KeyboardInputEvent& event) {
  for (View* child = top_child_; child; child = child->lower_sibling_)
    if (child->OnKeyUp(event))
      return true;
  return false;
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
    point = ConvertPointToSubview(*subview.Superview(), point);
    return subview.Superview()->ConvertPointToSubview(subview, point);
  }
  Point temp = point.TranslatedBy(-subview.origin_.x_, -subview.origin_.y_);
  return Point(temp.x_ / subview.scale_, temp.y_ / subview.scale_);
}
Rect View::ConvertRectToSubview(const View& subview, const Rect& rect) const {
  return Rect(ConvertPointToSubview(subview, rect.UpperLeft()),
              ConvertPointToSubview(subview, rect.LowerRight()));
}

}  // namespace pdfsketch
