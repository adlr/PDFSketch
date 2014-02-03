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

namespace {
double GetTextDisplayWidth(cairo_t* cr, const string& str) {
  cairo_text_extents_t ext;
  cairo_text_extents(cr, str.c_str(), &ext);
  return ext.width;
}
}  // namespace {}

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

  bool first_word = true;  // first word on the line
  size_t length = 0;
  const char* str = text_.c_str();
  string valid_line;  // line w/o final word fragment
  string line;
  for (; *str; ++str) {
    if (GetTextDisplayWidth(cr, valid_line) > frame_.size_.width_ ||
        GetTextDisplayWidth(cr, line) > frame_.size_.width_) {
      printf("valid_line or line is too long!\n");
      return;
    }

    if (*str == '\n') {
      cursor.CairoMoveTo(cr);
      cairo_show_text(cr, line.c_str());
      cursor.x_ = frame_.Left();
      cursor.y_ += extents.height;
      line = valid_line = "";
      first_word = true;
      continue;
    }

    line += *str;
    if (GetTextDisplayWidth(cr, line) > frame_.size_.width_) {
      cursor.CairoMoveTo(cr);
      cairo_show_text(cr, valid_line.c_str());
      cursor.x_ = frame_.Left();
      cursor.y_ += extents.height;

      line = valid_line = line.substr(valid_line.size());
      continue;
    }
    if (*str == ' ' || first_word) {
      valid_line = line;
      if (*str == ' ')
        first_word = false;
      continue;
    }
  }
  if (!line.empty()) {
    cursor.CairoMoveTo(cr);
    cairo_show_text(cr, line.c_str());
  }
}

}  // namespace pdfsketch
