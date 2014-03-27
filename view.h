// Copyright stuff

#ifndef PDFSKETCH_VIEW_H__
#define PDFSKETCH_VIEW_H__

#include <math.h>
#include <stdio.h>
#include <string>
#include <vector>

#include <cairo.h>

#include "document.pb.h"

namespace pdfsketch {

struct Point {
  Point() : x_(0.0), y_(0.0) {}
  Point(double x, double y) : x_(x), y_(y) {}
  explicit Point(const pdfsketchproto::Point& msg)
      : x_(msg.x()), y_(msg.y()) {}
  void Serialize(pdfsketchproto::Point* out) const {
    out->set_x(x_);
    out->set_y(y_);
  }
  Point TranslatedBy(double x, double y) const {
    return Point(x_ + x, y_ + y);
  }
  Point Add(const Point& that) const {
    return TranslatedBy(that.x_, that.y_);
  }
  Point Subtract(const Point& that) const {
    return Point(x_ - that.x_, y_ - that.y_);
  }
  Point ScaledBy(double factor) const {
    return Point(x_ * factor, y_ * factor);
  }
  Point Rounded() const {
    return Point(round(x_), round(y_));
  }
  void CairoMoveTo(cairo_t* cr) const {
    cairo_move_to(cr, x_, y_);
  }
  void CairoLineTo(cairo_t* cr) const {
    cairo_line_to(cr, x_, y_);
  }
  std::string String() const {
    char buf[100];
    int rc = snprintf(buf, sizeof(buf), "%f,%f", x_, y_);
    if (rc < 0 || rc == sizeof(buf))
      return "(err)";
    return buf;
  }
  bool operator==(const Point& that) const {
    return x_ == that.x_ && y_ == that.y_;
  }
  bool operator!=(const Point& that) const {
    return !(*this == that);
  }
  double x_, y_;
};

struct Size {
  Size() : width_(0.0), height_(0.0) {}
  Size(double w, double h) : width_(w), height_(h) {}
  explicit Size(const pdfsketchproto::Size& msg)
      : width_(msg.width()), height_(msg.height()) {}
  void Serialize(pdfsketchproto::Size* out) const {
    out->set_width(width_);
    out->set_height(height_);
  }
  Size ScaledBy(double scale) const {
    return Size(width_ * scale, height_ * scale);
  }
  Size RoundedUp() const {
    return Size(ceil(width_), ceil(height_));
  }
  std::string String() const {
    char buf[100];
    int rc = snprintf(buf, sizeof(buf), "%f,%f", width_, height_);
    if (rc < 0 || rc == sizeof(buf))
      return "(err)";
    return buf;
  }
  bool operator==(const Size& that) const {
    return width_ == that.width_ && height_ == that.height_;
  }
  bool operator!=(const Size& that) const {
    return !(*this == that);
  }
  double width_, height_;
};

struct Rect {
  Rect() {}
  Rect(double x, double y, double width, double height)
      : origin_(x, y), size_(width, height) {}
  explicit Rect(const Point& origin)
      : origin_(origin) {}
  explicit Rect(const Size& size)
      : size_(size) {}
  Rect(const Point& origin, const Size& size)
      : origin_(origin), size_(size) {}
  Rect(const Point& upper_left, const Point& lower_right)
      : origin_(upper_left),
        size_(lower_right.x_ - upper_left.x_,
              lower_right.y_ - upper_left.y_) {}
  explicit Rect(const pdfsketchproto::Rect& msg)
      : origin_(msg.origin()), size_(msg.size()) {}
  void Serialize(pdfsketchproto::Rect* out) const {
    origin_.Serialize(out->mutable_origin());
    size_.Serialize(out->mutable_size());
  }
  Rect Intersect(const Rect& that) const;
  bool Intersects(const Rect& that) const;
  bool Contains(const Point& point) const;
  Rect TranslatedBy(double x, double y) const {
    return Rect(origin_.TranslatedBy(x, y), size_);
  }
  Rect ScaledBy(double scale) const {
    return Rect(origin_, size_.ScaledBy(scale));
  }
  Rect InsetBy(double inset) const {
    return Rect(origin_.x_ + inset,
                origin_.y_ + inset,
                size_.width_ - 2.0 * inset,
                size_.height_ - 2.0 * inset);
  }
  double Top() const { return origin_.y_; }
  double Bottom() const { return origin_.y_ + size_.height_; }
  double Left() const { return origin_.x_; }
  double Right() const { return origin_.x_ + size_.width_; }
  Point UpperLeft() const { return origin_; }
  Point UpperRight() const { return Point(Right(), Top()); }
  Point LowerLeft() const { return Point(Left(), Bottom()); }
  Point LowerRight() const { return Point(Right(), Bottom()); }
  Point Center() const {
    return Point(origin_.x_ + 0.5 * size_.width_,
                 origin_.y_ + 0.5 * size_.height_);
  }
  
