// Copyright...

#include "graphic_factory.h"

#include "circle.h"
#include "checkmark.h"
#include "image.h"
#include "rectangle.h"
#include "squiggle.h"
#include "text_area.h"

using std::make_shared;
using std::shared_ptr;
using std::string;

namespace pdfsketch {

std::shared_ptr<Graphic> GraphicFactory::NewGraphic(
    const Toolbox::Tool& type) {
  switch (type) {
    case Toolbox::ARROW:
      return shared_ptr<Graphic>(NULL);
    case Toolbox::TEXT:
      return make_shared<TextArea>();
    case Toolbox::CIRCLE:
      return make_shared<Circle>();
    case Toolbox::RECTANGLE:
      return make_shared<Rectangle>();
    case Toolbox::SQUIGGLE:
      return make_shared<Squiggle>();
    case Toolbox::CHECKMARK:
      return make_shared<Checkmark>();
  }
  printf("%s: should not get to here\n", __func__);
  return shared_ptr<Graphic>(NULL);
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
    case pdfsketchproto::Graphic::IMAGE:
      return make_shared<Image>(msg);
  }
  return shared_ptr<Graphic>(NULL);
}

std::shared_ptr<Graphic> GraphicFactory::NewText(
    const string& str) {
  std::shared_ptr<TextArea> ret(make_shared<TextArea>());
  ret->set_text(str);
  return ret;
}

std::shared_ptr<Graphic> GraphicFactory::NewImage(
    const char* data, size_t length) {
  return make_shared<Image>(data, length);
}

}  // namespace pdfsketch
