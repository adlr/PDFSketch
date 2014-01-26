// Copyright...

#ifndef PDFSKETCH_DOCUMENT_VIEW_H__
#define PDFSKETCH_DOCUMENT_VIEW_H__

#include <stdlib.h>

#include <poppler/cpp/poppler-document.h>

#include "graphic.h"
#include "scroll_bar_view.h"
#include "view.h"

namespace pdfsketch {

class DocumentView : public View {
 public:
  DocumentView()
      : doc_(NULL),
        zoom_(1.0),
        top_graphic_(NULL),
        bottom_graphic_(NULL) {}
  virtual void DrawRect(cairo_t* cr, const Rect& rect);
  void LoadFromPDF(const char* pdf_doc, size_t pdf_doc_length);
  void SetZoom(double zoom);

 private:
  void UpdateSize();
  Size PageSize(int page) const {
    if (!doc_)
      return Size();
    return Size(doc_->GetPageWidth(page), doc_->GetPageHeight(page));
  }
  Rect PageRect(int page) const;

  void AddGraphic(Graphic* graphic);

  poppler::SimpleDocument* doc_;
  double zoom_;
  Graphic* top_graphic_;
  Graphic* bottom_graphic_;
};

}  // namespace pdfsketch

#endif  // PDFSKETCH_DOCUMENT_VIEW_H__
