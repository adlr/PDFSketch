// Copyright...

#ifndef PDFSKETCH_DOCUMENT_VIEW_H__
#define PDFSKETCH_DOCUMENT_VIEW_H__

#include <set>
#include <stdlib.h>
#include <vector>

#include <poppler/cpp/poppler-document.h>

#include "graphic.h"
#include "scroll_bar_view.h"
#include "toolbox.h"
#include "undo_manager.h"
#include "view.h"

namespace pdfsketch {

class DocumentView : public View,
                     public GraphicDelegate {
 public:
  DocumentView()
      : undo_manager_(NULL),
        doc_(NULL),
        zoom_(1.0),
        toolbox_(NULL),
        bottom_graphic_(NULL),
        placing_graphic_(NULL),
        editing_graphic_(NULL),
        resizing_graphic_(NULL) {}
  virtual std::string Name() const { return "DocumentView"; }
  virtual void DrawRect(cairo_t* cr, const Rect& rect);
  void LoadFromPDF(const char* pdf_doc, size_t pdf_doc_length);
  void GetPDFData(const char** out_buf, size_t* out_len) const;
  void SetZoom(double zoom);
  void ExportPDF(std::vector<char>* out);
  void Serialize(pdfsketchproto::Document* msg) const;
  void SetToolbox(Toolbox* toolbox) {
    toolbox_ = toolbox;
  }
  void SetUndoManager(UndoManager* undo_manager) {
    undo_manager_ = undo_manager;
  }

  void AddGraphic(std::shared_ptr<Graphic> graphic) {
    InsertGraphicAfter(graphic, NULL);
  }
  // GraphicDelegate methods
  virtual void SetNeedsDisplayInPageRect(int page, const Rect& rect);
  virtual Point ConvertPointFromGraphic(int page, const Point& point) {
    return ConvertPointFromPage(point, page);
  }
  virtual Point ConvertPointToGraphic(int page, const Point& point) {
    return ConvertPointToPage(point, page);
  }
  virtual double GetZoom() { return zoom_; }

  virtual View* OnMouseDown(const MouseInputEvent& event);
  virtual void OnMouseDrag(const MouseInputEvent& event);
  virtual void OnMouseUp(const MouseInputEvent& event);

  void MoveGraphicsUndo(const std::set<Graphic*>& graphics,
                        double dx, double dy);

  virtual bool OnKeyText(const KeyboardInputEvent& event);
  virtual bool OnKeyDown(const KeyboardInputEvent& event);

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

  void InsertGraphicAfter(std::shared_ptr<Graphic> graphic,
                          Graphic* upper_sibling);

  bool GraphicIsSelected(Graphic* graphic) {
    return selected_graphics_.find(graphic) != selected_graphics_.end();
  }

  std::shared_ptr<Graphic> SharedPtrForGraphic(Graphic* graphic) const;
  void RemoveGraphicsUndo(std::set<Graphic*> graphics);

  UndoManager* undo_manager_;

  // Returns the shard_ptr of the removed graphic, incase you want to
  // move it somewhere. If you ignore the return value, graphic may
  // be deleted.
  std::shared_ptr<Graphic> RemoveGraphic(Graphic* graphic);

  poppler::SimpleDocument* doc_;
  double zoom_;
  Toolbox* toolbox_;
  std::shared_ptr<Graphic> top_graphic_;
  Graphic* bottom_graphic_;
  Graphic* placing_graphic_;
  Graphic* editing_graphic_;

  std::set<Graphic*> selected_graphics_;
  Graphic* resizing_graphic_;
  Point last_move_pos_;
  Point start_move_pos_;
};

}  // namespace pdfsketch

#endif  // PDFSKETCH_DOCUMENT_VIEW_H__
