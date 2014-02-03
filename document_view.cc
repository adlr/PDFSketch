// Copyright...

#include "document_view.h"

#include <stdio.h>

#include <cairo.h>
#include <cairo-pdf.h>

#include "rectangle.h"
#include "text_area.h"

using std::set;
using std::vector;

namespace pdfsketch {

namespace {
const double kSpacing = 20.0;  // between pages
}  // namespace {}

void DocumentView::LoadFromPDF(const char* pdf_doc, size_t pdf_doc_length) {
  if (doc_)
    delete doc_;
  doc_ = new poppler::SimpleDocument(pdf_doc, pdf_doc_length);

  UpdateSize();

  // Add a silly graphic
  Graphic* gr = new Rectangle();
  gr->SetDelegate(this);
  AddGraphic(gr);

  SetNeedsDisplay();
}

void DocumentView::UpdateSize() {
  // Update bounds
  double max_page_width = 0.0;  // w/o spacing
  double total_height = kSpacing;  // w/ spacing
  for (int i = 1; i <= doc_->GetNumPages(); i++) {
    Size size = PageSize(i).ScaledBy(zoom_).RoundedUp();
    max_page_width = std::max(max_page_width, size.width_);
    total_height += size.height_ + kSpacing;
  }
  SetSize(Size(max_page_width + 2 * kSpacing, total_height));
}

void DocumentView::SetZoom(double zoom) {
  zoom_ = zoom;
  printf("zoom is %f\n", zoom_);
 UpdateSize();
}

Rect DocumentView::PageRect(int page) const {
  double existing_pages_height = kSpacing;
  for (int i = 1; i < page; i++) {
    existing_pages_height += ceil(doc_->GetPageHeight(i) * zoom_) + kSpacing;
  }
  Size page_size = PageSize(page).ScaledBy(zoom_).RoundedUp();
  double offset = static_cast<int>(size_.width_ / 2 - page_size.width_ / 2);
  return Rect(offset, existing_pages_height,
              page_size.width_, page_size.height_);
}

int DocumentView::PageForPoint(const Point& point) const {
  for (int i = 1; i <= doc_->GetNumPages(); i++) {
    if (point.y_ <= PageRect(i).Bottom())
      return i;
  }
  return doc_->GetNumPages();
}

Point DocumentView::ConvertPointToPage(const Point& point, int page) const {
  Rect page_rect = PageRect(page);
  return point.TranslatedBy(-page_rect.Left(), -page_rect.Top()).
      ScaledBy(1 / zoom_);
}

Point DocumentView::ConvertPointFromPage(const Point& point, int page) const {
  Rect page_rect = PageRect(page);
  return point.ScaledBy(zoom_).TranslatedBy(page_rect.Left(), page_rect.Top());
}

void DocumentView::AddGraphic(Graphic* graphic) {
  if (top_graphic_) {
    top_graphic_->upper_sibling_ = graphic;
  } else {
    bottom_graphic_ = graphic;
  }
  graphic->lower_sibling_ = top_graphic_;
  graphic->upper_sibling_ = NULL;
  top_graphic_ = graphic;
}

void DocumentView::RemoveGraphic(Graphic* graphic) {
  if (graphic->upper_sibling_) {
    graphic->upper_sibling_->lower_sibling_ = graphic->lower_sibling_;
  } else if (top_graphic_ == graphic) {
    top_graphic_ = graphic->lower_sibling_;
  } else {
    printf("top graphic is wrong!\n");
  }
  if (graphic->lower_sibling_) {
    graphic->lower_sibling_->upper_sibling_ = graphic->upper_sibling_;
  } else if (bottom_graphic_ == graphic) {
    bottom_graphic_ = graphic->upper_sibling_;
  } else {
    printf("bottom graphic is wrong!\n");
  }
  graphic->upper_sibling_ = graphic->lower_sibling_ = NULL;
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
    cairo_scale(cr, zoom_, zoom_);
    doc_->RenderPage(i, false, cr);

    // Draw graphics
    for (Graphic* gr = bottom_graphic_; gr; gr = gr->upper_sibling_) {
      if (gr->Page() != i)
        continue;
      gr->Draw(cr);
    }

    cairo_restore(cr);
  }

  // draw knobs
  for (Graphic* gr = bottom_graphic_; gr; gr = gr->upper_sibling_) {
    if (selected_graphics_.find(gr) == selected_graphics_.end())
      continue;
    cairo_save(cr);
    Rect page_rect = PageRect(gr->Page());
    cairo_translate(cr, page_rect.origin_.x_, page_rect.origin_.y_);
    cairo_scale(cr, zoom_, zoom_);
    gr->DrawKnobs(cr);
    cairo_restore(cr);
  }
}

namespace {
cairo_status_t HandleCairoStreamWrite(void* closure,
                                      const unsigned char *data,
                                      unsigned int length) {
  vector<char>* out = reinterpret_cast<vector<char>*>(closure);
  out->insert(out->end(), data, data + length);
  return CAIRO_STATUS_SUCCESS;
}
}  // namespace {}

