// Copyright

#include "view.h"

#include <algorithm>
#include <stdio.h>

using std::max;
using std::min;
using std::string;

namespace pdfsketch {

Rect Rect::Intersect(const Rect& that) const {
  Rect ret = Rect(Point(max(Left(), that.Left()),
                        max(Top(), that.Top())),
                  Point(min(Right(), that.Right()),
                        min(Bottom(), that.Bottom())));
  ret.size_.width_ = max(ret.size_.width_, 0.0);
  ret.size_.height_ = max(ret.size_.height_, 0.0);
  return ret;
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

void ScrollInputEvent::UpdateToSubview(View* subview, View* from_superview) {
  Size size(dx(), dy());
  size = from_superview->ConvertSizeToSubview(*subview, size);
  dx_ = size.width_;
  dy_ = size.height_;
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
  double x_delta = size.width_ - size_.width_;
  double y_delta = size.height_ - size_.height_;
  SetSize(size);

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
  Rect old_frame = Frame();
  origin_ = frame.origin_;
  Size new_size = frame.size_.ScaledBy(1 / scale_);
  if (new_size != size_)
    Resize(new_size);
  if (delegate_)
    delegate_->ViewFrameChanged(this, frame, old_frame);
}

void View::SetSize(const Size& size) {
  Rect old_frame = Frame();
  size_ = size;
  if (delegate_) {
    delegate_->ViewFrameChanged(this, Frame(), old_frame);
  }
}

void View::SetScale(double scale) {
  Rect old_frame = Frame();
  scale_ = scale;
  if (delegate_)
    delegate_->ViewFrameChanged(this, Frame(), old_frame);
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

void View::OnScrollEvent(const ScrollInputEvent& event) {
  // TODO(adlr): Only pass where mouse pointer is
  for (View* child = top_child_; child; child = child->lower_sibling_)
    child->OnScrollEvent(event);
}

string View::OnCopy() {
  string ret;
  for (View* child = top_child_; child; child = child->lower_sibling_) {
    ret = child->OnCopy();
    if (!ret.empty())
      break;
  }
  return ret;
}

bool View::OnPaste(const string& str) {
  for (View* child = top_child_; child; child = child->lower_sibling_)
    if (child->OnPaste(str))
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
Size View::ConvertSizeToSubview(const View& subview, const Size& size) const {
  return ConvertRectToSubview(subview, Rect(size)).size_;
}

}  // namespace pdfsketch
