// Copyright...

#include "squiggle.h"

#include <algorithm>

using std::max;
using std::min;

namespace pdfsketch {

Squiggle::Squiggle(const pdfsketchproto::Graphic& msg)
    : Graphic(msg),
      original_origin_(msg.squiggle().original_origin()) {
  for (int i = 0; i < msg.squiggle().point_size(); i++) {
    points_.push_back(Point(msg.squiggle().point(i)));
  }
}

void Squiggle::Serialize(pdfsketchproto::Graphic* out) const {
  Graphic::Serialize(out);
  out->set_type(pdfsketchproto::Graphic::SQUIGGLE);
  pdfsketchproto::Squiggle* msg = out->mutable_squiggle();
  original_origin_.Serialize(msg->mutable_original_origin());
  for (auto it = points_.begin(), e = points_.end(); it != e; ++it) {
    it->Serialize(msg->add_point());
  }
}

void Squiggle::Place(int page, const Point& location, bool constrain) {
  page_ = page;
  points_.push_back(location);
  frame_ = Rect(location);
  original_origin_ = location;
}

void Squiggle::PlaceUpdate(const Point& location, bool constrain) {
  if (points_.back() == location)
    return;
  points_.push_back(location);
  if (location.x_ < frame_.Left())
    frame_.SetLeftAbs(location.x_);
  if (location.x_ > frame_.Right())
    frame_.SetRightAbs(location.x_);
  if (location.y_ < frame_.Top())
    frame_.SetTopAbs(location.y_);
  if (location.y_ > frame_.Bottom())
    frame_.SetBottomAbs(location.y_);
  original_origin_ = frame_.origin_;
  natural_size_ = frame_.size_;
  SetNeedsDisplay(false);
}

bool Squiggle::PlaceComplete() {
  if (frame_.size_ == Size())
    return true;  // empty, so delete
  return false;

  // Now that all points are placed, compute proper frame, and update
  // points_
  double left = frame_.origin_.x_;
  double right = left;
  double top = frame_.origin_.y_;
  double bottom = top;
  for (auto it = points_.begin(), e = points_.end(); it != e; ++it) {
    left = min(left, it->x_ + frame_.Left());
    right = max(right, it->x_ + frame_.Left());
    top = min(top, it->y_ + frame_.Top());
    bottom = max(bottom, it->y_ + frame_.Top());
  }
  Point old_origin = frame_.origin_;
  frame_ = Rect(Point(left, top), Point(right, bottom));
  if (frame_.size_ == Size())
    return true;  // empty, so delete
  Point delta = frame_.origin_.Subtract(old_origin);
  for (auto it = points_.begin(), e = points_.end(); it != e; ++it) {
    *it = it->Add(delta);
  }  
  natural_size_ = frame_.size_;

  return false;
}

void Squiggle::Draw(cairo_t* cr, bool selected) {
  if (points_.size() < 2)
    return;
  if (natural_size_.height_ <= 0.0 ||
      natural_size_.width_ <= 0.0)
    return;  // Avoid divide by 0 below
  if (frame_.size_.width_ < 1.0e-7 ||
      frame_.size_.height_ < 1.0e-7)
    return;  // too skinny to draw
  cairo_save(cr);  // transform for h/v flip
  if (h_flip_) {
    cairo_translate(cr,
                    2.0 * frame_.origin_.x_ + frame_.size_.width_,
                    0.0);
    cairo_scale(cr, -1.0, 1.0);
  }
  if (v_flip_) {
    cairo_translate(cr, 0.0,
                    2.0 * frame_.origin_.y_ + frame_.size_.height_);
    cairo_scale(cr, 1.0, -1.0);
  }
  cairo_save(cr);
  cairo_translate(cr, frame_.origin_.x_ , frame_.origin_.y_ );
  cairo_scale(cr, frame_.size_.width_ / natural_size_.width_,
              frame_.size_.height_ / natural_size_.height_);
  cairo_translate(cr, -original_origin_.x_, -original_origin_.y_);
  points_.begin()->CairoMoveTo(cr);
  for (auto it = ++(points_.begin()), e = points_.end(); it != e; ++it)
    it->CairoLineTo(cr);
  cairo_restore(cr);
  stroke_color_.CairoSetSourceRGBA(cr);
  cairo_set_line_width(cr, line_width_);
  cairo_stroke(cr);
  cairo_restore(cr);  // h/v flip
}

}  // namespace pdfsketch