void DocumentView::ExportPDF(vector<char>* out) {
  if (!doc_) {
    printf("can't export w/o a doc\n");
    return;
  }
  cairo_surface_t *surface = cairo_pdf_surface_create_for_stream(
      HandleCairoStreamWrite, out, 6 * 72, 6 * 72);
  cairo_t* cr = cairo_create(surface);
  for (size_t i = 1; i <= doc_->GetNumPages(); i++) {
    Size pg_size = PageSize(i);
    cairo_pdf_surface_set_size(surface, pg_size.width_, pg_size.height_);
    // for each page:
    cairo_save(cr);
    printf("rendering page\n");
    doc_->RenderPage(i, true, cr);
    printf("rendering complete\n");
    // Draw graphics
    for (Graphic* gr = bottom_graphic_; gr; gr = gr->upper_sibling_) {
      if (gr->Page() != i)
        continue;
      gr->Draw(cr);
    }
    cairo_restore(cr);
    cairo_surface_show_page(surface);
  }
  cairo_destroy(cr);
  cairo_surface_finish(surface);
  cairo_surface_destroy(surface);
}

View* DocumentView::OnMouseDown(const MouseInputEvent& event) {
  if (!selected_graphics_.empty()) {
    // See if we hit a knob
    for (Graphic* gr = top_graphic_; gr; gr = gr->lower_sibling_) {
      if (selected_graphics_.find(gr) == selected_graphics_.end())
        continue;
      Point pos = ConvertPointToPage(event.position().TranslatedBy(0.5, 0.5),
                                     gr->Page());
      int knob = kKnobNone;
      printf("testing graphic for knob hit\n");
      if ((knob = gr->PointInKnob(pos)) != kKnobNone) {
        resizing_graphic_ = gr;
        gr->BeginResize(pos, knob, false);
        return this;
      }
      printf("no hit\n");
    }
  }

  selected_graphics_.clear();

  if (!toolbox_)
    return this;

  if (toolbox_->CurrentTool() == Toolbox::ARROW) {
    // See if we hit a graphic
    for (Graphic* gr = top_graphic_; gr; gr = gr->lower_sibling_) {
      Point page_pos = ConvertPointToPage(event.position().TranslatedBy(0.5, 0.5),
                                          gr->Page());
      if (gr->frame_.Contains(page_pos)) {
        selected_graphics_.insert(gr);
        last_move_pos_ = event.position();
        gr->SetNeedsDisplay(true);
        return this;
      }
    }
    return this;
  }

  Graphic* gr = toolbox_->NewGraphic();
  gr->SetDelegate(this);
  AddGraphic(gr);
  int page = PageForPoint(event.position());
  Point page_pos = ConvertPointToPage(event.position().TranslatedBy(0.5, 0.5),
                                      page);
  gr->Place(page, page_pos, false);
  placing_graphic_ = gr;
  return this;
}

void DocumentView::OnMouseDrag(const MouseInputEvent& event) {
  if (resizing_graphic_) {
    Point pos = ConvertPointToPage(event.position().TranslatedBy(0.5, 0.5),
                                   resizing_graphic_->Page());
    resizing_graphic_->UpdateResize(pos, false);
    return;
  }

  if (placing_graphic_) {
    Point page_pos = ConvertPointToPage(event.position().TranslatedBy(0.5, 0.5),
                                        placing_graphic_->Page());
    placing_graphic_->PlaceUpdate(page_pos, false);
    return;
  }

  if (!selected_graphics_.empty()) {
    // Move
    double dx = event.position().x_ - last_move_pos_.x_;
    double dy = event.position().y_ - last_move_pos_.y_;
    for (set<Graphic*>::iterator it = selected_graphics_.begin(),
             e = selected_graphics_.end(); it != e; ++it) {
      (*it)->SetNeedsDisplay(true);
      (*it)->frame_.origin_ = (*it)->frame_.origin_.TranslatedBy(dx, dy);
      (*it)->SetNeedsDisplay(true);
    }
    last_move_pos_ = event.position();
  }
}

void DocumentView::OnMouseUp(const MouseInputEvent& event) {
  if (resizing_graphic_) {
    resizing_graphic_->EndResize();
    resizing_graphic_ = NULL;
    return;
  }

  if (!placing_graphic_)
    return;
  placing_graphic_->SetNeedsDisplay(true);
  if (placing_graphic_->PlaceComplete()) {
    RemoveGraphic(placing_graphic_);
    delete placing_graphic_;
  } else {
    selected_graphics_.insert(placing_graphic_);
  }
  placing_graphic_ = NULL;
  toolbox_->SelectTool(Toolbox::ARROW);
}

bool DocumentView::OnKeyDown(const KeyboardInputEvent& event) {
  if (event.keycode() == 8 || event.keycode() == 46) {  // backspace, delete
    // delete selected graphics
    for (set<Graphic*>::iterator it = selected_graphics_.begin(),
             e = selected_graphics_.end(); it != e; ++it) {
      (*it)->SetNeedsDisplay(true);
      RemoveGraphic(*it);
      delete *it;
    }
    selected_graphics_.clear();
  }
  return true;
}

void DocumentView::SetNeedsDisplayInPageRect(int page, const Rect& rect) {
  printf("%s\n", __func__);
  Rect local(ConvertPointFromPage(rect.UpperLeft(), page),
             ConvertPointFromPage(rect.LowerRight(), page));
  SetNeedsDisplayInRect(local);
}

}  // namespace pdfsketch
