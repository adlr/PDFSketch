// Copyright stuff

#ifndef PDFSKETCH_VIEW_H__
#define PDFSKETCH_VIEW_H__

#include <math.h>
#include <stdio.h>
#include <string>
#include <vector>

#include <cairo.h>

namespace pdfsketch {

struct Point {
  Point() : x_(0.0), y_(0.0) {}
  Point(double x, double y) : x_(x), y_(y) {}
  Point TranslatedBy(double x, double y) const {
    return Point(x_ + x, y_ + y);
  }
  double x_, y_;
};

struct Size {
  Size() : width_(0.0), height_(0.0) {}
  Size(double w, double h) : width_(w), height_(h) {}
  Size ScaledBy(double scale) const {
    return Size(width_ * scale, height_ * scale);
  }
  Size RoundedUp() const {
    return Size(ceil(width_), ceil(height_));
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
  std::string String() const {
    char buf[100];
    snprintf(buf, sizeof(buf), "[%f,%f,%f,%f]",
             origin_.x_, origin_.y_, size_.width_, size_.height_);
    return std::string(buf);
  }
  void CairoRectangle(cairo_t* cr) const {
    cairo_rectangle(cr, origin_.x_, origin_.y_, size_.width_, size_.height_);
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
  MouseInputEvent(Point position, Type type)
      : position_(position), type_(type) {}
  void UpdateToSubview(View* subview);
  void UpdateFromSubview(View* subview);
  const Point& position() const { return position_; }

 private:
  Point position_;
  Type type_;
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
        upper_sibling_(NULL) {}
  virtual ~View() {}
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

  // Button is up here. Returns true if consumed.
  virtual bool OnMouseMove(const MouseInputEvent& event) {return true;}

  // You must pass in a direct subview of this.
  Point ConvertPointToSubview(const View& subview, Point point) const;

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
