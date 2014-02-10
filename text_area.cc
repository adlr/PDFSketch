// Copyright...

#include "text_area.h"

#include <string>

using std::string;

namespace pdfsketch {

void TextArea::Place(int page, const Point& location, bool constrain) {
  page_ = page;
  text_ = "The quick brown fox jumps over the lazy dog.";
  PlaceUpdate(location, constrain);
}
void TextArea::PlaceUpdate(const Point& location, bool constrain) {
  frame_ = Rect(location, Size(150.0, 72.0));
}
bool TextArea::PlaceComplete() {
  return false;
}

void TextArea::BeginEditing() {
  Graphic::BeginEditing();
  selection_start_ = text_.size();
  selection_size_ = 0;
}

void TextArea::EndEditing() {
  Graphic::EndEditing();
}

void TextArea::OnKeyText(const KeyboardInputEvent& event) {
  EraseSelection();
  text_.insert(selection_start_, event.text());
  selection_start_ += event.text().size();
  SetNeedsDisplay(false);
}

void TextArea::OnKeyDown(const KeyboardInputEvent& event) {
  printf("got keydown %d\n", event.keycode());
  if (event.keycode() == 8 || event.keycode() == 46) {
    // backspace, delete
    if (selection_size_ > 0) {
      EraseSelection();
    } else if (event.keycode() == 8) {
      // do backspace
      if (selection_start_ == 0)
        return;
      text_.erase(text_.begin() + selection_start_ - 1);
      selection_start_--;
    } else {
      // do delete
      if (selection_start_ >= text_.size())
        return;
      text_.erase(text_.begin() + selection_start_);
    }
  }
  if (event.keycode() == 37) {  // left arrow
    selection_size_ = 0;
    if (selection_start_ > 0)
      selection_start_--;
  }
  if (event.keycode() == 39) {  // right arrow
    selection_start_ += selection_size_;
    selection_size_ = 0;
    if (selection_start_ < text_.size())
      selection_start_++;
  }
  SetNeedsDisplay(false);
}

void TextArea::OnKeyUp(const KeyboardInputEvent& event) {
}

// namespace {
// double GetTextDisplayWidth(cairo_t* cr, const string& str) {
//   cairo_text_extents_t ext;
//   cairo_text_extents(cr, str.c_str(), &ext);
//   return ext.width;
// }
// }  // namespace {}

void TextArea::UpdateLeftEdges(cairo_t* cr) {
  const double max_width = frame_.size_.width_;
  double left_edge = 0.0;
  //double cursor_pos = 0.0;
  left_edges_.resize(text_.size() + 1, 0.0);
  size_t start_of_word = 0;  // index into text_
  for (size_t i = 0, e = text_.size(); i != e; ++i) {
    // TODO(adlr): handle UTF-8
    char str[2] = { text_[i], '\0' };
    
    if (text_[i] == '\n') {
      left_edges_[i] = left_edge = 0.0;
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

    if ((i + 1) < text_.size() &&
        (text_[i] == ' ' || text_[i] == '\n') &&
        text_[i + 1] != ' ' && text_[i + 1] != '\n') {
      start_of_word = i + 1;
    }

    left_edge = right_edge;  // update for next iteration
  }
  left_edges_[left_edges_.size() - 1] = left_edge;
}

string TextArea::DebugLeftEdges() {
  string ret;
  for (size_t i = 0; i < text_.size(); i++) {
    char buf[20] = {0};
    char str[2] = { text_[i], '\0' };
    snprintf(buf, sizeof(buf), " %s(%.2f)", str, left_edges_[i]);
    ret += buf;
  }
  return ret;
}

string TextArea::GetLine(size_t start_idx) {
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
  return ret;
}

size_t TextArea::GetRowIndex(size_t index) {
  size_t ret = 0;
  for (size_t i = 1; i < index; i++) {
    if (left_edges_[i] <= left_edges_[i - 1])
      ret++;
  }
  return ret;
}

void TextArea::EraseSelection() {
  if (selection_size_) {
    text_.erase(text_.begin() + selection_start_,
                text_.begin() + selection_start_ + selection_size_);
    selection_size_ = 0;
  }
}

void TextArea::Draw(cairo_t* cr) {
  // draw rectangle for clarity
  frame_.CairoRectangle(cr);
  cairo_set_source_rgba(cr, 1.0, 0.0, 0.0, 1.0);  // opaque red
  cairo_set_line_width(cr, 1.0);
  cairo_stroke(cr);

  stroke_color_.CairoSetSourceRGBA(cr);
  cairo_select_font_face(cr, "serif",
                         CAIRO_FONT_SLANT_NORMAL,
                         CAIRO_FONT_WEIGHT_NORMAL);
  cairo_set_font_size(cr, 13);

  cairo_font_extents_t extents;
  cairo_font_extents(cr, &extents);
  Point cursor = frame_.origin_;
  cursor.y_ += extents.ascent;

  UpdateLeftEdges(cr);
  //printf("DBG: %s\n", DebugLeftEdges().c_str());
  for (size_t index = 0; index < text_.size(); ) {
    string line = GetLine(index);
    if (line.empty())
      break;
    cursor.CairoMoveTo(cr);
    cairo_show_text(cr, line.c_str());
    //printf("DRAW:[%s]\n", line.c_str());
    cursor.y_ += extents.height;
    index += line.size();
  }

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
  }
}

}  // namespace pdfsketch
