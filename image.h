// Copyright...

#ifndef PDFSKETCH_IMAGE_H__
#define PDFSKETCH_IMAGE_H__

#include <vector>

#include "graphic.h"

namespace pdfsketch {

// Class that represents an image. Currently only supports PNG.

class Image : public Graphic {
 public:
  Image(const char* data, size_t len);
  explicit Image(const pdfsketchproto::Graphic& msg);
  void InitSurface();
  ~Image();
  virtual void Serialize(pdfsketchproto::Graphic* out) const;
  virtual void Draw(cairo_t* cr, bool selected);
 private:
  std::vector<char> data_;
  cairo_surface_t* surface_{nullptr};
};

}  // namespace pdfsketch

#endif  // PDFSKETCH_IMAGE_H__
