// Copyright stuff

#include <vector>

namespace pdfsketch {

struct Point {
  Point() : x_(0.0), y_(0.0) {}
  Point(double x, double y) : x_(x), y_(y) {}
  double X_, y_;
};

struct Size {
  Size() : width_(0.0), height_(0.0) {}
  Size(double w, double h) : width_(w), height_(h) {}
  double width_, height_;
};

class View {
 public:
  View() {}
  virtual ~View() {}

 private:
  Point origin_;  // In parent's coordinate space
  Size size_;  // In client's coordinate space
  // Size in parent's point of view =
  // scale_ * size in client's point of view
  double scale_;

  std::vector<View*> subviews_;
};

}  // namespace pdfsketch
