// Copyright...

#include <stdio.h>

#include "scroll_view.h"

namespace pdfsketch {

ScrollView::ScrollView()
    : document_(NULL),
      h_scroller_(false),
      h_visible_(false),
      v_scroller_(true),
      v_visible_(false) {
  h_scroller_.SetResizeParams(false, false, true, false);
  h_scroller_.SetDelegate(this);
  v_scroller_.SetResizeParams(true, false, false, false);
  v_scroller_.SetDelegate(this);
  AddSubview(&clip_view_);
}

void ScrollView::SetDocumentView(View* document) {
  if (document_) {
    printf("already have document!\n");
    return;
  }
  document_ = document;
  document_->SetDelegate(this);
  clip_view_.AddSubview(document);
  document->SetOrigin(Point());
  h_scroller_.SetDocSize(0.0, document_->size().width_);
  v_scroller_.SetDocSize(0.0, document_->size().height_);
}

void ScrollView::RepositionSubviews() {
  if (!document_) {
    // nothing to really do
    return;
  }

  // todo: update doc size w/ event, not every call here
  h_scroller_.SetDocSize(0.0, document_->size().width_);
  v_scroller_.SetDocSize(0.0, document_->size().height_);

  bool use_h = false;
  bool use_v = false;

  // Which scroll bars are needed?
  if (document_->size().height_ <= size_.height_ &&
      document_->size().width_ <= size_.width_) {
    // no scroll bars needed
  } else if (document_->size().height_ > size_.height_ &&
             document_->size().width_ <=
             size_.width_ - ScrollBarView::kThickness) {
    use_v = true;
  } else if (document_->size().width_ > size_.width_ &&
             document_->size().height_ <=
             size_.height_ - ScrollBarView::kThickness) {
    use_h = true;
  } else {
    use_v = use_h = true;
  }

  if (use_h != h_visible_) {
    if (use_h) {
      AddSubview(&h_scroller_);
    } else {
      RemoveSubview(&h_scroller_);
    }
  }
  if (use_v != v_visible_) {
    if (use_v) {
      AddSubview(&v_scroller_);
    } else {
      RemoveSubview(&v_scroller_);
    }
  }
  h_scroller_.SetShowSize(size_.width_ -
                          (use_v ? ScrollBarView::kThickness : 0.0));
  v_scroller_.SetShowSize(size_.height_ -
                          (use_h ? ScrollBarView::kThickness : 0.0));

  Rect h_scroller_frame =
      Rect(0.0, size_.height_ - ScrollBarView::kThickness,
           size_.width_ - (use_v ? ScrollBarView::kThickness : 0.0),
           ScrollBarView::kThickness);
  h_scroller_.SetFrame(h_scroller_frame);
  Rect v_scroller_frame =
      Rect(size_.width_ - ScrollBarView::kThickness, 0.0,
           ScrollBarView::kThickness,
           size_.height_ - (use_h ? ScrollBarView::kThickness : 0.0));
  v_scroller_.SetFrame(v_scroller_frame);

  printf("v scroll frame: %s\n", v_scroller_.Frame().String().c_str());

  h_visible_ = use_h;
  v_visible_ = use_v;

  Rect clip_frame =
      Rect(0.0,
           0.0,
           size_.width_ - (use_v ? ScrollBarView::kThickness : 0.0),
           size_.height_ - (use_h ? ScrollBarView::kThickness : 0.0));
  clip_view_.SetFrame(clip_frame);
  SetNeedsDisplay();
}

void ScrollView::Resize(const Size& size) {
  printf("scroll view: resize to: %f x %f\n", size.width_, size.height_);
  SetSize(size);
  RepositionSubviews();
}

void ScrollView::ScrollBarMovedTo(ScrollBarView* scroll_bar, double show_min) {
  if (!document_)
    return;
  show_min = static_cast<int>(show_min);
  Rect doc_frame = document_->Frame();
  if (scroll_bar == &h_scroller_) {
    doc_frame.origin_.x_ = -show_min;
  } else {
    doc_frame.origin_.y_ = -show_min;
  }
  document_->SetFrame(doc_frame);
  printf("doc height: %f bot: %f\n", document_->size().height_,
         clip_view_.size().height_ + show_min);
}

void ScrollView::ViewFrameChanged(View* view, const Rect& frame) {
  if (view == document_)
    RepositionSubviews();
}

}  // namespace pdfsketch
