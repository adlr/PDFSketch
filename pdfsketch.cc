// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/// @file hello_tutorial.cc
/// This example demonstrates loading, running and scripting a very simple NaCl
/// module.  To load the NaCl module, the browser first looks for the
/// CreateModule() factory method (at the end of this file).  It calls
/// CreateModule() once to load the module code.  After the code is loaded,
/// CreateModule() is not called again.
///
/// Once the code is loaded, the browser calls the CreateInstance()
/// method on the object returned by CreateModule().  It calls CreateInstance()
/// each time it encounters an <embed> tag that references your NaCl module.
///
/// The browser can talk to your NaCl module via the postMessage() Javascript
/// function.  When you call postMessage() on your NaCl module from the browser,
/// this becomes a call to the HandleMessage() method of your pp::Instance
/// subclass.  You can send messages back to the browser by calling the
/// PostMessage() method on your pp::Instance.  Note that these two methods
/// (postMessage() in Javascript and PostMessage() in C++) are asynchronous.
/// This means they return immediately - there is no waiting for the message
/// to be handled.  This has implications in your program design, particularly
/// when mutating property values that are exposed to both the browser and the
/// NaCl module.

#include <stdio.h>
#include <string.h>
#include <vector>

#include <poppler/cpp/poppler-document.h>
#include <cairo.h>
#include <cairo-pdf.h>

#include "ppapi/cpp/completion_callback.h"
#include "ppapi/utility/completion_callback_factory.h"
#include "ppapi/utility/threading/simple_thread.h"
#include "ppapi/cpp/graphics_2d.h"
#include "ppapi/cpp/image_data.h"
#include "ppapi/cpp/instance.h"
#include "ppapi/cpp/module.h"
#include "ppapi/cpp/size.h"
#include "ppapi/cpp/var.h"
#include "ppapi/cpp/var_array_buffer.h"
#include "ppapi/c/ppb_image_data.h"

#include "root_view.h"

using std::vector;

cairo_status_t HandleCairoStreamWrite(void* closure,
                                      const unsigned char *data,
                                      unsigned int length) {
  vector<char>* out = reinterpret_cast<vector<char>*>(closure);
  out->insert(out->end(), data, data + length);
  return CAIRO_STATUS_SUCCESS;
}

void ModifyPDF(const char* orig, size_t orig_length, vector<char>* out) {
  poppler::SimpleDocument doc(orig, orig_length);
  cairo_surface_t *surface = cairo_pdf_surface_create_for_stream(
      HandleCairoStreamWrite, out, 6 * 72, 6 * 72);
  cairo_t* cr = cairo_create(surface);
  for (size_t i = 1; i <= 1; i++) {
    // for each page:
    cairo_save(cr);
    printf("rendering page\n");
    doc.RenderPage(i, true, cr);
    printf("rendering complete\n");
    cairo_restore(cr);
    cairo_surface_show_page(surface);
  }
  cairo_destroy(cr);
  cairo_surface_finish(surface);
  cairo_surface_destroy(surface);
  printf("modifyPDF done\n");
}

void MyCompletionCallback(void* user_data, int32_t result) {}

void ShowPDF(const char* orig, size_t orig_length, pp::Instance* inst) {
  pp::ImageData image_data(inst, PP_IMAGEDATAFORMAT_BGRA_PREMUL, pp::Size(300, 300), true);

  poppler::SimpleDocument doc(orig, orig_length);
  cairo_surface_t *surface =
      cairo_image_surface_create_for_data((unsigned char*)image_data.data(),
                                          CAIRO_FORMAT_ARGB32,
                                          300, 300,
                                          image_data.stride());
  cairo_t* cr = cairo_create(surface);
  for (size_t i = 1; i <= 1; i++) {
    // for each page:
    cairo_save(cr);
    printf("rendering page\n");
    doc.RenderPage(i, false, cr);
    printf("rendering complete\n");
    cairo_restore(cr);
    cairo_surface_show_page(surface);
  }
  cairo_destroy(cr);
  cairo_surface_finish(surface);
  cairo_surface_destroy(surface);

  // Paint to screen
  pp::Graphics2D* graphics = new pp::Graphics2D(inst, pp::Size(300, 300), false);  // leaks
  inst->BindGraphics(*graphics);
  //graphics->PaintImageData(image_data, pp::Point());
  graphics->ReplaceContents(&image_data);
  graphics->Flush(pp::CompletionCallback(MyCompletionCallback, NULL));
  printf("showPDF done\n");
}

