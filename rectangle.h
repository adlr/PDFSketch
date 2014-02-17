// Copyright...

#ifndef PDFSKETCH_RECTANGLE_H__
#define PDFSKETCH_RECTANGLE_H__

#include "graphic.h"

namespace pdfsketch {

class Rectangle : public Graphic {
 public:
  ~Rectangle();
  virtual void Draw(cairo_t* cr, bool selected);
};

}  // namespace pdfsketch

#endif  // PDFSKETCH_RECTANGLE_H__
