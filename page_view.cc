// Copyright

#include <stdio.h>

#include "page_view.h"

namespace pdfsketch {

PageView::PageView(const char* pdf_doc, size_t pdf_doc_length)
    : doc_(pdf_doc, pdf_doc_length) {
  top_fixed_to_top_ = true;
  bot_fixed_to_top_ = false;
  left_fixed_to_left_ = true;
  right_fixed_to_left_ = false;
  size_ = Size(doc_.GetPageWidth(1), doc_.GetPageHeight(1));
}

void PageView::DrawRect(cairo_t* cr, const Rect& rect) {
  printf("PageView::Draw called %f %f\n", size_.width_, size_.height_);
  if (doc_.GetPageWidth(1) < 1 || doc_.GetPageHeight(1) < 1) {
    printf("page too small. Avoiding divide by 0\n");
    return;
  }
  double scale_x = size_.width_ / doc_.GetPageWidth(1);
  double scale_y = size_.height_ / doc_.GetPageHeight(1);
  cairo_save(cr);
  cairo_scale(cr, scale_x, scale_y);
  doc_.RenderPage(1, false, cr);
  cairo_restore(cr);
  cairo_move_to(cr, 0.0, 0.0);
  cairo_line_to(cr, size_.width_, size_.height_);
}

}  // namespace pdfsketch
