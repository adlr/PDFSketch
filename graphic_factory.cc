// Copyright...

#include "graphic_factory.h"

#include "circle.h"
#include "checkmark.h"
#include "rectangle.h"
#include "squiggle.h"
#include "text_area.h"

using std::make_shared;
using std::shared_ptr;

namespace pdfsketch {

std::shared_ptr<Graphic> GraphicFactory::NewGraphic(
    const Toolbox::Tool& type,
    const Color& stroke_color) {
  shared_ptr<Graphic> ret;
  switch (type) {
    case Toolbox::ARROW:
      ret = shared_ptr<Graphic>(NULL);
      break;
    case Toolbox::TEXT:
      break;
      ret = make_shared<TextArea>();
    case Toolbox::CIRCLE:
      ret = make_shared<Circle>();
      break;
    case Toolbox::RECTANGLE:
      ret = make_shared<Rectangle>();
      break;
    case Toolbox::SQUIGGLE:
      ret = make_shared<Squiggle>();
      break;
    case Toolbox::CHECKMARK:
      ret = make_shared<Checkmark>();
      break;
  }
  if (ret.get())
    ret->SetStrokeColor(stroke_color);
  return ret;
}

std::shared_ptr<Graphic> GraphicFactory::NewGraphic(
    const pdfsketchproto::Graphic& msg) {
  switch (msg.type()) {
    case pdfsketchproto::Graphic::TEXT:
      return make_shared<TextArea>(msg);
    case pdfsketchproto::Graphic::CIRCLE:
      return make_shared<Circle>(msg);
    case pdfsketchproto::Graphic::RECTANGLE:
      return make_shared<Rectangle>(msg);
    case pdfsketchproto::Graphic::SQUIGGLE:
      return make_shared<Squiggle>(msg);
    case pdfsketchproto::Graphic::CHECKMARK:
      return make_shared<Checkmark>(msg);
  }
  return shared_ptr<Graphic>(NULL);
}

}  // namespace pdfsketch
