// Copyright...

#include "scroll_view.h"

namespace pdfsketch {

ScrollView::ScrollView()
    : document_(NULL),
      h_scroller_(false),
      h_visible_(false),
      v_scroller_(true),
      v_visible_(false) {
  h_scroller_.top_fixed_to_top_ = false;
  h_scroller_.bot_fixed_to_top_ = false;
  h_scroller_.left_fixed_to_left_ = true;
  h_scroller_.right_fixed_to_left_ = false;

  v_scroller_.top_fixed_to_top_ = true;
  v_scroller_.bot_fixed_to_top_ = false;
  v_scroller_.left_fixed_to_left_ = false;
  v_scroller_.right_fixed_to_left_ = false;

  AddSubview(&clip_view_);
}

ScrollView::SetDocumentView(View* document) {
  if (document_) {
    printf("already have document!\n");
    return;
  }
  document_ = document;
  clip_view_.AddSubview(document);
  document.origin_ = Point();
}

ScrollView::RepositionSubviews() {
  if (!document_) {
    // nothing to really do
    return;
  }
  bool use_h = false;
  bool use_v = false;

  // Which scroll bars are needed?
  if (document_.size_.height_ <= size_.height_ &&
      document_.size_.width_ <= size_.width_) {
    // no scroll bars needed
  } else if (document_.size_.height_ <= size_.height_ &&
             document_.size_.width_ <=
             size_.width_ - ScrollBarView::kThickness) {
    use_v = true;
  } else if (document_.size_.width_ <= size_.width_ &&
             document_.size_.height_ <=
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

  if (use_h != h_visible_ || use_v != v_visible_) {
    Rect h_scroller_frame =
        Rect(0.0, size_.height_ - ScrollBarView::kThickness,
             size_.width_, ScrollBarView::kThickness);
    h_scroller_.SetFrame(h_scroller_frame);
    Rect h_scroller_frame =
        Rect(size_.width_ - ScrollBarView::kThickness, 0.0,
             ScrollBarView::kThickness, size_.height_);
    v_scroller_.SetFrame(v_scroller_frame);
  }

  h_visible_ = use_h;
  v_visible_ = use_v;

  Rect clip_frame =
      Rect(0.0,
           0.0,
           size_.width_ - (use_h ? ScrollBarView::kThickness : 0.0),
           size_.height_ - (use_v ? ScrollBarView::kThickness : 0.0));
  clip_view_.SetFrame(clip_frame);
  SetNeedsDisplay();
}

}  // namespace pdfsketch
