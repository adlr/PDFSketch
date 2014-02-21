// Copyright...

#ifndef PDFSKETCH_RECTANGLE_H__
#define PDFSKETCH_RECTANGLE_H__

#include "graphic.h"

namespace pdfsketch {

class Rectangle : public Graphic {
 public:
  Rectangle() {}
  explicit Rectangle(const pdfsketchproto::Graphic& msg)
      : Graphic(msg) {}
  ~Rectangle();
  virtual void Serialize(pdfsketchproto::Graphic* out) const;
  virtual void Draw(cairo_t* cr, bool selected);
};

}  // namespace pdfsketch

#endif  // PDFSKETCH_RECTANGLE_H__
