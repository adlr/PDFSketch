// Copyright...

#ifndef PDFSKETCH_PAGE_VIEW_H__
#define PDFSKETCH_PAGE_VIEW_H__

#include <stdlib.h>

#include <poppler-document.h>

#include "view.h"

namespace pdfsketch {

class PageView : public View {
 public:
  PageView(const char* pdf_doc, size_t pdf_doc_length);
  virtual void DrawRect(cairo_t* cr, const Rect& rect);

 private:
  poppler::SimpleDocument doc_;
};

}  // namespace pdfsketch

#endif  // PDFSKETCH_PAGE_VIEW_H__
