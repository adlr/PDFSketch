// Copyright...

#ifndef PDFKSETCH_TEXT_AREA_H__
#define PDFKSETCH_TEXT_AREA_H__

#include "graphic.h"

namespace pdfsketch {

class TextArea : public Graphic {
 public:
  virtual void Place(int page, const Point& location, bool constrain);
  virtual void PlaceUpdate(const Point& location, bool constrain);
  virtual bool PlaceComplete();

  // virtual void BeginResize(const Point& location, int knob, bool constrain);
  // virtual void UpdateResize(const Point& location, bool constrain);
  // virtual void EndResize();

  virtual void Draw(cairo_t* cr);

 protected:
  virtual int Knobs() const { return kKnobMiddleLeft | kKnobMiddleRight; }

 private:
  std::string text_;
};

}  // namespace pdfsketch

#endif  // PDFKSETCH_TEXT_AREA_H__
