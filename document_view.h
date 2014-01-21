// Copyright...

#ifndef PDFSKETCH_DOCUMENT_VIEW_H__
#define PDFSKETCH_DOCUMENT_VIEW_H__

#include "scroll_bar_view.h"
#include "view.h"

namespace pdfsketch {

class DocumentView : public View {
 public:
  DocumentView() {}
  virtual void DrawRect(cairo_t* ctx, const Rect& rect);
}

}  // namespace pdfsketch

#endif  // PDFSKETCH_DOCUMENT_VIEW_H__
