// Copyright...

#ifndef PDFSKETCH_GRAPHIC_H__
#define PDFSKETCH_GRAPHIC_H__

#include <cairo.h>

#include "view.h"

namespace pdfsketch {

class Color {
 public:
  Color()
      : red_(0.0), green_(0.0), blue_(0.0), alpha_(1.0) {}  // opaque black
  Color(double red, double green, double blue, double alpha)
      : red_(red), green_(green), blue_(blue), alpha_(alpha) {}
  void CairoSetSourceRGBA(cairo_t* cr) const {
    cairo_set_source_rgba(cr, red_, green_, blue_, alpha_);
  }
  double red_, green_, blue_, alpha_;
};

class Graphic {
 public:
  Graphic()
      : frame_(30.0, 30.0, 10.0, 20.0),
        page_(1),
        fill_color_(0.0, 1.0, 0.0, 0.3),
        line_width_(1.0),
        upper_sibling_(NULL),
        lower_sibling_(NULL) {}
  virtual void Draw(cairo_t* cr) {}
  int Page() const { return page_; }

  Rect frame_;  // location in page
  int page_;
  Color fill_color_;
  Color stroke_color_;
  double line_width_;

  Graphic* upper_sibling_;
  Graphic* lower_sibling_;
};

}  // namespace pdfsketch

#endif  // PDFSKETCH_GRAPHIC_H__
