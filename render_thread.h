// Copyright header

#include "ppapi/cpp/instance.h"
#include "ppapi/cpp/paint_manager.h"

namespace pdfsketch {

class RenderThread :: public pp::PaintManager::Client {
 public:
  RenderThread(pp::Instance* instance);
  pp::MessageLoop* MainLoop() const { return thread_loop_; }

  void SetPDF(const char* data, size_t length);

  // PaintManager callback
  virtual bool OnPaint(pp::Graphics2D& graphics,
                       const std::vector<pp::Rect>& paint_rects,
                       const Rect& paint_bounds);

 private:
  pp::Instance* instance_;
  pp::MessageLoop* thread_loop_;
  pp::PaintManager* paint_manager_;
};

}  // namespace pdfsketch
