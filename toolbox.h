// Copyright...

#ifndef PDFSKETCH_TOOLBOX_H__
#define PDFSKETCH_TOOLBOX_H__

#include "graphic.h"

namespace pdfksetch {

class ToolboxDelegate;

class Toolbox {
 public:
  enum Tool {
    ARROW, TEXT, CIRCLE, RECTANGLE, SQUIGGLE, CHECKMARK
  };
  void SetDelegate(ToolboxDelegate* delegate) {
    delegate_ = delegate;
  }

  // Caller takes ownership of returned Graphic:
  Graphic* NewGraphic();
  void SelectTool(const std::string& tool);
  void SelectTool(Tool tool);

 private:
  Tool current_;
  ToolboxDelegate* delegate_;
};

class ToolboxDelegate {
 public:
  virtual void ToolSelected(Toolbox::Tool tool) = 0;
};

}  // namespace pdfsketch

#endif  // PDFSKETCH_TOOLBOX_H__
