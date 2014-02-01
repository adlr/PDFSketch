// Copyright...

#include "graphic.h"

namespace pdfsketch {

namespace {
const double kKnobEdgeLength = 7.0;
const double kKnobLineWidth = 1.0;
}  // namespace {}

Rect Graphic::DrawingFrame() const {
  return frame_.InsetBy(-line_width_ / 2.0);
}

Rect Graphic::DrawingFrameWithKnobs() const {
  // TODO(adlr): be more thorough here
  Rect draw_frame = DrawingFrame();
  Rect knob_frame(KnobFrame(kKnobUpperLeft));
  if (knob_frame.Left() < draw_frame.Left()) {
    return Rect(KnobFrame(kKnobUpperLeft).UpperLeft(),
                KnobFrame(kKnobLowerRight).LowerRight());
  }
  return draw_frame;
}

Rect Graphic::KnobFrame(int knob) const {
  if (!delegate_) {
    printf("missing delegate!\n");
    return Rect();
  }
  Point center(frame_.origin_);
  switch (knob) {
    case kKnobUpperMiddle:
    case kKnobLowerMiddle:
      center.x_ += frame_.size_.width_ / 2.0;
      break;
    case kKnobUpperRight:
    case kKnobMiddleRight:
    case kKnobLowerRight:
      center.x_ += frame_.size_.width_;
      break;
  }
  switch (knob) {
    case kKnobMiddleLeft:
    case kKnobMiddleRight:
      center.y_ += frame_.size_.height_ / 2.0;
      break;
    case kKnobLowerLeft:
    case kKnobLowerMiddle:
    case kKnobLowerRight:
      center.y_ += frame_.size_.height_;
      break;
  }

  Point knob_upper_left =
      delegate_->ConvertPointToGraphic(
          Page(),
          delegate_->ConvertPointFromGraphic(Page(), center).Rounded().
          TranslatedBy(-kKnobEdgeLength / 2.0, -kKnobEdgeLength / 2.0));
  Point knob_lower_right =
      delegate_->ConvertPointToGraphic(
          Page(),
          delegate_->ConvertPointFromGraphic(Page(), center).Rounded().
          TranslatedBy(kKnobEdgeLength / 2.0, kKnobEdgeLength / 2.0));
  printf("center: %s corners: %s, %s\n",
         center.String().c_str(),
         knob_upper_left.String().c_str(),
         knob_lower_right.String().c_str());

  return Rect(knob_upper_left, knob_lower_right);

}

Rect Graphic::DrawingKnobFrame(int knob) const {
  return KnobFrame(knob).InsetBy(-kKnobLineWidth / 2.0);
}

int Graphic::PointInKnob(const Point& location) const {
  if (!delegate_)
    return kKnobNone;
  for (int i = 0; i < 8; i++) {
    int knob = 1 << i;
    if (!(Knobs() & knob))
      continue;
    Rect frame = KnobFrame(knob);
    if (frame.Contains(location))
      return knob;
  }
  printf("no knobs hit\n");
  return kKnobNone;
}

void Graphic::SetNeedsDisplay(bool withKnobs) const {
  if (!delegate_)
    return;
  delegate_->SetNeedsDisplayInPageRect(Page(),
                                       withKnobs ?
                                       DrawingFrameWithKnobs() :
                                       DrawingFrame());
}

void Graphic::Place(int page, const Point& location, bool constrain) {
  page_ = page;
  frame_ = Rect(location);
  resizing_knob_ = kKnobLowerRight;
}
void Graphic::PlaceUpdate(const Point& location, bool constrain) {
  UpdateResize(location, constrain);
}
bool Graphic::PlaceComplete() {
  resizing_knob_ = kKnobNone;
  return frame_.size_ == Size();
}

void Graphic::BeginResize(const Point& location, int knob, bool constrain) {
  printf("graphic: being resize\n");
  resizing_knob_ = knob;
  UpdateResize(location, constrain);
}
void Graphic::UpdateResize(const Point& location, bool constrain) {
  // Structure from https://github.com/adlr/formulatepro/blob/master/FPGraphic.m
  printf("UpdateResize before: %s\n", frame_.String().c_str());

  double shift_slope = 0.0;
  if (frame_.size_.width_ > 0.0 &&
      frame_.size_.height_ > 0.0)
    shift_slope = frame_.size_.height_ / frame_.size_.width_;
  else
    shift_slope = natural_size_.height_ / natural_size_.width_;

  if (delegate_) {
    delegate_->SetNeedsDisplayInPageRect(Page(), DrawingFrameWithKnobs());
  }
  bool flip_x = false;
  bool flip_y = false;
  if (resizing_knob_ == kKnobUpperLeft ||
      resizing_knob_ == kKnobUpperMiddle ||
      resizing_knob_ == kKnobUpperRight)
    flip_y = frame_.SetTopAbs(location.y_);
  if (resizing_knob_ == kKnobLowerLeft ||
      resizing_knob_ == kKnobLowerMiddle ||
      resizing_knob_ == kKnobLowerRight)
    flip_y = frame_.SetBottomAbs(location.y_);
  if (resizing_knob_ == kKnobUpperLeft ||
      resizing_knob_ == kKnobMiddleLeft ||
      resizing_knob_ == kKnobLowerLeft)
    flip_x = frame_.SetLeftAbs(location.x_);
  if (resizing_knob_ == kKnobUpperRight ||
      resizing_knob_ == kKnobMiddleRight ||
      resizing_knob_ == kKnobLowerRight)
    flip_x = frame_.SetRightAbs(location.x_);
        
  if (flip_y) {
    switch (resizing_knob_) {
      case kKnobUpperLeft: resizing_knob_ = kKnobLowerLeft; break;
      case kKnobUpperMiddle: resizing_knob_ = kKnobLowerMiddle; break;
      case kKnobUpperRight: resizing_knob_ = kKnobLowerRight; break;
      case kKnobLowerLeft: resizing_knob_ = kKnobUpperLeft; break;
      case kKnobLowerMiddle: resizing_knob_ = kKnobUpperMiddle; break;
      case kKnobLowerRight: resizing_knob_ = kKnobUpperRight; break;
    }
  }
  if (flip_x) {
    switch (resizing_knob_) {
      case kKnobUpperLeft: resizing_knob_ = kKnobUpperRight; break;
      case kKnobMiddleLeft: resizing_knob_ = kKnobMiddleRight; break;
      case kKnobLowerLeft: resizing_knob_ = kKnobLowerRight; break;
      case kKnobUpperRight: resizing_knob_ = kKnobUpperLeft; break;
      case kKnobMiddleRight: resizing_knob_ = kKnobMiddleLeft; break;
      case kKnobLowerRight: resizing_knob_ = kKnobLowerLeft; break;
    }
  }

  if (constrain) {
    /*
    switch (resizing_knob_) {
      case kKnobUpperRight:
      case kKnobLowerRight:
        didFlip = FPRectSetRightAbs(&_bounds,
                                    _bounds.origin.x +
                                    (_bounds.size.height /
                                     shiftSlope));
        break;
      case kKnobLowerLeft:
      case kKnobUpperLeft:
        didFlip = FPRectSetLeftAbs(&_bounds,
                                   _bounds.origin.x +
                                   _bounds.size.width -
                                   (_bounds.size.height /
                                    shiftSlope));
        break;
        }*/
  }
  if (delegate_) {
    delegate_->SetNeedsDisplayInPageRect(Page(), DrawingFrameWithKnobs());
  }
  printf("UpdateResize after: %s\n", frame_.String().c_str());
}
void Graphic::EndResize() {
  resizing_knob_ = kKnobNone;
}

void Graphic::DrawKnobs(cairo_t* cr) {
  if (!delegate_)
    return;
  for (int i = 0; i < 8; i++) {
    int knob = 1 << i;
    if (!(Knobs() & knob))
      continue;
    Rect frame = KnobFrame(knob);
    frame.CairoRectangle(cr);
    cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, 1.0);  // opaque white
    cairo_fill_preserve(cr);
    cairo_set_source_rgba(cr, 0.0, 0.0, 0.0, 1.0);  // opaque black
    cairo_set_line_width(cr, kKnobLineWidth / delegate_->GetZoom());
    cairo_stroke(cr);
  }
}

}  // namespace pdfsketch
