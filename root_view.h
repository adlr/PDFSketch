// Copyright...

#include "view.h"

namespace pdfsketch {

class RootViewDelegate {
 public:
  virtual cairo_t* AllocateCairo() = 0;
  virtual void FlushCairo() = 0;
};

class RootView : public View {
 public:
  virtual void DrawRect(cairo_t* ctx, const Rect& rect);
  virtual void SetNeedsDisplayInRect(const Rect& rect);
  void SetDelegate(RootViewDelegate* delegate) {
    delegate_ = delegate;
  }
  virtual void Resize(const Size& size);

 private:
  RootViewDelegate* delegate_;
};

}  // namespace pdfsketch