class PDFSketchInstance;

class PDFRenderer : public pdfsketch::RootViewDelegate {
 public:
  PDFRenderer(PDFSketchInstance* instance)
      : doc_(NULL), instance_(instance), graphics_(NULL) {
    printf("got a new PDFRenderer going\n");
    root_view_.SetDelegate(this);
  }
  void SetPDF(const char* doc, size_t doc_len) {
    if (doc_)
      delete doc_;
    doc_ = new poppler::SimpleDocument(doc, doc_len);
    //Render();
  }
  void SetSize(const pp::Size& size) {
    printf("PDFRenderer got new view (doc: %d)\n", doc_ != NULL);
    size_ = size;
    //if (doc_)
    //  Render();
    root_view_.Resize(pdfsketch::Size(size_.width(), size_.height()));
  }
  void Render();

  virtual cairo_t* AllocateCairo();
  virtual void FlushCairo();

 private:
  pdfsketch::RootView root_view_;

  poppler::SimpleDocument* doc_;
  PDFSketchInstance* instance_;
  pp::Size size_;
  pp::Graphics2D* graphics_;

  pp::ImageData* image_data_;
  cairo_surface_t* surface_;
  cairo_t* cr_;
};

/// The Instance class.  One of these exists for each instance of your NaCl
/// module on the web page.  The browser will ask the Module object to create
/// a new Instance for each occurrence of the <embed> tag that has these
/// attributes:
///     src="hello_tutorial.nmf"
///     type="application/x-pnacl"
class PDFSketchInstance : public pp::Instance {
 public:
  explicit PDFSketchInstance(PP_Instance instance)
      : pp::Instance(instance),
        callback_factory_(this),
        render_thread_(this),
        renderer_(NULL) {
  }
  virtual ~PDFSketchInstance() {}

  virtual bool Init(uint32_t argc, const char *argn[], const char *argv[]) {
    printf("init called\n");
    renderer_ = new PDFRenderer(this);
    render_thread_.Start();
    return true;
  }

  virtual void DidChangeView(const pp::View& view) {
    printf("View did change\n");
    if (size_ == view.GetRect().size()) {
      printf("no size change\n");
      return;
    }
    size_ = view.GetRect().size();
    graphics_ =
        pp::Graphics2D(this, size_, false);
    BindGraphics(graphics_);
    render_thread_.message_loop().PostWork(
        callback_factory_.NewCallback(&PDFSketchInstance::SetSize,
                                      size_));
  }

  void Paint(int32_t result, const pp::ImageData& data) {
    if (data.size() != size_) {
      printf("Paint: bad size: skipping\n");
      return;
    }
    graphics_.PaintImageData(data, pp::Point());
    graphics_.Flush(pp::CompletionCallback(MyCompletionCallback, NULL));
  }

 private:
  void SetPDF(int32_t result, const pp::Var& doc) {
    printf("SEtPDF called\n");
    pp::VarArrayBuffer vab(doc);
    void* buf = vab.Map();
    size_t length = vab.ByteLength();
    renderer_->SetPDF((char*)buf, length);
    vab.Unmap();
    printf("SetPDF done\n");
  }

  void SetSize(int32_t result, const pp::Size& size) {
    renderer_->SetSize(size);
  }

  virtual void HandleMessage(const pp::Var& var_message) {
    if (var_message.is_array_buffer()) {
      pp::Var var_reply("got array buffer");
      PostMessage(var_reply);
      //pp::VarArrayBuffer vab(var_message);
      //void* buf = vab.Map();
      //size_t length = vab.ByteLength();
      //vector<char> out;
      PostMessage(pp::Var("about to modify PDF"));
      //PostMessage(pp::Var((int32_t)length));
      /*
      ModifyPDF((char*)buf, length, &out);
      printf("about to unmap\n");
      vab.Unmap();
      
      printf("sending new size\n");
      pp::VarArrayBuffer out_var(out.size());
      char* out_var_buf = (char*)out_var.Map();
      memcpy(out_var_buf, &out[0], out.size());
      out_var.Unmap();
      PostMessage(out_var);
      printf("handle message done\n");
      */

      //ShowPDF((char*)buf, length, this);

      //render_thread_.MainLoop()->PostWork
      printf("about to launch thread\n");
      render_thread_.message_loop().PostWork(
          callback_factory_.NewCallback(&PDFSketchInstance::SetPDF,
                                        var_message));
      printf("thread launched\n");

      return;
    }

    // Ignore the message if it is not a string.
    if (!var_message.is_string())
      return;

    // Get the string message and compare it to "hello".
    std::string message = var_message.AsString();
    if (message == "hello") {
      // If it matches, send our response back to JavaScript.
      pp::Var var_reply("got hello");
      PostMessage(var_reply);
    }    
  }

