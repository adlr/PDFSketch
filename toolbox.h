// Copyright...

#ifndef PDFSKETCH_TOOLBOX_H__
#define PDFSKETCH_TOOLBOX_H__

#include "graphic.h"

namespace pdfsketch {

class ToolboxDelegate;

class Toolbox {
 public:
  enum Tool {
    ARROW, TEXT, CIRCLE, RECTANGLE, SQUIGGLE, CHECKMARK
  };
  Toolbox() : current_(ARROW), delegate_(NULL) {}
  void SetDelegate(ToolboxDelegate* delegate) {
    delegate_ = delegate;
  }
  static std::string ToolAsString(Toolbox::Tool tool);

  // Caller takes ownership of returned Graphic:
  Graphic* NewGraphic();
  void SelectTool(const std::string& tool);
  void SelectTool(Tool tool);
  Tool CurrentTool() const { return current_; }

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
