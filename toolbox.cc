// Copyright...

#include "toolbox.h"

#include "checkmark.h"
#include "rectangle.h"
#include "text_area.h"

using std::string;

namespace pdfsketch {

Graphic* Toolbox::NewGraphic() {
  switch (current_) {
    case ARROW:
      return NULL;
    case TEXT:
      return new TextArea();
    case CIRCLE:
      return new Rectangle();
    case RECTANGLE:
      return new Rectangle();
    case SQUIGGLE:
      return new Rectangle();
    case CHECKMARK:
      return new Checkmark();
  }
  printf("Toolbox::NewGraphic: should not get to here\n");
  return NULL;
}

void Toolbox::SelectTool(const std::string& tool) {
  printf("select tool: %s\n", tool.c_str());
  Tool new_tool = ARROW;
  if (tool == "Arrow") {
    new_tool = ARROW;
  } else if (tool == "Text") {
    new_tool = TEXT;
  } else if (tool == "Circle") {
    new_tool = CIRCLE;
  } else if (tool == "Rectangle") {
    new_tool = RECTANGLE;
  } else if (tool == "Squiggle") {
    new_tool = SQUIGGLE;
  } else if (tool == "Checkmark") {
    new_tool = CHECKMARK;
  } else {
    printf("Unknown tool: %s\n", tool.c_str());
    return;
  }
  SelectTool(new_tool);
}

void Toolbox::SelectTool(Tool tool) {
  current_ = tool;
  if (delegate_) {
    delegate_->ToolSelected(current_);
  }
}

string Toolbox::ToolAsString(Toolbox::Tool tool) {
  switch (tool) {
    case ARROW: return "Arrow";
    case TEXT: return "Text";
    case CIRCLE: return "Circle";
    case RECTANGLE: return "Rectangle";
    case SQUIGGLE: return "Squiggle";
    case CHECKMARK: return "Checkmark";
  }
  return "UnknownTool";
}

}  // namespace pdfsketch