  // These Set*Abs return true if flipped
  bool SetTopAbs(double top);
  bool SetRightAbs(double right);
  bool SetLeftAbs(double left);
  bool SetBottomAbs(double bottom);
  
  void SetCenter(const Point& location) {
    origin_.x_ = location.x_ - size_.width_ * 0.5;
    origin_.y_ = location.y_ - size_.height_ * 0.5;
  }

  std::string String() const {
    char buf[100];
    snprintf(buf, sizeof(buf), "[%f,%f,%f,%f]",
             origin_.x_, origin_.y_, size_.width_, size_.height_);
    return std::string(buf);
  }
  void CairoRectangle(cairo_t* cr) const {
    cairo_rectangle(cr, origin_.x_, origin_.y_, size_.width_, size_.height_);
  }
  bool operator==(const Rect& that) const {
    return origin_ == that.origin_ && size_ == that.size_;
  }
  bool operator!=(const Rect& that) const {
    return !(*this == that);
  }
  Point origin_;
  Size size_;
};

class View;

class MouseInputEvent {
 public:
  enum Type {
    DOWN, DRAG, UP, MOVE
  };
  MouseInputEvent(Point position, Type type,
                  int32_t click_count)
      : position_(position),
        type_(type),
        click_count_(click_count) {}
  void UpdateToSubview(View* subview, View* from_superview);
  void UpdateFromSubview(View* subview);
  const Point& position() const { return position_; }
  int32_t ClickCount() const { return click_count_; }

 private:
  Point position_;
  Type type_;
  int32_t click_count_;
};

class ScrollInputEvent {
 public:
  ScrollInputEvent(double dx, double dy)
      : dx_(dx), dy_(dy) {}
  void UpdateToSubview(View* subview, View* from_superview);
  double dx() const { return dx_; }
  double dy() const { return dy_; }

 private:
  double dx_, dy_;
};

class KeyboardInputEvent {
 public:
  enum Type {
    TEXT, DOWN, UP
  };
  static const uint32_t kShift   = 1 << 0;
  static const uint32_t kControl = 1 << 1;
  static const uint32_t kAlt     = 1 << 2;
  static const uint32_t kMeta    = 1 << 3;
  static const uint32_t kShortcutMask = kControl | kAlt | kMeta;
  static const uint32_t kModifiersMask = 0xf;
  KeyboardInputEvent(Type type, uint32_t keycode, uint32_t modifiers)
      : type_(type), keycode_(keycode), modifiers_(modifiers) {}
  KeyboardInputEvent(Type type, std::string text, uint32_t modifiers)
      : type_(type), keycode_(0), text_(text), modifiers_(modifiers) {}
  Type type() const { return type_; }
  uint32_t keycode() const { return keycode_; }
  const std::string& text() const { return text_; }
  uint32_t modifiers() const { return modifiers_; }

