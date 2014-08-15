// Copyright...

#ifndef PDFSKETCH_DOCUMENT_VIEW_H__
#define PDFSKETCH_DOCUMENT_VIEW_H__

#include <set>
#include <stdlib.h>
#include <vector>

#include <poppler-document.h>

#include "graphic.h"
#include "scroll_bar_view.h"
#include "toolbox.h"
#include "undo_manager.h"
#include "view.h"

namespace pdfsketch {

class DocumentView : public View,
                     public GraphicDelegate {
 public:
  DocumentView() {}
  virtual std::string Name() const { return "DocumentView"; }
  virtual void DrawRect(cairo_t* cr, const Rect& rect);
  void LoadFromPDF(const char* pdf_doc, size_t pdf_doc_length);
  void GetPDFData(const char** out_buf, size_t* out_len) const;
  void SetZoom(double zoom);
  void ExportPDF(std::vector<char>* out);
  void Serialize(pdfsketchproto::Document* msg) const {
    SerializeGraphics(false, msg);
  }
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

  virtual std::string OnCopy();
  virtual bool OnPaste(const std::string& str);

  void MoveGraphicsUndo(const std::set<Graphic*>& graphics,
                        double dx, double dy, int dpage);
  void SetGraphicFrameUndo(Graphic* gr, Rect frame);

  virtual bool OnKeyText(const KeyboardInputEvent& event);
  virtual bool OnKeyDown(const KeyboardInputEvent& event);
  virtual bool OnKeyUp(const KeyboardInputEvent& event);

 private:
  void SerializeGraphics(bool selected_only,
                         pdfsketchproto::Document* msg) const;

  void UpdateSize();
  Size PageSize(int page) const;  // graphic/PDF coords
  Rect PageRect(int page) const;  // view coords

  // point may be outside page
  int PageForPoint(const Point& point) const;

  // Convert from local view coordinates to/from page coordinates
  Point ConvertPointToPage(const Point& point, int page) const;
  Point ConvertPointFromPage(const Point& point, int page) const;
  Rect ConvertRectToPage(const Rect& rect, int page) const {
    return Rect(ConvertPointToPage(rect.UpperLeft(), page),
                ConvertPointToPage(rect.LowerRight(), page));
  }

  void GetVisibleCenterPageAndPoint(Point* out_point,
                                    int* out_page) const;

  void InsertGraphicAfter(std::shared_ptr<Graphic> graphic,
                          Graphic* upper_sibling);
  void InsertGraphicAfterUndo(std::shared_ptr<Graphic> graphic,
                              Graphic* upper_sibling);

  bool GraphicIsSelected(Graphic* graphic) {
    return selected_graphics_.find(graphic) != selected_graphics_.end();
  }

  std::shared_ptr<Graphic> SharedPtrForGraphic(Graphic* graphic) const;
  void RemoveGraphicsUndo(std::set<Graphic*> graphics);

  UndoManager* undo_manager_{nullptr};

  // Returns the shard_ptr of the removed graphic, incase you want to
  // move it somewhere. If you ignore the return value, graphic may
  // be deleted.
  std::shared_ptr<Graphic> RemoveGraphic(Graphic* graphic);

  // poppler::SimpleDocument* doc_;
  std::vector<char> poppler_doc_data_;
  std::unique_ptr<poppler::document> poppler_doc_;
  float cached_surface_device_zoom_{1.0};
  Rect cached_subrect_;
  cairo_surface_t* cached_surface_{nullptr};  // TODO(adlr): free in dtor

  double zoom_{1.0};
  Toolbox* toolbox_{nullptr};
  std::shared_ptr<Graphic> top_graphic_;
  Graphic* bottom_graphic_{nullptr};
  Graphic* placing_graphic_{nullptr};
  Graphic* editing_graphic_{nullptr};

  std::set<Graphic*> selected_graphics_;
  Graphic* resizing_graphic_{nullptr};
  Point last_move_pos_;
  int last_move_page_{0};
  Point start_move_pos_;
  int start_move_page_{0};

  Rect resize_graphic_original_frame_;
};

}  // namespace pdfsketch

#endif  // PDFSKETCH_DOCUMENT_VIEW_H__
