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
      const Toolbox::Tool& type,
      const Color& stroke_color);
  static std::shared_ptr<Graphic> NewGraphic(
      const pdfsketchproto::Graphic& msg);
};

}  // namespace pdfsketch

#endif  // PDFSKETCH_GRAPHIC_FACTORY_H__
