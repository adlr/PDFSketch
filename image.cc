// Copyright...

#include "image.h"

#include <cairo.h>

namespace pdfsketch {

struct ImageReadData {
  const char* data_;
  size_t len_;
};

cairo_status_t ImageRead(void* closure,
                         unsigned char *data,
                         unsigned int length) {
  ImageReadData* source = static_cast<ImageReadData*>(closure);
  if (length > source->len_)
    return CAIRO_STATUS_READ_ERROR;
  memcpy(data, source->data_, length);
  source->data_ += length;
  source->len_ -= length;
  return CAIRO_STATUS_SUCCESS;
}

Image::Image(const char* data, size_t len)
    : data_(data, data + len) {
  InitSurface();
}

Image::Image(const pdfsketchproto::Graphic& msg)
    : Graphic(msg),
      data_(msg.image().data().begin(), msg.image().data().end()) {
  InitSurface();
}

void Image::InitSurface() {
  ImageReadData source = {&data_[0], data_.size()};
  surface_ = cairo_image_surface_create_from_png_stream(ImageRead, &source);
  if (!surface_) {
    printf("NULL surface\n");
  } else {
    switch (cairo_surface_status(surface_)) {
#define ERRSTR(x) case x : printf("image load err: %s\n", #x ); break;
      case 0: break;  // Success
      ERRSTR(CAIRO_STATUS_NO_MEMORY);
      ERRSTR(CAIRO_STATUS_READ_ERROR);
      default: printf("err: %d\n", cairo_surface_status(surface_));
    }
  }
  natural_size_ = Size(cairo_image_surface_get_width(surface_),
                       cairo_image_surface_get_height(surface_));
  frame_.size_ = natural_size_;
  // Scale to fit in an allowable square
  double max_len = std::max(frame_.size_.width_, frame_.size_.height_);
  double max_allowed = 500.0;
  if (max_len > max_allowed) {
    frame_.size_.height_ *= max_allowed / max_len;
    frame_.size_.width_ *= max_allowed / max_len;
  }
}

Image::~Image() {
  if (surface_) {
    cairo_surface_finish(surface_);
    cairo_surface_destroy(surface_);
    surface_ = nullptr;
  }
}

void Image::Serialize(pdfsketchproto::Graphic* out) const {
  Graphic::Serialize(out);
  out->set_type(pdfsketchproto::Graphic::IMAGE);
  pdfsketchproto::Image* msg = out->mutable_image();
  msg->mutable_data()->assign(data_.begin(), data_.end());
}

void Image::Draw(cairo_t* cr, bool selected) {
  cairo_save(cr);
  cairo_translate(cr, frame_.Left(), frame_.Top());
  cairo_scale(cr, frame_.size_.width_ / cairo_image_surface_get_width(surface_),
              frame_.size_.height_ / cairo_image_surface_get_height(surface_));
  cairo_set_source_surface(cr, surface_, 0, 0);
  cairo_paint(cr);
  cairo_restore(cr);
}

}  // namespace pdfsketch
