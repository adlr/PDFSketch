// Copyright...

#include <stdio.h>

#include "scroll_view.h"

namespace pdfsketch {

ScrollView::ScrollView() {
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

void ScrollView::MoveDocPointToVisibleCenter(const Point& center) {
  if (h_visible_) {
    h_scroller_.CenterDocValue(center.x_);
  }
  if (v_visible_) {
    v_scroller_.CenterDocValue(center.y_);
  }
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

void ScrollView::ViewFrameChanged(View* view, const Rect& frame,
                                  const Rect& old_frame) {
  if (view && view == document_) {
    RepositionSubviews();
    if (old_frame.size_ != frame.size_ &&
        doc_size_.width_ && doc_size_.height_) {
      // Zoom event
      double x_ratio = doc_visible_center_.x_ / doc_size_.width_;
      double y_ratio = doc_visible_center_.y_ / doc_size_.height_;
      // Try to keep the visible center of the doc in the same place
      Point new_center(x_ratio * document_->size().width_,
                       y_ratio * document_->size().height_);
      printf("Zoom from: %s %s to %s (new size %s)\n",
             doc_visible_center_.String().c_str(),
             doc_size_.String().c_str(),
             new_center.String().c_str(),
             document_->size().String().c_str());
      MoveDocPointToVisibleCenter(new_center);
      printf("Zoom gave me: %s\n",
             document_->VisibleSubrect().Center().String().c_str());
    }
    doc_visible_center_ = document_->VisibleSubrect().Center();
    doc_size_ = document_->size();
  }
}

void ScrollView::OnScrollEvent(const ScrollInputEvent& event) {
  if (h_visible_ && event.dx())
    h_scroller_.ScrollBy(-event.dx());
  if (v_visible_ && event.dy())
    v_scroller_.ScrollBy(-event.dy());
}

}  // namespace pdfsketch