 private:
  Type type_;
  uint32_t keycode_;
  std::string text_;
  uint32_t modifiers_;
};

class ViewDelegate {
 public:
  virtual void ViewFrameChanged(View* view, const Rect& frame) = 0;
};

class View {
 public:
  View()
      : scale_(1.0),
        top_fixed_to_top_(true),
        bot_fixed_to_top_(true),
        left_fixed_to_left_(true),
        right_fixed_to_left_(true),
        top_child_(NULL),
        bottom_child_(NULL),
        parent_(NULL),
        lower_sibling_(NULL),
        upper_sibling_(NULL),
        delegate_(NULL) {}
  virtual ~View() {}
  virtual std::string Name() const { return "View"; }
  virtual void DrawRect(cairo_t* ctx, const Rect& rect);
  virtual void SetNeedsDisplayInRect(const Rect& rect);
  void SetNeedsDisplay() {
    SetNeedsDisplayInRect(Bounds());
  }
  View* Superview() const { return parent_; }
  void AddSubview(View* subview);
  void RemoveSubview(View* subview);
  Rect Bounds() const { return Rect(size_); }
  Size size() const { return size_; }
  Rect Frame() const {
    return Rect(origin_, size_.ScaledBy(scale_));
  }
  // Returns in view's coordinates the region that's visible. May
  // return a region that's larger than what's actually visible, e.g.,
  // if something is obscuring this view.
  Rect VisibleSubrect() const;
  virtual void Resize(const Size& size);  // Resizes subviews
  void SetResizeParams(bool top_fixed_to_top,
                       bool bot_fixed_to_top,
                       bool left_fixed_to_left,
                       bool right_fixed_to_left) {
    top_fixed_to_top_ = top_fixed_to_top;
    bot_fixed_to_top_ = bot_fixed_to_top;
    left_fixed_to_left_ = left_fixed_to_left;
    right_fixed_to_left_ = right_fixed_to_left;
  }

  void SetOrigin(const Point& origin) { origin_ = origin; }
  void SetFrame(const Rect& frame);
  void SetSize(const Size& size);  // does not resize subviews
  void SetScale(double scale);

  // Returns the consumer of the event, or NULL if none.
  // The consumer will get the rest of the drag.
  virtual View* OnMouseDown(const MouseInputEvent& event); // passes to subviews
  virtual void OnMouseDrag(const MouseInputEvent& event) {}
  virtual void OnMouseUp(const MouseInputEvent& event) {}
  virtual void OnScrollEvent(const ScrollInputEvent& event); // passes to subviews

  // Returns true if consumed
  virtual bool OnKeyText(const KeyboardInputEvent& event); // passes to subviews
  virtual bool OnKeyDown(const KeyboardInputEvent& event); // passes to subviews
  virtual bool OnKeyUp(const KeyboardInputEvent& event); // passes to subviews

  // Button is up here. Returns true if consumed.
  virtual bool OnMouseMove(const MouseInputEvent& event) { return true; }

  // You must pass in a direct subview of this.
  Point ConvertPointToSubview(const View& subview, Point point) const;
  Rect ConvertRectToSubview(const View& subview, const Rect& rect) const;
  Size ConvertSizeToSubview(const View& subview, const Size& size) const;

  Point ConvertPointFromSubview(const View& subview, const Point& point) const;
  Size ConvertSizeFromSubview(const View& subview, const Size& size) const;
  Rect ConvertRectFromSubview(const View& subview, const Rect& rect) const {
    return Rect(ConvertPointFromSubview(subview, rect.origin_),
                ConvertSizeFromSubview(subview, rect.size_));
  }

  void SetDelegate(ViewDelegate* delegate) {
    delegate_ = delegate;
  }

 protected:
  Point origin_;  // In parent's coordinate space
  Size size_;  // In client's coordinate space
  // Size in parent's point of view =
  // scale_ * size in child's point of view
  double scale_;

  // How resizing parent affects this view. Each edge of this view is a fixed
  // distance from one of the edges in the parent view. For example, this top
  // edge can be fixed to the parent's top or bottom edge.
  bool top_fixed_to_top_:1;  // This' top is fixed to parent's top.
  bool bot_fixed_to_top_:1;  // This' bot is fixed to parent's top.
  bool left_fixed_to_left_:1;  // etc...
  bool right_fixed_to_left_:1;

  // Weak references to other views, managed by this.
  View* top_child_;
  View* bottom_child_;

  // Weak references to sibling views, managed by parent.
  View* parent_;
  View* lower_sibling_;
  View* upper_sibling_;

  ViewDelegate* delegate_;
};

}  // namespace pdfsketch

#endif  // PDFSKETCH_VIEW_H__
