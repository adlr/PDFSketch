// Copyright...

#ifndef PDFSKETCH_SQUIGGLE_H__
#define PDFSKETCH_SQUIGGLE_H__

#include "graphic.h"

namespace pdfsketch {

class Squiggle : public Graphic {
 public:
  Squiggle() {}
  explicit Squiggle(const pdfsketchproto::Graphic& msg)
      : Graphic(msg) {}
  virtual void Serialize(pdfsketchproto::Graphic* out) const;
  virtual void Place(int page, const Point& location, bool constrain);
  virtual void PlaceUpdate(const Point& location, bool constrain);
  virtual bool PlaceComplete();
  virtual void Draw(cairo_t* cr, bool selected);

 private:
  std::vector<Point> points_;

  // original_origin_ combined with natural_size_ form a rectangle
  // that represents where the original points came down. These points
  // are stored in points_, and during draw, the path is transformed
  // to the current frame.
  Point original_origin_;
};

}  // namespace pdfsketch

#endif  // PDFSKETCH_SQUIGGLE_H__
