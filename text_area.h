// Copyright...

#ifndef PDFKSETCH_TEXT_AREA_H__
#define PDFKSETCH_TEXT_AREA_H__

#include "graphic.h"

namespace pdfsketch {

// Text responsibilities:

// text_ - the text to show (saved)

// Graphic when not editing: paint text to screen
// Prerendered: just draw prerendered PDF
//   graphic_

// Non-prerendered: use fonts to draw text at precomputed locations
//   left_edges_
//   This will get more complex as we do more than just have new lines

// Graphic when resizing: reflow text
// Prerendered: rerender the graphic. Generates graphic_
// Non-prerendered: generates left edges

// Graphic when editing: handle keyboard/mouse input to make edits,
// redraw portions (while reflowing).

// class TextDraw {
//   graphic_ Prerender(string text);
//   Update(offset);
//   line_state_;
// };

// class TextEdit {
//   HandleKeyboard/Mouse();
//   Draw();
//   cursor_location_, virtual_cursor_location_;
//   selection_;
// };

class TextAreaTransformUndoOp;

class TextArea : public Graphic {
 public:
  TextArea() {
    frame_.size_.width_ = 150.0;
  }
  TextArea(const pdfsketchproto::Graphic& msg)
      : Graphic(msg) {
    frame_.size_.width_ = 150.0;
  }
  virtual void Restore(const pdfsketchproto::Graphic& msg) {
    Graphic::Restore(msg);
    text_ = msg.text_area().text();
  }
  virtual void Serialize(pdfsketchproto::Graphic* out) const;
  virtual void Place(int page, const Point& location);
  virtual void PlaceUpdate(const Point& location);
  virtual bool PlaceComplete();

  virtual bool Editable() const { return true; }
  virtual void BeginEditing(UndoManager* undo_manager);
  virtual void EndEditing();
  virtual void OnKeyText(const KeyboardInputEvent& event);
  virtual void OnKeyDown(const KeyboardInputEvent& event);
  virtual void OnKeyUp(const KeyboardInputEvent& event);

  virtual bool OnPaste(const std::string& str);

  virtual bool OnMouseDown(const Point& position);
  virtual void OnMouseDrag(const Point& position);
  virtual void OnMouseUp();

  size_t GetNewCursorPositionForHomeEnd(bool is_home) const;
  size_t GetNewCursorPositionForUpDownArrow(bool is_up) const;
  size_t GetNewCursorPositionForLeftRightArrow(
      bool is_left, bool shift_down, bool control_down) const;

  void set_text(const std::string& str) {
    text_ = str;
  }

  virtual void Draw(cairo_t* cr, bool selected);

  void ApplyUndoOp(const TextAreaTransformUndoOp& op,
                   UndoManager* undo_manager);

 protected:
  virtual int Knobs() const { return kKnobMiddleLeft | kKnobMiddleRight; }

 private:
  void UpdateLeftEdges(cairo_t* cr);
  std::string GetLine(size_t start_idx, size_t* out_advance);
  std::string DebugLeftEdges();
  size_t GetRowIndex(size_t index) const;

  size_t CursorPos() const {
    return (cursor_side_ == kLeft) ? selection_start_
        : selection_start_ + selection_size_;
  }
  // returns other side of selection
  size_t NonCursorPos() const {
    return (cursor_side_ == kRight) ? selection_start_
        : selection_start_ + selection_size_;
  }

  void SetSelection(size_t non_cursor_pos, size_t cursor_pos);
  void EraseSelection();

  size_t IndexForRowAndOffset(size_t row, double x_offset) const;
  size_t IndexForPoint(const Point& point) const;

  // Left edges is one longer than text_
  std::string text_;
  std::vector<double> left_edges_;
  // These cursor positions are the start of new rows.
  // Index 0 is implied, not stored in new_row_indexes_.
  std::vector<size_t> new_row_indexes_;

  // For use while editing
  UndoManager* undo_manager_{nullptr};

  size_t selection_start_{0};
  size_t selection_size_{0};

  // If the cursor is on the left or right end of the selection.
  enum SelectionSide { kLeft, kRight };
  SelectionSide cursor_side_{kLeft};

  // When moving cursor up/down, the ideal x offset of the cursor.
  // If negative, it's unset.
  double cursor_x_{-1.0};

  bool alt_down_{false};
};

class TextAreaTransformUndoOp : public UndoOp {
 public:
  TextAreaTransformUndoOp(TextArea* text_area,
                          size_t remove_start,
                          size_t remove_size,
                          std::string insert,
                          size_t final_selection_start,
                          size_t final_selection_size)
      : text_area_(text_area),
        remove_start_(remove_start),
        remove_size_(remove_size),
        insert_(insert),
        final_selection_start_(final_selection_start),
        final_selection_size_(final_selection_size) {}
  virtual ~TextAreaTransformUndoOp() {}
  // On Exec(), removes characters in remove_*_ range, inserts insert_
  // into that space, and finally sets the selection to the
  // final_selection_*_ values.
  virtual void Exec(UndoManager* manager) override;
  virtual UndoType Type() const override { return TextAreaTransform; }
  virtual bool Merge(const UndoOp& that) override;

  std::string String() const;
  size_t remove_start() const { return remove_start_; }
  size_t remove_size() const { return remove_size_; }
  const std::string& insert() const { return insert_; }
  size_t final_selection_start() const { return final_selection_start_; }
  size_t final_selection_size() const { return final_selection_size_; }

 private:
  TextArea* text_area_{nullptr};
  size_t remove_start_{0};
  size_t remove_size_{0};
  std::string insert_;
  size_t final_selection_start_{0};
  size_t final_selection_size_{0};
};

}  // namespace pdfsketch

#endif  // PDFKSETCH_TEXT_AREA_H__
