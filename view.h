// Copyright stuff

#ifndef PDFSKETCH_VIEW_H__
#define PDFSKETCH_VIEW_H__

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
  Rect TranslatedBy(double x, double y) const {
    return Rect(origin_.TranslatedBy(x, y), size_);
  }
  Rect ScaledBy(double scale) const {
    return Rect(origin_, size_.ScaledBy(scale));
  }
  Point origin_;
  Size size_;
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
  void AddSubview(View* subview);
  Rect Bounds() const { return Rect(size_); }
  Rect Frame() const {
    return Rect(origin_, size_.ScaledBy(scale_));
  }
  virtual void Resize(const Size& size);
  void SetFrame(const Rect& frame);

  // You must pass in a direct subview of this.
  Point ConvertPointFromSubview(const View& subview, const Point& point);
  Size ConvertSizeFromSubview(const View& subview, const Size& size);
  Rect ConvertRectFromSubview(const View& subview, const Rect& rect) {
    return Rect(ConvertPointFromSubview(subview, rect.origin_),
                ConvertSizeFromSubview(subview, rect.size_));
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
};

}  // namespace pdfsketch

#endif  // PDFSKETCH_VIEW_H__