 public:
  pp::CompletionCallbackFactory<PDFSketchInstance> callback_factory_;
 private:
  pp::Size size_;
  pp::SimpleThread render_thread_;
  PDFRenderer* renderer_;
  pp::Graphics2D graphics_;
};

cairo_t* PDFRenderer::AllocateCairo() {
  if (cr_) {
    printf("Already have Cairo!\n");
    return NULL;
  }
  if (image_data_) {
    printf("Bug: image_data_ already allocated\n");
    return NULL;
  }
  image_data_ = new pp::ImageData(instance_,
                                  PP_IMAGEDATAFORMAT_BGRA_PREMUL,
                                  size_,
                                  true);
  surface_ =
      cairo_image_surface_create_for_data((unsigned char*)image_data_->data(),
                                          CAIRO_FORMAT_ARGB32,
                                          size_.width(),
                                          size_.height(),
                                          image_data_->stride());
  cr_ = cairo_create(surface_);
  return cr_;
}

void PDFRenderer::FlushCairo() {
  cairo_destroy(cr_);
  cr_ = NULL;
  cairo_surface_finish(surface_);
  cairo_surface_destroy(surface_);
  surface_ = NULL;

  pp::Module::Get()->core()->CallOnMainThread(
      0,
      instance_->callback_factory_.NewCallback(
          &PDFSketchInstance::Paint, *image_data_));
  delete image_data_;
  image_data_ = NULL;
}

void PDFRenderer::Render() {
  pp::ImageData image_data(instance_,
                           PP_IMAGEDATAFORMAT_BGRA_PREMUL,
                           size_,
                           true);
  cairo_surface_t *surface =
      cairo_image_surface_create_for_data((unsigned char*)image_data.data(),
                                          CAIRO_FORMAT_ARGB32,
                                          size_.width(),
                                          size_.height(),
                                          image_data.stride());
  cairo_t* cr = cairo_create(surface);
  for (size_t i = 1; i <= 1; i++) {
    // for each page:
    cairo_save(cr);
    printf("rendering page - test\n");
    doc_->RenderPage(i, false, cr);
    printf("rendering complete\n");
    cairo_restore(cr);
    cairo_surface_show_page(surface);
  }
  cairo_destroy(cr);
  cairo_surface_finish(surface);
  cairo_surface_destroy(surface);
  
  pp::Module::Get()->core()->CallOnMainThread(
      0,
      instance_->callback_factory_.NewCallback(
          &PDFSketchInstance::Paint, image_data));    
}

/// The Module class.  The browser calls the CreateInstance() method to create
/// an instance of your NaCl module on the web page.  The browser creates a new
/// instance for each <embed> tag with type="application/x-pnacl".
class HelloTutorialModule : public pp::Module {
 public:
  HelloTutorialModule() : pp::Module() {}
  virtual ~HelloTutorialModule() {}

  /// Create and return a HelloTutorialInstance object.
  /// @param[in] instance The browser-side instance.
  /// @return the plugin-side instance.
  virtual pp::Instance* CreateInstance(PP_Instance instance) {
    return new PDFSketchInstance(instance);
  }
};

namespace pp {
/// Factory function called by the browser when the module is first loaded.
/// The browser keeps a singleton of this module.  It calls the
/// CreateInstance() method on the object you return to make instances.  There
/// is one instance per <embed> tag on the page.  This is the main binding
/// point for your NaCl module with the browser.
Module* CreateModule() {
  return new HelloTutorialModule();
}
}  // namespace pp
