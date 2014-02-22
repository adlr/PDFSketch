// Copyright...

#ifndef PDFSKETCH_CIRCLE_H__
#define PDFSKETCH_CIRCLE_H__

#include "graphic.h"

namespace pdfsketch {

class Circle : public Graphic {
 public:
  Circle() {}
  explicit Circle(const pdfsketchproto::Graphic& msg)
      : Graphic(msg) {}
  virtual void Serialize(pdfsketchproto::Graphic* out) const;
  virtual void Draw(cairo_t* cr, bool selected);
};

}  // namespace pdfsketch

#endif  // PDFSKETCH_CIRCLE_H__
