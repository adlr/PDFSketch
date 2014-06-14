// Copyright

#include <math.h>
#include <stdio.h>

#include "root_view.h"

namespace pdfsketch {

void Dbg(const char* str);

void RootView::DrawRect(cairo_t* cr, const Rect& rect) {
  View::DrawRect(cr, rect);
  /*
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
  */
}

void RootView::SetNeedsDisplayInRect(const Rect& rect) {
  if (!delegate_) {
    printf("%s: can't draw, no delegate\n", __func__);
    return;
  }
  if (!draw_requested_ && !flush_in_progress_) {
    pp::MessageLoop::GetCurrent().PostWork(
        callback_factory_.NewCallback(&RootView::HandleDrawRequest));
  }
  draw_requested_ = true;
}

void RootView::HandleDrawRequest(int32_t result) {
  draw_requested_ = false;
  cairo_t* cr = delegate_->AllocateCairo();
  if (!cr)
    return;
  DrawRect(cr, Bounds());
  if (delegate_->FlushCairo([this] (int32_t result) { FlushComplete(); }))
    flush_in_progress_ = true;
}

void RootView::FlushComplete() {
  flush_in_progress_ = false;
  if (draw_requested_)
    HandleDrawRequest(0);
}

void RootView::Resize(const Size& size) {
  printf("root view resize to: %f %f\n", size.width_, size.height_);
  View::Resize(size);
  SetNeedsDisplay();
}

void RootView::HandlePepperInputEvent(const pp::InputEvent& event,
                                      float scale) {
  switch (event.GetType()) {
    case PP_INPUTEVENT_TYPE_MOUSEDOWN: {
      pp::MouseInputEvent mouse_evt(event);
      pp::Point mouse_pos = mouse_evt.GetPosition();
      MouseInputEvent evt(Point(mouse_pos.x() * scale,
                                mouse_pos.y() * scale),
                          MouseInputEvent::DOWN,
                          mouse_evt.GetClickCount());
      down_mouse_handler_ = OnMouseDown(evt);
      return;
    }
    case PP_INPUTEVENT_TYPE_MOUSEUP: {
      if (!down_mouse_handler_)
        return;
      pp::MouseInputEvent mouse_evt(event);
      pp::Point mouse_pos = mouse_evt.GetPosition();
      MouseInputEvent evt(Point(mouse_pos.x() * scale,
                                mouse_pos.y() * scale),
                          MouseInputEvent::UP,
                          mouse_evt.GetClickCount());
      evt.UpdateToSubview(down_mouse_handler_, this);
      down_mouse_handler_->OnMouseUp(evt);
      return;
    }
    case PP_INPUTEVENT_TYPE_MOUSEMOVE: {
      pp::MouseInputEvent mouse_evt(event);
      pp::Point mouse_pos = mouse_evt.GetPosition();
      if (mouse_evt.GetButton() == PP_INPUTEVENT_MOUSEBUTTON_LEFT) {
        if (!down_mouse_handler_)
          return;
        MouseInputEvent evt(Point(mouse_pos.x() * scale,
                                  mouse_pos.y() * scale),
                            MouseInputEvent::DRAG,
                            mouse_evt.GetClickCount());
        evt.UpdateToSubview(down_mouse_handler_, this);
        down_mouse_handler_->OnMouseDrag(evt);
      } else {
        MouseInputEvent evt(Point(mouse_pos.x(), mouse_pos.y()),
                            MouseInputEvent::MOVE,
                            mouse_evt.GetClickCount());
        OnMouseMove(evt);
      }
      return;
    }
    case PP_INPUTEVENT_TYPE_CHAR: {
      pp::KeyboardInputEvent key_evt(event);
      KeyboardInputEvent evt(KeyboardInputEvent::TEXT,
                             key_evt.GetCharacterText().AsString(),
                             key_evt.GetModifiers() &
                             KeyboardInputEvent::kModifiersMask);
      OnKeyText(evt);
      return;
    }
    case PP_INPUTEVENT_TYPE_KEYDOWN: {
      pp::KeyboardInputEvent key_evt(event);
      KeyboardInputEvent evt(KeyboardInputEvent::DOWN,
                             key_evt.GetKeyCode(),
                             key_evt.GetModifiers() &
                             KeyboardInputEvent::kModifiersMask);
      OnKeyDown(evt);
      return;
    }
    case PP_INPUTEVENT_TYPE_KEYUP: {
      pp::KeyboardInputEvent key_evt(event);
      KeyboardInputEvent evt(KeyboardInputEvent::UP,
                             key_evt.GetKeyCode(),
                             key_evt.GetModifiers() &
                             KeyboardInputEvent::kModifiersMask);
      OnKeyUp(evt);
      return;
    }
    case PP_INPUTEVENT_TYPE_WHEEL: {
      pp::WheelInputEvent wheel_evt(event);
      ScrollInputEvent evt(wheel_evt.GetDelta().x() * scale,
                           wheel_evt.GetDelta().y() * scale);
      OnScrollEvent(evt);
      return;
    }
    default:
      return;
  }
}

}  // namespace pdfsketch
