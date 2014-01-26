// Copyright...

#ifndef PDFSKETCH_RECTANGLE_H__
#define PDFSKETCH_RECTANGLE_H__

#include "graphic.h"

namespace pdfsketch {

class Rectangle : public Graphic {
 public:
  virtual void Draw(cairo_t* cr);
};

}  // namespace pdfsketch

#endif  // PDFSKETCH_RECTANGLE_H__
