// Copyright...

// This is the bulk of the glue that connects the core PDFSketch code
// to the NaCl API. Some even handling happens in RootView, but the
// bulk is here.

// Generally everything happens on the render thread, with the main
// thread being only used for accepting calls from NaCl. They are then
// bounced to the render thread for processing.

#include <string>
#include <vector>

#include <ppapi/cpp/graphics_2d.h>
#include <ppapi/cpp/image_data.h>
#include <ppapi/cpp/instance.h>
#include <ppapi/cpp/size.h>
#include <ppapi/utility/threading/simple_thread.h>

#include "document_view.h"
#include "root_view.h"
#include "scroll_view.h"
#include "toolbox.h"
#include "undo_manager.h"

class PDFSketchInstance : public pp::Instance,
                          public pdfsketch::RootViewDelegate,
                          public pdfsketch::ToolboxDelegate,
                          public pdfsketch::UndoManagerDelegate {
 public:
  explicit PDFSketchInstance(PP_Instance instance);
  virtual ~PDFSketchInstance() {}

  void SetCrosshairCursor();
  virtual bool Init(uint32_t argc, const char *argn[],
                    const char *argv[]);
  virtual void DidChangeView(const pp::View& view);
  void PostStr(const char* str);
  void ExecOnRenderThread(std::function<void ()> func);
  void Exec(int32_t resize, std::function<void ()> func);
  void Paint(int32_t result, const pp::ImageData& data);
  void SendPDFOut(const std::vector<char>& out);
  void PostMessageOut(int32_t result, const std::string& msg);

 private:
  void SetPDF(const pp::Var& doc);
  void SaveFile();
  void ExportPDF();
  virtual bool HandleInputEvent(const pp::InputEvent& event);
  virtual void HandleMessage(const pp::Var& var_message);
  void RunOnMainThread(std::function<void ()> func);
  void RunOnRenderThread(std::function<void ()> func);

  bool ListAndRemove(const char* dir);
  virtual void ToolSelected(pdfsketch::Toolbox::Tool tool);
  virtual void SetUndoEnabled(bool enabled);
  virtual void SetRedoEnabled(bool enabled);
  int SetupFS();
  void SetPDF(const char* doc, size_t doc_len);
  void SetSize(const pp::Size& size, float scale);
  virtual cairo_t* AllocateCairo();
  virtual bool FlushCairo(std::function<void(int32_t)> complete_callback);
  virtual void CopyToClipboard(const std::string& str);
  virtual void RequestPaste();

 public:
  pp::CompletionCallbackFactory<PDFSketchInstance> callback_factory_;
  pp::Size size_;
  float scale_;
  pp::SimpleThread render_thread_;
  pp::Graphics2D graphics_;

  bool setup_;
  pdfsketch::RootView root_view_;
  pdfsketch::ScrollView scroll_view_;
  pdfsketch::DocumentView document_view_;
  pdfsketch::Toolbox toolbox_;
  pdfsketch::UndoManager undo_manager_;

  pp::ImageData* image_data_;
  cairo_surface_t* surface_;
  cairo_t* cr_;
};
