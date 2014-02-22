// Copyright...

#ifndef PDFSKETCH_CHECKMARK_H__
#define PDFSKETCH_CHECKMARK_H__

#include "graphic.h"

namespace pdfsketch {

class Checkmark : public Graphic {
 public:
  Checkmark() {}
  explicit Checkmark(const pdfsketchproto::Graphic& msg)
      : Graphic(msg) {}
  ~Checkmark() {}
  virtual void Serialize(pdfsketchproto::Graphic* out) const;
  virtual void Place(int page, const Point& location, bool constrain);
  virtual void PlaceUpdate(const Point& location, bool constrain);
  virtual bool PlaceComplete() { return false; }
  virtual void Draw(cairo_t* cr, bool selected);
};

}  // namespace pdfsketch

#endif  // PDFSKETCH_CHECKMARK_H__
