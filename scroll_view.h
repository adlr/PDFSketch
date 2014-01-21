// Copyright...

#ifndef PDFSKETCH_SCROLL_VIEW_H__
#define PDFSKETCH_SCROLL_VIEW_H__

#include "scroll_bar_view.h"
#include "view.h"

namespace pdfsketch {

class ScrollView : public View,
                   public ScrollBarDelegate {
 public:
  ScrollView();
  // Adds document as a subview of this:
  void SetDocumentView(View* document);

  virtual void Resize(const Size& size);

  // ScrollBarDelegate method:
  virtual void ScrollBarMovedTo(ScrollBarView* scroll_bar, double show_min);

 private:
  void RepositionSubviews();

  View* document_;
  ScrollBarView h_scroller_;
  bool h_visible_;
  ScrollBarView v_scroller_;
  bool v_visible_;

  View clip_view_;
};

}  // namespace pdfsketch

#endif  // PDFSKETCH_SCROLL_VIEW_H__
