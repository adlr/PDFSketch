// Copyright...

#ifndef PDFKSETCH_TEXT_AREA_H__
#define PDFKSETCH_TEXT_AREA_H__

#include "graphic.h"

namespace pdfsketch {

class TextArea : public Graphic {
 public:
  TextArea()
      : selection_start_(0),
        selection_size_(0) {}
  virtual void Place(int page, const Point& location, bool constrain);
  virtual void PlaceUpdate(const Point& location, bool constrain);
  virtual bool PlaceComplete();

  virtual bool Editable() const { return true; }
  virtual void BeginEditing();
  virtual void EndEditing();
  virtual void OnKeyText(const KeyboardInputEvent& event);
  virtual void OnKeyDown(const KeyboardInputEvent& event);
  virtual void OnKeyUp(const KeyboardInputEvent& event);

  // virtual void BeginResize(const Point& location, int knob, bool constrain);
  // virtual void UpdateResize(const Point& location, bool constrain);
  // virtual void EndResize();

  virtual void Draw(cairo_t* cr);

 protected:
  virtual int Knobs() const { return kKnobMiddleLeft | kKnobMiddleRight; }

 private:
  void UpdateLeftEdges(cairo_t* cr);
  std::string GetLine(size_t start_idx);
  std::string DebugLeftEdges();
  size_t GetRowIndex(size_t index);

  void EraseSelection();

  // These two should have the same length:
  std::string text_;
  std::vector<double> left_edges_;

  size_t selection_start_;
  size_t selection_size_;
};

}  // namespace pdfsketch

#endif  // PDFKSETCH_TEXT_AREA_H__
