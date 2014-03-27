// Copyright...

#ifndef PDFSKETCH_SCROLL_BAR_VIEW_H__
#define PDFSKETCH_SCROLL_BAR_VIEW_H__

#include "view.h"

namespace pdfsketch {

class ScrollBarView;

class ScrollBarDelegate {
 public:
  virtual void ScrollBarMovedTo(ScrollBarView* scroll_bar, double show_min) = 0;
};

class ScrollBarView : public View {
 public:
  static const double kThickness;

  ScrollBarView(bool vertical)
      : vertical_(vertical),
        delegate_(NULL),
        drag_start_pos_(0.0),
        drag_start_show_min_(0.0) {}
  virtual std::string Name() const { return "ScrollBarView"; }
  virtual void DrawRect(cairo_t* cr, const Rect& rect);
  void SetDocSize(double min, double max) {
    doc_min_ = min;
    doc_max_ = max;
  }
  void SetShowSize(double size) { show_size_ = size; }
  void SetShowMin(double min) { show_min_ = min; }
  void SetDelegate(ScrollBarDelegate* delegate) {
    delegate_ = delegate;
  }
  virtual View* OnMouseDown(const MouseInputEvent& event);
  virtual void OnMouseDrag(const MouseInputEvent& event);
  virtual void OnMouseUp(const MouseInputEvent& event);

  void ScrollBy(double delta);

 private:
  bool vertical_;

  // the doc min/max are the size of the document that's being scrolled.
  // A subset (show_min_ to show_min_+show_size_) is visible.
  // When scrolled, show_min_ is reported to the delegate.
  double doc_min_;
  double doc_max_;
  double show_size_;
  double show_min_;
  ScrollBarDelegate* delegate_;

  double drag_start_pos_;
  double drag_start_show_min_;
};

}  // namespace pdfsketch

#endif  // PDFSKETCH_SCROLL_BAR_VIEW_H__
