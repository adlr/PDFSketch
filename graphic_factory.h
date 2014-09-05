// Copyright...

#ifndef PDFSKETCH_GRAPHIC_FACTORY_H__
#define PDFSKETCH_GRAPHIC_FACTORY_H__

#include "document.pb.h"
#include "graphic.h"
#include "toolbox.h"

namespace pdfsketch {

// The graphic factory is responsible for creating graphics.
// Generally, users of Graphic objects shouldn't care which specific
// type they are, but when creating a graphic, you do need to know the
// type. We put that functionality here.

class GraphicFactory {
 public:
  static std::shared_ptr<Graphic> NewGraphic(
      const Toolbox::Tool& type);
  static std::shared_ptr<Graphic> NewGraphic(
      const pdfsketchproto::Graphic& msg);
  static std::shared_ptr<Graphic> NewText(
      const std::string& str);
  static std::shared_ptr<Graphic> NewImage(
      const char* data, size_t length);
};

}  // namespace pdfsketch

#endif  // PDFSKETCH_GRAPHIC_FACTORY_H__
