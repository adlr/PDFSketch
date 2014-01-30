// Copyright...

#ifndef PDFSKETCH_DOCUMENT_VIEW_H__
#define PDFSKETCH_DOCUMENT_VIEW_H__

#include <stdlib.h>
#include <vector>

#include <poppler/cpp/poppler-document.h>

#include "graphic.h"
#include "scroll_bar_view.h"
#include "view.h"

namespace pdfsketch {

class DocumentView : public View,
                     public GraphicDelegate {
 public:
  DocumentView()
      : doc_(NULL),
        zoom_(1.0),
        top_graphic_(NULL),
        bottom_graphic_(NULL) {}
  virtual void DrawRect(cairo_t* cr, const Rect& rect);
  void LoadFromPDF(const char* pdf_doc, size_t pdf_doc_length);
  void SetZoom(double zoom);
  void ExportPDF(std::vector<char>* out);

  // GraphicDelegate methods
  virtual void SetNeedsDisplayInPageRect(int page, const Rect& rect);
  virtual Point ConvertPointFromGraphic(int page, const Point& point) {
    return ConvertPointFromPage(point, page);
  }
  virtual Point ConvertPointToGraphic(int page, const Point& point) {
    return ConvertPointToPage(point, page);
  }

  virtual View* OnMouseDown(const MouseInputEvent& event);
  virtual void OnMouseDrag(const MouseInputEvent& event);
  virtual void OnMouseUp(const MouseInputEvent& event);

 private:
  void UpdateSize();
  Size PageSize(int page) const {
    if (!doc_)
      return Size();
    return Size(doc_->GetPageWidth(page), doc_->GetPageHeight(page));
  }
  Rect PageRect(int page) const;

  // point may be outside page
  int PageForPoint(const Point& point) const;

  // Convert from local view coordinates to/from page coordinates
  Point ConvertPointToPage(const Point& point, int page) const;
  Point ConvertPointFromPage(const Point& point, int page) const;

  void AddGraphic(Graphic* graphic);
  void RemoveGraphic(Graphic* graphic);

  poppler::SimpleDocument* doc_;
  double zoom_;
  Graphic* top_graphic_;
  Graphic* bottom_graphic_;
  Graphic* placing_graphic_;
};

}  // namespace pdfsketch

#endif  // PDFSKETCH_DOCUMENT_VIEW_H__
