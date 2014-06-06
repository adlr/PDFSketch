// Copyright...

#include "document_view.h"

#include <stdio.h>

#include <cairo.h>
#include <cairo-pdf.h>
#include <poppler-page.h>
#include <poppler-page-renderer.h>

#include "graphic_factory.h"
#include "rectangle.h"

using std::make_pair;
using std::pair;
using std::set;
using std::shared_ptr;
using std::unique_ptr;
using std::vector;

namespace pdfsketch {

namespace {
const double kSpacing = 20.0;  // between pages
}  // namespace {}

void DocumentView::LoadFromPDF(const char* pdf_doc, size_t pdf_doc_length) {
  // if (doc_)
  //   delete doc_;
  poppler_doc_data_.clear();
  poppler_doc_data_.insert(poppler_doc_data_.begin(),
                           pdf_doc,
                           pdf_doc + pdf_doc_length);
  // doc_ = new poppler::SimpleDocument(&poppler_doc_data_[0],
  //                                    poppler_doc_data_.size());
  poppler_doc_.reset(poppler::document::load_from_raw_data(pdf_doc, pdf_doc_length));

  UpdateSize();

  SetNeedsDisplay();
}

void DocumentView::GetPDFData(const char** out_buf,
                              size_t* out_len) const {
  if (!poppler_doc_.get())
    return;
  // *out_buf = doc_->buf();
  // *out_len = doc_->buf_len();
  *out_buf = &poppler_doc_data_[0];
  *out_len = poppler_doc_data_.size();
}

void DocumentView::UpdateSize() {
  // Update bounds
  double max_page_width = 0.0;  // w/o spacing
  double total_height = kSpacing;  // w/ spacing
  for (int i = 0; i < poppler_doc_->pages(); i++) {
    Size size = PageSize(i).ScaledBy(zoom_).RoundedUp();
    max_page_width = std::max(max_page_width, size.width_);
    total_height += size.height_ + kSpacing;
  }
  SetSize(Size(max_page_width + 2 * kSpacing, total_height));
  // invalidate cache
  cached_subrect_ = Rect();
}

void DocumentView::Serialize(pdfsketchproto::Document* msg) const {
  printf("%s:%d\n", __FILE__, __LINE__);
  for (Graphic* gr = bottom_graphic_; gr; gr = gr->upper_sibling_) {
    printf("%s:%d\n", __FILE__, __LINE__);
    pdfsketchproto::Graphic* gr_msg = msg->add_graphic();
    printf("%s:%d\n", __FILE__, __LINE__);
    gr->Serialize(gr_msg);
    printf("%s:%d\n", __FILE__, __LINE__);
  }
  printf("%s:%d\n", __FILE__, __LINE__);
}

void DocumentView::SetZoom(double zoom) {
  zoom_ = zoom;
  printf("zoom is %f\n", zoom_);
 UpdateSize();
}

Size DocumentView::PageSize(int page) const {
  if (!poppler_doc_.get())
    return Size();
  unique_ptr<poppler::page> ppage(poppler_doc_->create_page(page));
  if (!ppage.get())
    printf("Bug - null page3\n");
  poppler::rectf rect = ppage->page_rect();
  return Size(rect.width(), rect.height());
  // return Size(doc_->GetPageWidth(page), doc_->GetPageHeight(page));
}

Rect DocumentView::PageRect(int page) const {
  double existing_pages_height = kSpacing;
  for (int i = 0; i < page; i++) {
    unique_ptr<poppler::page> ppage(poppler_doc_->create_page(i));
    if (!ppage.get())
      printf("Bug - null page\n");
    poppler::rectf rect = ppage->page_rect();
    existing_pages_height += ceil(rect.height() * zoom_) + kSpacing;
  }
  Size page_size = PageSize(page).ScaledBy(zoom_).RoundedUp();
  double offset = static_cast<int>(size_.width_ / 2 - page_size.width_ / 2);
  return Rect(offset, existing_pages_height,
              page_size.width_, page_size.height_);
}

int DocumentView::PageForPoint(const Point& point) const {
  for (int i = 0; i < poppler_doc_->pages(); i++) {
    unique_ptr<poppler::page> page(poppler_doc_->create_page(i));
    if (!page.get())
      printf("Bug - null page2\n");
    if (point.y_ <= page->page_rect().bottom())
      return i;
  }
  return poppler_doc_->pages() - 1;
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

void DocumentView::InsertGraphicAfter(shared_ptr<Graphic> graphic,
                                      Graphic* upper_sibling) {
  graphic->SetDelegate(this);
  if (!upper_sibling) {
    if (top_graphic_) {
      top_graphic_->upper_sibling_ = graphic.get();
    } else {
      bottom_graphic_ = graphic.get();
    }
    graphic->lower_sibling_ = top_graphic_;
    graphic->upper_sibling_ = NULL;
    top_graphic_ = graphic;
  } else {
    if (upper_sibling->lower_sibling_.get())
      graphic->lower_sibling_ = upper_sibling->lower_sibling_;
    else
      graphic->lower_sibling_.reset();
    graphic->upper_sibling_ = upper_sibling;
    upper_sibling->lower_sibling_ = graphic;
  }
  graphic->SetNeedsDisplay(GraphicIsSelected(graphic.get()));
}

shared_ptr<Graphic> DocumentView::RemoveGraphic(Graphic* graphic) {
  graphic->SetNeedsDisplay(GraphicIsSelected(graphic));
  if (GraphicIsSelected(graphic)) {
    selected_graphics_.erase(selected_graphics_.find(graphic));
  }
  shared_ptr<Graphic> ret;
  if (graphic->upper_sibling_) {
    ret = graphic->upper_sibling_->lower_sibling_;
    graphic->upper_sibling_->lower_sibling_ = graphic->lower_sibling_;
  } else if (top_graphic_.get() == graphic) {
    ret = top_graphic_;
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
  graphic->upper_sibling_ = NULL;
  graphic->lower_sibling_.reset();

  return ret;
}

void DocumentView::DrawRect(cairo_t* cr, const Rect& rect) {
  // Do some caching
  if (VisibleSubrect() != cached_subrect_ && poppler_doc_.get()) {
    cached_subrect_ = VisibleSubrect();
    if (cached_surface_) {
      cairo_surface_finish(cached_surface_);
      cairo_surface_destroy(cached_surface_);
      cached_surface_ = NULL;
    }

    // Create new cache
    double width = cached_subrect_.size_.width_;
    double height = cached_subrect_.size_.height_;
    cairo_user_to_device_distance(cr, &width, &height);
    cached_surface_ =
        cairo_image_surface_create(CAIRO_FORMAT_ARGB32, width, height);

    cairo_t* cache_cr = cairo_create(cached_surface_);
    cairo_translate(cache_cr, -cached_subrect_.Left(), -cached_subrect_.Top());
    poppler::page_renderer renderer;
    for (int i = 0; i < poppler_doc_->pages(); i++) {
      Rect page_rect = PageRect(i);
      printf("page rect(%d): %s\n", i, page_rect.String().c_str());
      if (!cached_subrect_.Intersects(page_rect))
        continue;
      // draw this page
      cairo_save(cache_cr);
      cairo_set_source_rgb(cache_cr, 0.0, 0.0, 0.0);
      cairo_set_line_width(cache_cr, 1.0);
      Rect outline = page_rect.InsetBy(-0.5);
      outline.CairoRectangle(cache_cr);
      cairo_stroke(cache_cr);

      cairo_set_source_rgb(cache_cr, 1.0, 1.0, 1.0);
      page_rect.CairoRectangle(cache_cr);
      cairo_fill(cache_cr);
      cairo_translate(cache_cr, page_rect.origin_.x_, page_rect.origin_.y_);
      cairo_scale(cache_cr, zoom_, zoom_);
      printf("rendering page %d\n", i);

      unique_ptr<poppler::page> page(poppler_doc_->create_page(i));
      if (!page.get())
        printf("BUG- null page in render\n");
      renderer.cairo_render_page(cache_cr,
                                 page.get(),
                                 false);  // TODO(adlr): rotation?
      // doc_->RenderPage(i, false, cache_cr);

      printf("rendering page (done)\n");
      cairo_restore(cache_cr);
    }
    cairo_destroy(cache_cr);
  }

  if (cached_surface_) {
    cairo_pattern_t* old_pattern = cairo_pattern_reference(cairo_get_source(cr));
    cairo_save(cr);
    cairo_set_source_surface(cr, cached_surface_,
                             cached_subrect_.Left(),
                             cached_subrect_.Top());
    cairo_paint(cr);
    cairo_set_source(cr, old_pattern);
    cairo_pattern_destroy(old_pattern);
    cairo_restore(cr);
  }

  if (!poppler_doc_.get()) {
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

  for (int i = 0; i < poppler_doc_->pages(); i++) {
    Rect page_rect = PageRect(i);
    if (!rect.Intersects(page_rect))
      continue;
    // draw this page
    cairo_save(cr);

    // cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);
    // cairo_set_line_width(cr, 1.0);
    // Rect outline = page_rect.InsetBy(-0.5);
    // outline.CairoRectangle(cr);
    // cairo_stroke(cr);

    // cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
    // page_rect.CairoRectangle(cr);
    // cairo_fill(cr);

    cairo_translate(cr, page_rect.origin_.x_, page_rect.origin_.y_);
    cairo_scale(cr, zoom_, zoom_);

    // printf("rendering page\n");
    // doc_->RenderPage(i, false, cr);
    // printf("rendering page (done)\n");

    // Draw graphics
    for (Graphic* gr = bottom_graphic_; gr; gr = gr->upper_sibling_) {
      if (gr->Page() != i)
        continue;
      gr->Draw(cr, GraphicIsSelected(gr));
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
  if (!poppler_doc_.get()) {
    printf("can't export w/o a doc\n");
    return;
  }
  cairo_surface_t *surface = cairo_pdf_surface_create_for_stream(
      HandleCairoStreamWrite, out, 6 * 72, 6 * 72);
  cairo_t* cr = cairo_create(surface);
  poppler::page_renderer renderer;
  for (int i = 0; i < poppler_doc_->pages(); i++) {
    Size pg_size = PageSize(i);
    cairo_pdf_surface_set_size(surface, pg_size.width_, pg_size.height_);
    // for each page:
    cairo_save(cr);
    printf("rendering page\n");
    unique_ptr<poppler::page> page(poppler_doc_->create_page(i));
    renderer.cairo_render_page(cr,
                               page.get(),
                               true);  // TODO(adlr): rotation?
    // doc_->RenderPage(i, true, cr);
    printf("rendering complete\n");
    // Draw graphics
    for (Graphic* gr = bottom_graphic_; gr; gr = gr->upper_sibling_) {
      if (gr->Page() != i)
        continue;
      gr->Draw(cr, false);
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
    for (Graphic* gr = top_graphic_.get(); gr; gr = gr->lower_sibling_.get()) {
      if (selected_graphics_.find(gr) == selected_graphics_.end())
        continue;
      Point pos = ConvertPointToPage(event.position().TranslatedBy(0.5, 0.5),
                                     gr->Page());
      int knob = kKnobNone;
      printf("testing graphic for knob hit\n");
      if ((knob = gr->PointInKnob(pos)) != kKnobNone) {
        resize_graphic_original_frame_ = gr->Frame();
        resizing_graphic_ = gr;
        gr->BeginResize(pos, knob, false);
        return this;
      }
      printf("no hit\n");
    }
  }

  selected_graphics_.clear();

  if (editing_graphic_) {
    editing_graphic_->EndEditing();
    editing_graphic_ = NULL;
  }

  if (!toolbox_)
    return this;

  if (toolbox_->CurrentTool() == Toolbox::ARROW) {
    // See if we hit a graphic
    for (Graphic* gr = top_graphic_.get(); gr; gr = gr->lower_sibling_.get()) {
      Point page_pos = ConvertPointToPage(event.position().TranslatedBy(0.5, 0.5),
                                          gr->Page());
      if (gr->frame_.Contains(page_pos)) {
        if (event.ClickCount() == 1) {
          selected_graphics_.insert(gr);
          start_move_pos_ = last_move_pos_ =
              event.position().ScaledBy(1.0 / zoom_);
        } else if (event.ClickCount() == 2 &&
                   gr->Editable()) {
          if (editing_graphic_) {
            printf("Already editing!\n");
            return this;
          }
          editing_graphic_ = gr;
          gr->BeginEditing();
        }
        gr->SetNeedsDisplay(true);
        return this;
      }
    }
    return this;
  }

  shared_ptr<Graphic> gr = GraphicFactory::NewGraphic(toolbox_->CurrentTool());
  AddGraphic(gr);
  int page = PageForPoint(event.position());
  Point page_pos = ConvertPointToPage(event.position().TranslatedBy(0.5, 0.5),
                                      page);
  gr->Place(page, page_pos, false);
  placing_graphic_ = gr.get();
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
    printf("Move from %s to %s\n", last_move_pos_.String().c_str(),
           event.position().String().c_str());
    Point pos = event.position().ScaledBy(1.0 / zoom_);
    double dx = pos.x_ - last_move_pos_.x_;
    double dy = pos.y_ - last_move_pos_.y_;
    for (set<Graphic*>::iterator it = selected_graphics_.begin(),
             e = selected_graphics_.end(); it != e; ++it) {
      (*it)->SetNeedsDisplay(true);
      (*it)->frame_.origin_ = (*it)->frame_.origin_.TranslatedBy(dx, dy);
      (*it)->SetNeedsDisplay(true);
    }
    last_move_pos_ = pos;
  }
}

shared_ptr<Graphic> DocumentView::SharedPtrForGraphic(
    Graphic* graphic) const {
  if (!top_graphic_.get()) {
    printf("no graphics!\n");
    return shared_ptr<Graphic>();
  }
  if (graphic == top_graphic_.get())
    return top_graphic_;
  for (Graphic* gr = top_graphic_.get(); gr->lower_sibling_.get(); gr = gr->lower_sibling_.get()) {
    if (gr->lower_sibling_.get() == graphic)
      return gr->lower_sibling_;
  }
  printf("Error: %s called for missing graphic!\n", __func__);
  return shared_ptr<Graphic>();
}

void DocumentView::OnMouseUp(const MouseInputEvent& event) {
  if (resizing_graphic_) {
    resizing_graphic_->EndResize();

    // if (resize_graphic_original_frame_ != resizing_graphic_->Frame()) {
    //   // Generate undo op
    //   undo_manager_->AddClosure(
    //       [this, resizing_graphic_, resize_graphic_original_frame_] () {
    //         Rect old_frame = resizing_graphic_->Frame();
    //         resizing_graphic_->frame_ = resize_graphic_original_frame_;
    //         SetNeedsDisplay();
    //         // TODO(adlr): add next undo (redo)
    //       });
    // }

    resizing_graphic_ = NULL;
    return;
  }

  if (placing_graphic_) {
    placing_graphic_->SetNeedsDisplay(true);
    if (placing_graphic_->PlaceComplete()) {
      RemoveGraphic(placing_graphic_);
    } else {
      if (undo_manager_) {
        set<Graphic*> gr;
        gr.insert(placing_graphic_);
        undo_manager_->AddClosure(
            [this, gr] () {
              RemoveGraphicsUndo(gr);
            });
      }
      selected_graphics_.insert(placing_graphic_);
      if (placing_graphic_->Editable()) {
        placing_graphic_->BeginEditing();
        editing_graphic_ = placing_graphic_;
      }
    }
    placing_graphic_ = NULL;
    //toolbox_->SelectTool(Toolbox::ARROW);
  } else {
    // Moving
    if (last_move_pos_ == start_move_pos_)
      return;
    if (!undo_manager_) {
      printf("no undo manager\n");
      return;
    }
    // Generate undo op
    double dx = start_move_pos_.x_ - last_move_pos_.x_;
    double dy = start_move_pos_.y_ - last_move_pos_.y_;
    auto current_selected_graphics = selected_graphics_;
    undo_manager_->AddClosure(
        [this, current_selected_graphics, dx, dy] () {
          MoveGraphicsUndo(current_selected_graphics, dx, dy);

        });
  }
}

void DocumentView::MoveGraphicsUndo(const std::set<Graphic*>& graphics,
                                    double dx, double dy) {
  for (auto it = graphics.begin(), e = graphics.end();
       it != e; ++it) {
    (*it)->SetNeedsDisplay(true);
    (*it)->frame_.origin_ = (*it)->frame_.origin_.TranslatedBy(dx, dy);
    (*it)->SetNeedsDisplay(true);
  }
  if (!undo_manager_)
    return;
  undo_manager_->AddClosure(
      [this, graphics, dx, dy] () {
        MoveGraphicsUndo(graphics, -dx, -dy);
      });
}

void DocumentView::RemoveGraphicsUndo(set<Graphic*> graphics) {
  set<pair<shared_ptr<Graphic>, Graphic*> > pairs;
    for (set<Graphic*>::iterator it = graphics.begin(),
             e = graphics.end(); it != e; ++it) {
      (*it)->SetNeedsDisplay(true);
      Graphic* upper_sibling = (*it)->upper_sibling_;
      pairs.insert(make_pair(RemoveGraphic(*it), upper_sibling));
    }
    if (undo_manager_)
      undo_manager_->AddClosure(
          [this, pairs, graphics] () {
            for (auto graphic : pairs)
              InsertGraphicAfter(graphic.first, graphic.second);
            if (undo_manager_)
              undo_manager_->AddClosure(
                  [this, graphics] () {
                    RemoveGraphicsUndo(graphics);
                  });
          });
}

bool DocumentView::OnKeyText(const KeyboardInputEvent& event) {
  if (editing_graphic_) {
    editing_graphic_->OnKeyText(event);
    return true;
  }
  return false;
}

bool DocumentView::OnKeyDown(const KeyboardInputEvent& event) {
  if (editing_graphic_) {
    editing_graphic_->OnKeyDown(event);
    return true;
  }
  if (event.keycode() == 8 || event.keycode() == 46) {  // backspace, delete
    // delete selected graphics
    RemoveGraphicsUndo(selected_graphics_);
  }
  return true;
}

void DocumentView::SetNeedsDisplayInPageRect(int page, const Rect& rect) {
  Rect local(ConvertPointFromPage(rect.UpperLeft(), page),
             ConvertPointFromPage(rect.LowerRight(), page));
  SetNeedsDisplayInRect(local);
}

}  // namespace pdfsketch
