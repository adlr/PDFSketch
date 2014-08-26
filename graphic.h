// Copyright...

#ifndef PDFSKETCH_GRAPHIC_H__
#define PDFSKETCH_GRAPHIC_H__

#include <memory>

#include <cairo.h>

#include "undo_manager.h"
#include "view.h"

namespace pdfsketch {

class Color {
 public:
  Color()
      : red_(0.0), green_(0.0), blue_(0.0), alpha_(1.0) {}  // opaque black
  Color(double red, double green, double blue, double alpha)
      : red_(red), green_(green), blue_(blue), alpha_(alpha) {}
  explicit Color(const pdfsketchproto::Color& msg)
      : red_(msg.red()),
        green_(msg.green()),
        blue_(msg.blue()),
        alpha_(msg.alpha()) {}
  void Serialize(pdfsketchproto::Color* out) const {
    out->set_red(red_);
    out->set_green(green_);
    out->set_blue(blue_);
    out->set_alpha(alpha_);
  }
  void CairoSetSourceRGBA(cairo_t* cr) const {
    cairo_set_source_rgba(cr, red_, green_, blue_, alpha_);
  }
  double red_, green_, blue_, alpha_;
};

const int kKnobNone        = 0;
const int kKnobUpperLeft   = 1 << 0;
const int kKnobUpperMiddle = 1 << 1;
const int kKnobUpperRight  = 1 << 2;
const int kKnobMiddleLeft  = 1 << 3;
const int kKnobMiddleRight = 1 << 4;
const int kKnobLowerLeft   = 1 << 5;
const int kKnobLowerMiddle = 1 << 6;
const int kKnobLowerRight  = 1 << 7;
const int kAllKnobs = 0xff;

inline bool KnobIsCorner(int knob) {
  switch (knob) {
    case kKnobUpperLeft:  // fallthough
    case kKnobUpperRight:  // fallthough
    case kKnobLowerLeft:  // fallthough
    case kKnobLowerRight:
      return true;
  }
  return false;
}

class GraphicDelegate {
 public:
  virtual void SetNeedsDisplayInPageRect(int page, const Rect& rect) = 0;
  virtual Point ConvertPointFromGraphic(int page, const Point& point) = 0;
  virtual Point ConvertPointToGraphic(int page, const Point& point) = 0;
  virtual double GetZoom() = 0;
};

class Graphic {
 public:
  Graphic()
      : frame_(30.0, 30.0, 10.0, 20.0),
        natural_size_(1.0, 1.0),
        fill_color_(0.0, 0.0, 0.0, 0.0),
        h_flip_(false),
        v_flip_(false) {}
  Graphic(const pdfsketchproto::Graphic& msg) { Restore(msg); }
  virtual ~Graphic() {}

  virtual void Serialize(pdfsketchproto::Graphic* out) const;
  virtual void Restore(const pdfsketchproto::Graphic& msg) {
    frame_ = Rect(msg.frame());
    natural_size_ = msg.has_natural_size() ?
                  Size(msg.natural_size()) : Size(1.0, 1.0);
    page_ = msg.page();
    fill_color_ = Color(msg.fill_color());
    stroke_color_ = Color(msg.stroke_color());
    line_width_ = msg.line_width();
    h_flip_ = msg.h_flip();
    v_flip_ = msg.v_flip();
  }

  void SetDelegate(GraphicDelegate* delegate) {
    delegate_ = delegate;
  }

  // Placement
  virtual void Place(int page, const Point& location);
  virtual void PlaceUpdate(const Point& location);
  // Returns true if this graphic should be deleted.
  // By default, graphics with 0 size are deleted.
  virtual bool PlaceComplete();

  virtual bool Editable() const { return false; }
  virtual void BeginEditing(UndoManager* undo_manager) { editing_ = true; }
  virtual void EndEditing() { editing_ = false; }
  bool IsEditing() const { return editing_; }
  // These keyboard events arrive only when editing is in progress:
  virtual void OnKeyText(const KeyboardInputEvent& event) {}
  virtual void OnKeyDown(const KeyboardInputEvent& event) {}
  virtual void OnKeyUp(const KeyboardInputEvent& event) {}

  // Return true if handling this drag event. Only called when a graphic
  // is being edited.
  virtual bool OnMouseDown(const Point& position) { return false; }
  virtual void OnMouseDrag(const Point& position) {}
  virtual void OnMouseUp() {}

  // Return true if handled
  virtual bool OnPaste(const std::string& str) { return false; }

  virtual void BeginResize(const Point& location, int knob);
  virtual void UpdateResize(const Point& location);
  virtual void EndResize();

  virtual void Draw(cairo_t* cr, bool selected) {}
  void DrawKnobs(cairo_t* cr);
  int Page() const { return page_; }
  void SetPage(int page) { page_ = page; }

  // returns the knob hit, or kKnobNone if none.
  int PointInKnob(const Point& location) const;

  const Rect& Frame() const { return frame_; }
  void SetFrame(const Rect& frame) { frame_ = frame; }

  // Drawing frames are for the regions that need to be redrawn
  Rect DrawingFrame() const;
  Rect DrawingFrameWithKnobs() const;

  Rect KnobFrame(int knob) const;
  Rect DrawingKnobFrame(int knob) const;

  void SetNeedsDisplay(bool withKnobs) const;
  Rect frame_;  // location in page
  Size natural_size_;
  int page_{1};
  Color fill_color_;
  Color stroke_color_;
  double line_width_{1.0};

  // if should be drawn horizontall/vertically flipped
  bool h_flip_:1;
  bool v_flip_:1;

  // We only need one shared pointer here to keep these all "owned"
  // by the containing DocumentView.
  Graphic* upper_sibling_{nullptr};
  std::shared_ptr<Graphic> lower_sibling_;

 protected:
  virtual int Knobs() const { return kAllKnobs; }

 private:
  int resizing_knob_{kKnobNone};  // kKnobNone if no resize in progress
  GraphicDelegate* delegate_{nullptr};

  bool editing_{false};  // is being edited
};

}  // namespace pdfsketch

#endif  // PDFSKETCH_GRAPHIC_H__
