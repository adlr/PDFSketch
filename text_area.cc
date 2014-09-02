// Copyright...

#include "text_area.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <string>

using std::string;
using std::unique_ptr;
using std::vector;

namespace pdfsketch {

void Dbg(const char* str);

void TextArea::Serialize(pdfsketchproto::Graphic* out) const {
  Graphic::Serialize(out);
  out->set_type(pdfsketchproto::Graphic::TEXT);
  pdfsketchproto::TextArea* msg = out->mutable_text_area();
  *msg->mutable_text() = text_;
}

void TextArea::Place(int page, const Point& location) {
  page_ = page;
  PlaceUpdate(location);
}
void TextArea::PlaceUpdate(const Point& location) {
  frame_ = Rect(location, Size(150.0, 72.0));
}
bool TextArea::PlaceComplete() {
  return false;
}

void TextArea::BeginEditing(UndoManager* undo_manager) {
  Graphic::BeginEditing(undo_manager);
  undo_manager_ = undo_manager;
  undo_manager_->SetMarker();
  selection_start_ = text_.size();
  selection_size_ = 0;
}

void TextArea::EndEditing() {
  Graphic::EndEditing();
  undo_manager_->ClearFromMarker();
  undo_manager_ = nullptr;
}

void TextArea::OnKeyText(const KeyboardInputEvent& event) {
  if (event.modifiers() & KeyboardInputEvent::kShortcutMask)
    return;
  size_t old_sel_start = selection_start_;
  size_t old_sel_size = selection_size_;
  string old_selected_text = text_.substr(selection_start_,
                                          selection_size_);
  EraseSelection();
  const string* use = &event.text();
  const string newline("\n");
  if (!strcmp(event.text().c_str(), "\r"))
    use = &newline;
  text_.insert(selection_start_, *use);
  selection_start_ += event.text().size();
  cursor_x_ = -1.0;
  SetNeedsDisplay(false);

  unique_ptr<UndoOp> op(
      new TextAreaTransformUndoOp(this,
                                  selection_start_ - event.text().size(),
                                  event.text().size(),
                                  old_selected_text,
                                  old_sel_start,
                                  old_sel_size));
  undo_manager_->AddUndoOp(std::move(op));
}

void TextArea::OnKeyDown(const KeyboardInputEvent& event) {
  KeyboardInputEvent use_event = event;
  bool shift_down = event.modifiers() & KeyboardInputEvent::kShift;
  bool control_down =
      event.modifiers() & KeyboardInputEvent::kControl;
  bool alt_down =
      event.modifiers() & KeyboardInputEvent::kAlt;
  if (alt_down && event.keycode() == 18) {
    alt_down_ = true;
    return;
  }
  if (alt_down_ && !alt_down && event.keycode() == 33) {
    // switch to 38
    use_event = KeyboardInputEvent(event.type(),
                                   38,
                                   event.modifiers() |
                                   KeyboardInputEvent::kAlt);
    alt_down = true;
  }
  // Hack: Chrome OS replaces alt-up/down with page up/down.
  // We detect this and put alt-up/down back
  if (alt_down_ && !alt_down && event.keycode() == 34) {
    // switch to 40
    use_event = KeyboardInputEvent(event.type(),
                                   40,
                                   event.modifiers() |
                                   KeyboardInputEvent::kAlt);
    alt_down = true;
  }
  // Shift the frame
  if (alt_down && (use_event.keycode() == 37 ||
                   use_event.keycode() == 38 ||
                   use_event.keycode() == 39 ||
                   use_event.keycode() == 40)) {
    float dx = use_event.keycode() == 37 ? -1.0 :
        (use_event.keycode() == 39 ? 1.0 : 0.0);
    float dy = use_event.keycode() == 38 ? -1.0 :
        (use_event.keycode() == 40 ? 1.0 : 0.0);
    if (shift_down) {
      dx *= 10.0;
      dy *= 10.0;
    }
    SetNeedsDisplay(false);
    frame_ = frame_.TranslatedBy(dx, dy);
    SetNeedsDisplay(false);
    return;
  }
  if (use_event.keycode() == 8 || use_event.keycode() == 46) {
    // backspace, delete
    if (selection_size_ > 0) {
      string trimmed = text_.substr(selection_start_, selection_size_);
      unique_ptr<UndoOp> op(
          new TextAreaTransformUndoOp(this,
                                      selection_start_,
                                      0,
                                      trimmed,
                                      selection_start_,
                                      selection_size_));
      undo_manager_->AddUndoOp(std::move(op));
      EraseSelection();
    } else if (use_event.keycode() == 8) {
      // do backspace
      if (selection_start_ == 0)
        return;
      string trimmed = text_.substr(selection_start_ - 1, 1);
      text_.erase(text_.begin() + selection_start_ - 1);
      selection_start_--;

      unique_ptr<UndoOp> op(
          new TextAreaTransformUndoOp(this,
                                      selection_start_,
                                      0,
                                      trimmed,
                                      selection_start_ + 1,
                                      0));
      undo_manager_->AddUndoOp(std::move(op));
    } else {
      // do delete
      if (selection_start_ >= text_.size())
        return;
      string trimmed = text_.substr(selection_start_, 1);
      text_.erase(text_.begin() + selection_start_);

      unique_ptr<UndoOp> op(
          new TextAreaTransformUndoOp(this,
                                      selection_start_,
                                      0,
                                      trimmed,
                                      selection_start_,
                                      0));
      undo_manager_->AddUndoOp(std::move(op));
    }
  }
  if (use_event.keycode() == 36 || use_event.keycode() == 35 ||
      use_event.keycode() == 38 || use_event.keycode() == 40 ||
      use_event.keycode() == 37 || use_event.keycode() == 39) {
    size_t new_cursor = 0;
    if (use_event.keycode() == 36 || use_event.keycode() == 35) {
      new_cursor = GetNewCursorPositionForHomeEnd(
          use_event.keycode() == 36);
    }
    if (use_event.keycode() == 38 || use_event.keycode() == 40) {
      new_cursor = GetNewCursorPositionForUpDownArrow(
          use_event.keycode() == 38);
      cursor_x_ = left_edges_[new_cursor];
    } else {
      cursor_x_ = -1.0;
    }
    if (use_event.keycode() == 37 || use_event.keycode() == 39) {
      new_cursor = GetNewCursorPositionForLeftRightArrow(
          use_event.keycode() == 37,
          shift_down,
          control_down);
    }
    if (!shift_down) {
      selection_start_ = new_cursor;
      selection_size_ = 0;
    } else {
      // Modify selection
      size_t new_sel_start = std::min(NonCursorPos(), new_cursor);
      size_t new_sel_end = std::max(NonCursorPos(), new_cursor);
      cursor_side_ = new_cursor < NonCursorPos() ? kLeft : kRight;
      selection_start_ = new_sel_start;
      selection_size_ = new_sel_end - new_sel_start;
    }
  }
  if (use_event.keycode() == 65 &&
      (use_event.modifiers() & KeyboardInputEvent::kControl)) {
    // Select all
    selection_start_ = 0;
    selection_size_ = text_.size();
    cursor_side_ = kRight;
    cursor_x_ = -1.0;
  }
  SetNeedsDisplay(false);
}

size_t TextArea::GetNewCursorPositionForHomeEnd(bool is_home) const {
  // home (36), end (35).
  if (new_row_indexes_.empty()) {
    if (!is_home)
      return text_.size();
    return 0;
  }
  vector<size_t>::const_iterator needle =
      std::upper_bound(new_row_indexes_.begin(),
                       new_row_indexes_.end(),
                       CursorPos());
  if (needle == new_row_indexes_.end()) {
    // on last line.
    if (is_home)
      return *(needle - 1);
    return text_.size();
  }
  if (!is_home)
    return *needle - 1;
  if (needle == new_row_indexes_.begin())
    return 0;
  return *(needle - 1);
}

size_t TextArea::GetNewCursorPositionForUpDownArrow(
    bool is_up) const {
  size_t idx = GetRowIndex(CursorPos());
  //size_t new_cursor = CursorPos();
  size_t last_row_idx = GetRowIndex(text_.size());
  if (idx == 0 && is_up) {
    // move to beginning of line
    return 0;
  }
  if (idx == last_row_idx && !is_up) {
    // move to end of line
    return text_.size();
  }
  if (is_up)
    idx--;
  else
    idx++;
  double offset = cursor_x_ >= 0.0 ?
      cursor_x_ : left_edges_[CursorPos()];
  // Find index for offset in row idx
  return IndexForRowAndOffset(idx, offset);
}

size_t TextArea::GetNewCursorPositionForLeftRightArrow(
    bool is_left, bool shift_down, bool control_down) const {
  // Left, right arrows. Move cursor.
  // Special case: left or right w/o shift when there's a selection.
  // Just move to the edge of the old selection.
  if (!shift_down && !control_down && selection_size_ > 0) {
    if (is_left)
      return selection_start_;
    return selection_start_ + selection_size_;
  }
  size_t cursor = CursorPos();
  if (!control_down) {
    if (is_left && cursor > 0)
      return cursor - 1;
    if (!is_left && cursor < text_.size())
      return cursor + 1;
    return cursor;
  }
  // control is down
  if (is_left) {
    // Scan backwards for word boundary
    if (cursor > 0)
      cursor--;
    while (cursor > 0) {
      if (isalpha(text_[cursor - 1]))
        cursor--;
      else
        break;
    }
    return cursor;
  }
  // Scan forwards for word boundary
  if (cursor < text_.size())
    cursor++;
  while (cursor < text_.size()) {
    if (isalpha(text_[cursor]))
      cursor++;
    else
      break;
  }
  return cursor;
}

void TextArea::OnKeyUp(const KeyboardInputEvent& event) {
  if (alt_down_ && event.keycode() == 18) {
    alt_down_ = false;
    return;
  }
}

bool TextArea::OnMouseDown(const Point& position) {
  selection_start_ = IndexForPoint(position);
  selection_size_ = 0;
  SetNeedsDisplay(false);
  return true;
}

void TextArea::SetSelection(size_t non_cursor_pos, size_t cursor_pos) {
  selection_start_ = std::min(non_cursor_pos, cursor_pos);
  selection_size_ = labs(static_cast<ssize_t>(non_cursor_pos) -
                         static_cast<ssize_t>(cursor_pos));
  cursor_side_ = cursor_pos < non_cursor_pos ? kLeft : kRight;
}

void TextArea::OnMouseDrag(const Point& position) {
  size_t drag_start = NonCursorPos();
  size_t drag_end = IndexForPoint(position);
  SetSelection(drag_start, drag_end);
  SetNeedsDisplay(false);
}

void TextArea::OnMouseUp() {}

bool TextArea::OnPaste(const std::string& str) {
  if (str.empty())
    return false;
  text_.replace(selection_start_, selection_size_, str);
  selection_start_ += str.size();
  selection_size_ = 0;
  SetNeedsDisplay(false);
  return true;
}

void TextArea::UpdateLeftEdges(cairo_t* cr) {
  const double max_width = frame_.size_.width_;
  double left_edge = 0.0;
  //double cursor_pos = 0.0;
  left_edges_.resize(text_.size() + 1, 0.0);
  new_row_indexes_.clear();
  size_t start_of_word = 0;  // index into text_
  for (size_t i = 0, e = text_.size(); i != e; ++i) {
    // TODO(adlr): handle UTF-8
    char str[2] = { text_[i], '\0' };
    
    if (text_[i] == '\n') {
      left_edges_[i] = left_edge;
      if (left_edge == 0.0 && i != 0)
        new_row_indexes_.push_back(i);
      left_edge = 0.0;
      continue;
    }

    cairo_text_extents_t ext;
    cairo_text_extents(cr, str, &ext);
    double width = ext.x_advance;

    double right_edge = left_edge + width;
    if (right_edge > max_width) {
      if (text_[i] == ' ') {
        // keep left_edge for this space, but update for next char
        right_edge = 0.0;
      } else {
        // this word is overflowing the width. can we move the
        // whole word down?
        if (start_of_word > 0 && left_edges_[start_of_word] > 0.0) {
          if (start_of_word > i) {
            printf("invalid start of word: %zu vs i=%zu\n",
                   start_of_word, i);
          }
          // Yes, we can
          i = start_of_word - 1;  // to override ++i in for loop
          left_edge = 0.0;
          continue;
        }
        // Guess we're breaking this word into pieces
        left_edge = 0.0;
        right_edge = width;
      }
    }
    left_edges_[i] = left_edge;
    if (left_edge == 0.0 && i != 0)
      new_row_indexes_.push_back(i);

    if ((i + 1) < text_.size() &&
        (text_[i] == ' ' || text_[i] == '\n') &&
        text_[i + 1] != ' ' && text_[i + 1] != '\n') {
      start_of_word = i + 1;
    }

    left_edge = right_edge;  // update for next iteration
  }
  left_edges_[left_edges_.size() - 1] = left_edge;
  if (left_edge == 0.0 && left_edges_.size() != 1)
    new_row_indexes_.push_back(left_edges_.size() - 1);
}

string TextArea::DebugLeftEdges() {
  string ret;
  for (size_t i = 0; i <= text_.size(); i++) {
    char buf[20] = {0};
    char str[2] = { i < text_.size() ? text_[i] : '$', '\0' };
    snprintf(buf, sizeof(buf), " %s(%.2f)[%zu]", str, left_edges_[i],
             GetRowIndex(i));
    ret += buf;
  }
  return ret;
}

string TextArea::GetLine(size_t start_idx, size_t* out_advance) {
  if (text_.size() + 1 != left_edges_.size()) {
    printf("Size mismatch!\n");
    return "";
  }

  string ret;
  for (size_t i = start_idx; i < text_.size(); ++i) {
    ret += text_[i];
    if ((i + 1) < text_.size()) {
      if (left_edges_[i + 1] <= left_edges_[i])
        break;
    }
  }
  if (out_advance)
    *out_advance = ret.size();
  if (!ret.empty() && *ret.rbegin() == '\n')
    ret.resize(ret.size() - 1);
  return ret;
}

size_t TextArea::GetRowIndex(size_t index) const {
  vector<size_t>::const_iterator needle =
      std::upper_bound(new_row_indexes_.begin(),
                       new_row_indexes_.end(),
                       index);
  if (needle == new_row_indexes_.end())
    return new_row_indexes_.size();
  return needle - new_row_indexes_.begin();
}

void TextArea::EraseSelection() {
  if (selection_size_) {
    text_.erase(text_.begin() + selection_start_,
                text_.begin() + selection_start_ + selection_size_);
    selection_size_ = 0;
  }
}

size_t TextArea::IndexForRowAndOffset(size_t row,
                                      double x_offset) const {
  // Left edge of first_in_row is 0
  size_t first_in_row = row == 0 ? 0 : new_row_indexes_[row - 1];
  if (left_edges_[first_in_row] != 0.0) {
    printf("Err: first_in_row has bad left edge! (%zu %f) %zu (%f)\n",
           row, x_offset, first_in_row, left_edges_[first_in_row]);
  }
  size_t last_in_row = 0;
  if (row >= new_row_indexes_.size()) {
    // use end of doc
    last_in_row = text_.size();
  } else {
    last_in_row = new_row_indexes_[row] - 1;
  }
  if (last_in_row == first_in_row) {
    return last_in_row;
  }
  vector<double>::const_iterator needle =
      std::lower_bound(left_edges_.begin() + first_in_row,
                       left_edges_.begin() + last_in_row,
                       x_offset);
  if ((*needle - x_offset == 0.0) ||
      (*needle - x_offset < x_offset - *(needle - 1)))
    return needle - left_edges_.begin();
  return needle - left_edges_.begin() - 1;
}

size_t TextArea::IndexForPoint(const Point& point) const {
  // get row height
  if (frame_.size_.height_ < 0.0001) {
    // avoid divide by zero
    return 0;
  }
  double y_fraction = (point.y_ - frame_.origin_.y_) / frame_.size_.height_;
  if (y_fraction < 0)
    return 0;
  if (y_fraction >= 1.0)
    return text_.size();
  size_t row = static_cast<size_t>(y_fraction * (new_row_indexes_.size() + 1));
  return IndexForRowAndOffset(row, point.x_ - frame_.origin_.x_);
}

void TextArea::Draw(cairo_t* cr, bool selected) {
  stroke_color_.CairoSetSourceRGBA(cr);
  cairo_select_font_face(cr, "Helvetica",
                         CAIRO_FONT_SLANT_NORMAL,
                         CAIRO_FONT_WEIGHT_NORMAL);
  cairo_set_font_size(cr, 13);

  cairo_font_extents_t extents;
  cairo_font_extents(cr, &extents);
  Point cursor = frame_.origin_;
  cursor.y_ += extents.ascent;

  UpdateLeftEdges(cr);
  //printf("DBG: %s\n", DebugLeftEdges().c_str());
  if (IsEditing() && selection_size_) {
    // Draw hilight for selection
    cairo_save(cr);
    for (size_t i = selection_start_;
         i < (selection_start_ + selection_size_); i++) {
      Point letter_origin(left_edges_[i],
                          GetRowIndex(i) * extents.height);
      double width = left_edges_[i + 1] - left_edges_[i];
      if (width <= 0.0)
        width = frame_.size_.width_ - left_edges_[i];
      Rect box(letter_origin, Size(width, extents.height));
      box = box.TranslatedBy(frame_.Left(), frame_.Top());
      box.CairoRectangle(cr);
      cairo_set_source_rgba(cr, 0.0, 0.0, 1.0, 0.2);
      cairo_fill(cr);
    }
    cairo_restore(cr);
  }

  for (size_t index = 0; index < text_.size(); ) {
    size_t advance = 0;
    string line = GetLine(index, &advance);
    //printf("Line: [%s]. adv: %zu\n", line.c_str(), advance);
    cursor.CairoMoveTo(cr);
    // Dbg("Printing:");
    // Dbg(line.c_str());
    cairo_show_text(cr, line.c_str());
    cursor.y_ += extents.height;
    index += advance;
    if (advance == 0)
      break;
  }
  frame_.size_.height_ =
      (GetRowIndex(text_.size()) + 1) * extents.height;

  if (IsEditing() || selected) {
    // draw rectangle for clarity
    frame_.CairoRectangle(cr);
    cairo_set_source_rgba(cr, 0.0, 0.0, 0.0, 0.2);
    cairo_set_line_width(cr, 1.0);
    cairo_stroke(cr);

    // Draw cursor if editing
    if (IsEditing() && selection_size_ == 0) {
      if (selection_start_ > (text_.size() + 1)) {
        printf("Illegal selection_start_: %zu\n", selection_start_);
        return;
      }
      double x_pos = left_edges_[selection_start_] + frame_.Left();
      double y_pos = extents.height * GetRowIndex(selection_start_) +
          frame_.Top();
      cairo_set_source_rgba(cr, 0.0, 0.0, 0.0, 1.0);  // opaque black
      cairo_set_line_width(cr, 1.0);
      cairo_move_to(cr, x_pos, y_pos);
      cairo_line_to(cr, x_pos, y_pos + extents.height);
      cairo_stroke(cr);
    }
  }
}

void TextArea::ApplyUndoOp(const TextAreaTransformUndoOp& op,
                           UndoManager* undo_manager) {
  size_t pre_sel_start = selection_start_;
  size_t pre_sel_size = selection_size_;
  string trimmed = text_.substr(op.remove_start(), op.remove_size());

  text_.erase(op.remove_start(), op.remove_size());
  text_.insert(op.remove_start(), op.insert());
  selection_start_ = op.final_selection_start();
  selection_size_ = op.final_selection_size();
  SetNeedsDisplay(false);
  
  unique_ptr<UndoOp> reverse(
      new TextAreaTransformUndoOp(this,
                                  op.remove_start(),
                                  op.insert().size(),
                                  trimmed,
                                  pre_sel_start,
                                  pre_sel_size));
  undo_manager->AddUndoOp(std::move(reverse));
}

void TextAreaTransformUndoOp::Exec(UndoManager* undo_manager) {
  printf("doing text edit undo: %s\n", String().c_str());
  text_area_->ApplyUndoOp(*this, undo_manager);
}

bool TextAreaTransformUndoOp::Merge(const UndoOp& that) {
  if (that.Type() != UndoOp::UndoType::TextAreaTransform)
    return false;

  const TextAreaTransformUndoOp* op =
      reinterpret_cast<const TextAreaTransformUndoOp*>(&that);
  if (text_area_ == op->text_area_ &&
      insert_.empty() && op->insert_.empty() &&
      op->remove_start_ == (remove_start_ + remove_size_)) {
    remove_size_ += op->remove_size_;
    return true;
  }
  return false;
}

string TextAreaTransformUndoOp::String() const {
  char buf[200];
  snprintf(buf, sizeof(buf),
           "remove[%zu, size %zu] ins[%s](size %zu) sel[%zu, size %zu]",
           remove_start_,
           remove_size_,
           insert_.c_str(),
           insert_.size(),
           final_selection_start_,
           final_selection_size_);
  return buf;
}

}  // namespace pdfsketch
