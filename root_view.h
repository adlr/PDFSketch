// Copyright...

#ifndef PDFSKETCH_ROOT_VIEW_H__
#define PDFSKETCH_ROOT_VIEW_H__

#include <ppapi/utility/completion_callback_factory.h>
#include <ppapi/cpp/message_loop.h>
#include <ppapi/cpp/input_event.h>

#include "view.h"

namespace pdfsketch {

class RootViewDelegate {
 public:
  virtual cairo_t* AllocateCairo() = 0;
  virtual bool FlushCairo(std::function<void(int32_t)> complete_callback) = 0;
};

class RootView : public View {
 public:
  RootView()
      : draw_requested_(false),
        flush_in_progress_(false),
        callback_factory_(this),
        down_mouse_handler_(NULL) {}
  virtual std::string Name() const { return "RootView"; }
  virtual void DrawRect(cairo_t* ctx, const Rect& rect);
  virtual void SetNeedsDisplayInRect(const Rect& rect);
  void SetDelegate(RootViewDelegate* delegate) {
    delegate_ = delegate;
  }
  virtual void Resize(const Size& size);
  void HandleDrawRequest(int32_t result);
  void FlushComplete();
  void HandlePepperInputEvent(const pp::InputEvent& event,
                              float scale);

 private:
  RootViewDelegate* delegate_;
  bool draw_requested_;
  bool flush_in_progress_;
  pp::CompletionCallbackFactory<RootView> callback_factory_;
  View* down_mouse_handler_;
};

}  // namespace pdfsketch

#endif  // PDFSKETCH_ROOT_VIEW_H__
