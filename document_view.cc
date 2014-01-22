// Copyright...

#include "document_view.h"

#include <stdio.h>

#include <cairo.h>

namespace pdfsketch {

namespace {
const double kSpacing = 20.0;
}  // namespace {}

void DocumentView::LoadFromPDF(const char* pdf_doc, size_t pdf_doc_length) {
  if (doc_)
    delete doc_;
  doc_ = new poppler::SimpleDocument(pdf_doc, pdf_doc_length);

  // Update bounds
  double max_page_width = 0.0;  // w/o spacing
  double total_height = kSpacing;  // w/ spacing
  for (int i = 1; i <= doc_->GetNumPages(); i++) {
    Size size = PageSize(i);
    max_page_width = std::max(max_page_width, size.width_);
    total_height += size.height_ + kSpacing;
  }
  size_ = Size(max_page_width + 2 * kSpacing, total_height);

  SetNeedsDisplay();
}

Rect DocumentView::PageRect(int page) const {
  double existing_pages_height = kSpacing;
  for (int i = 1; i < page; i++) {
    existing_pages_height += doc_->GetPageHeight(i) + kSpacing;
  }
  Size page_size = PageSize(page);
  double offset = static_cast<int>(size_.width_ / 2 - page_size.width_ / 2);
  return Rect(offset, existing_pages_height,
              page_size.width_, page_size.height_);
}

void DocumentView::DrawRect(cairo_t* cr, const Rect& rect) {
  if (!doc_) {
    double border = 15.0;
    cairo_set_line_width(cr, 2.0);
    cairo_set_line_join(cr, CAIRO_LINE_JOIN_MITER);
    cairo_set_line_cap(cr, CAIRO_LINE_CAP_SQUARE);
    for (int i = 0; i < 3; i++) {
      Rect page(0.0, double(i) * size_.height_ / 3.0,
                size_.width_, size_.height_ / 3.0);
      page = page.InsetBy(border + 1);
      cairo_set_source_rgb(cr, i == 0, i == 1, i == 2);
      page.CairoRectangle(cr);
      cairo_stroke(cr);
    }
    return;
  }

  for (int i = 1; i <= doc_->GetNumPages(); i++) {
    Rect page_rect = PageRect(i);
    if (!rect.Intersects(page_rect))
      continue;
    // draw this page
    cairo_save(cr);
    cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);
    cairo_set_line_width(cr, 1.0);
    Rect outline = page_rect.InsetBy(-0.5);
    outline.CairoRectangle(cr);
    cairo_stroke(cr);

    cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
    page_rect.CairoRectangle(cr);
    cairo_fill(cr);
    cairo_translate(cr, page_rect.origin_.x_, page_rect.origin_.y_);
    doc_->RenderPage(i, false, cr);
    cairo_restore(cr);
  }
}

}  // namespace pdfsketch
