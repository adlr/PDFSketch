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

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <functional>
#include <stdio.h>
#include <string.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <vector>

#include <podofo/doc/PdfMemDocument.h>
#include <podofo/doc/PdfFileSpec.h>
#include <poppler/cpp/poppler-document.h>
#include <poppler/cpp/poppler-embedded-file.h>
#include <cairo.h>
#include <cairo-pdf.h>
#include <libtar.h>

#include "nacl_io/nacl_io.h"

#include "ppapi/cpp/completion_callback.h"
#include "ppapi/utility/completion_callback_factory.h"
#include "ppapi/utility/threading/simple_thread.h"
#include "ppapi/cpp/graphics_2d.h"
#include "ppapi/cpp/image_data.h"
#include "ppapi/cpp/instance.h"
#include "ppapi/cpp/module.h"
#include "ppapi/cpp/mouse_cursor.h"
#include "ppapi/cpp/size.h"
#include "ppapi/cpp/var.h"
#include "ppapi/cpp/var_array_buffer.h"
#include "ppapi/c/ppb_image_data.h"
#include "ppapi/c/ppb_mouse_cursor.h"

#include "document_view.h"
#include "file_io.h"
#include "page_view.h"
#include "root_view.h"
#include "scroll_bar_view.h"
#include "scroll_view.h"

using std::unique_ptr;
using std::vector;

class PDFSketchInstance;
PDFSketchInstance* g_inst_ = NULL;

void RunOnMainThread(std::function<void ()> func);
void RunOnRenderThread(std::function<void ()> func);

namespace pdfsketch {
void Dbg(const char* str);
}
using pdfsketch::Dbg;

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

void FlushCompletionCallback(void* user_data, int32_t result) {
  std::function<void (int32_t)>* func_p =
      reinterpret_cast<std::function<void (int32_t)>*>(user_data);
  std::function<void (int32_t)> func = *func_p;
  RunOnRenderThread([func, result]() { func(result); });
  delete func_p;
}

void MyCompletionCallback(void* user_data, int32_t result) {
  // if (user_data) {
  //   std::pair<pp::Instance*, const char*>* mypair = reinterpret_cast<std::pair<pp::Instance*, const char*>*>(user_data);
  //   mypair->first->PostMessage(pp::Var(mypair->second));
  //   delete mypair;
  // }
}

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

void Execute(std::function<void ()> func) {
  printf("EXECUTE start\n");
  func();
  printf("EXECUTE done\n");
}

class MyFoo {
 public:
  std::function<void ()> Set(int num) {
    printf("Setting[%d] to %d\n", id_, num);
    int newnum = 1 - num;
    return [this, newnum] () { Set(newnum); };
  }
  int id_;
};

void CXX11Test() {
  MyFoo foo;
  foo.id_ = 4;
  auto closure = foo.Set(1);
  foo.id_ = 3;
  Execute(closure);
}

class PDFSketchInstance;

class PDFRenderer : public pdfsketch::RootViewDelegate,
                    public pdfsketch::ToolboxDelegate,
                    public pdfsketch::UndoManagerDelegate {
 public:
  PDFRenderer(PDFSketchInstance* instance)
      : setup_(false),
        doc_(NULL), instance_(instance),
        scale_(1.0),
        image_data_(NULL),
        surface_(NULL),
        cr_(NULL) {
    printf("got a new PDFRenderer going\n");

    toolbox_.SetDelegate(this);
    printf("%s:%d\n", __FILE__, __LINE__);
    undo_manager_.SetDelegate(this);
    printf("%s:%d\n", __FILE__, __LINE__);

    root_view_.SetDelegate(this);
    printf("%s:%d\n", __FILE__, __LINE__);

    root_view_.AddSubview(&scroll_view_);
    printf("%s:%d\n", __FILE__, __LINE__);
    document_view_.SetToolbox(&toolbox_);
    printf("%s:%d\n", __FILE__, __LINE__);
    document_view_.SetUndoManager(&undo_manager_);
    printf("%s:%d\n", __FILE__, __LINE__);
    scroll_view_.SetDocumentView(&document_view_);
    printf("%s:%d\n", __FILE__, __LINE__);
    scroll_view_.SetResizeParams(true, false, true, false);
    printf("%s:%d\n", __FILE__, __LINE__);
    scroll_view_.SetFrame(root_view_.Frame());
    printf("%s:%d\n", __FILE__, __LINE__);
    document_view_.Resize(pdfsketch::Size(800.0, 2000.0));
    printf("%s:%d\n", __FILE__, __LINE__);
  }

  bool ListAndRemove(const char* dir) {
    printf("ListAndRemove(%s) start\n", dir);
    DIR* dirp = opendir(dir);
    if (!dirp) {
      printf("unable to open dir %s\n", dir);
      return false;
    }
    bool ret = false;
    while (true) {
      struct dirent* result = NULL;
      result = readdir(dirp);
      if (!result) {
        ret = true;
        goto exit;
      }
      if (!strcmp(result->d_name, ".") || !strcmp(result->d_name, ".."))
        continue;
      struct stat stbuf;

      vector<char> name_backing(strlen(dir) + strlen(result->d_name) + 2);
      char* name = &name_backing[0];
      memset(name, 0, name_backing.size());
      strcat(name, dir);
      strcat(name, "/");
      strcat(name, result->d_name);

      int rc = stat(name, &stbuf);
      if (rc < 0) {
        printf("unable to stat %s\n", name);
        goto exit;
      }
      printf("RM: %s (%s)\n", name, !S_ISDIR(stbuf.st_mode) ? "file" : "dir");
      if (!S_ISDIR(stbuf.st_mode)) {
        unlink(name);
      } else {
        ListAndRemove(name);
        printf("RMDIR: %s\n", name);
        rmdir(name);
      }
    }
 exit:
    closedir(dirp);
    printf("ListAndRemove(%s) end\n", dir);
    return ret;
  }

  virtual void ToolSelected(pdfsketch::Toolbox::Tool tool) {
    std::string message =
        std::string("ToolSelected:") + pdfsketch::Toolbox::ToolAsString(tool);
    PostMessage(message);
  }
  void PostMessage(const std::string& message);

  void SaveFile();
  void ExportPDF();

  void Undo() {
    undo_manager_.PerformUndo();
  }
  void Redo() {
    undo_manager_.PerformRedo();
  }
  virtual void SetUndoEnabled(bool enabled) {
    PostMessage(enabled ? "undoEnabled:true" : "undoEnabled:false");
  }
  virtual void SetRedoEnabled(bool enabled) {
    PostMessage(enabled ? "redoEnabled:true" : "redoEnabled:false");
  }

  int SetupFS() {
    printf("calling umount\n");
    int ret = umount("/");
    printf("umount ret: %d\n", ret);
    if (ret) {
      printf("unmounting root fs failed\n");
      return 1;
    }
    ret = mount("", "/", "memfs", 0, NULL);
    printf("mount ret: %d\n", ret);
    if (ret) {
      printf("mounting root fs failed\n");
      return 1;
    }

    printf("calling mkdirs\n");
    mkdir("/mnt", 0777);
    mkdir("/mnt/http", 0777);
    mkdir("/mnt/html5", 0777);
    printf("calling getenv\n");

    const char* data_url = getenv("NACL_DATA_URL");
    if (!data_url)
      data_url = "./";

    printf("mounting http\n");
    ret = mount(data_url, "/mnt/http", "httpfs", 0,
                "");
    //"allow_cross_origin_requests=true,allow_credentials=false");
    if (ret) {
      printf("mounting http filesystem failed\n");
      return 1;
    }
    printf("http mounted\n");
    
    // ret = mount("",                                       /* source */
    //       "/mnt/html5",                             /* target */
    //       "html5fs",                                /* filesystemtype */
    //       0,                                        /* mountflags */
    //       "type=PERSISTENT,expected_size=10000000"); /* data */    
    // if (ret) {
    //   printf("html5 mount failed\n");
    //   return 1;
    // }

    ListAndRemove("/mnt/html5");

    TAR* tar = NULL;
    const char kTarPath[] = "/mnt/http/system.tar";
    char tar_path[sizeof(kTarPath)];
    memcpy(tar_path, kTarPath, sizeof(kTarPath));
    ret = tar_open(&tar, tar_path, NULL, O_RDONLY, 0, 0);
    if (ret) {
      printf("tar open failed\n");
      return 1;    }
    const char kPrefix[] = "/";
    char prefix[sizeof(kPrefix)];
    memcpy(prefix, kPrefix, sizeof(kPrefix));
    ret = tar_extract_all(tar, prefix);
    if (ret) {
      printf("tar extract failed: %s\n", strerror(errno));
      tar_close(tar);
      return 1;
    }
    ret = tar_close(tar);
    if (ret) {
      printf("tar close failed\n");
      return 1;
    }

    // printf("calling open\n");
    // int fd = open("/mnt/http/datafile.txt", O_RDONLY, 0);
    // if (fd < 0) {
    //   int err = errno;
    //   printf("open file failed: %d\n", err);
    //   return 1;
    // }
    // char buf[100];
    // printf("calling read\n");
    // int ret = read(fd, buf, sizeof(buf));
    // if (ret < 0) {
    //   printf("read failed\n");
    // }
    // buf[ret] = '\0';
    // printf("read data:%s\n", buf);
    return 0;
  }

  void SetPDF(const char* doc, size_t doc_len) {
    //if (doc_)
    //  delete doc_;
    //doc_ = new poppler::SimpleDocument(doc, doc_len);
    //Render();
    //page_view_ = new pdfsketch::PageView(doc, doc_len);
    //root_view_.AddSubview(page_view_);
    const char kMagic[] = {'s', 'k', 'c', 'h'};
    if (doc_len < sizeof(kMagic))
      return;
    if (!strncmp(doc, kMagic, sizeof(kMagic))) {
      printf("loading saved file\n");
      pdfsketch::FileIO::Open(doc, doc_len, &document_view_);
    } else {
      // Loading PDF. Check for embedded save file

      unique_ptr<poppler::document> poppler_doc(
          poppler::document::load_from_raw_data(doc, doc_len));
      if (!poppler_doc.get()) {
        printf("can't make poppler doc from data\n");
        return;
      }
      if (poppler_doc->has_embedded_files()) {
        for (auto file : poppler_doc->embedded_files()) {
          if (file->is_valid() &&
              file->name() == "source.pdfsketch" &&
              file->size() > sizeof(kMagic)) {
            printf("loading embedded save file\n");
            vector<char> data = file->data();
            pdfsketch::FileIO::Open(&data[0], data.size(), &document_view_);
            return;
          }
        }
      }
      printf("no save file found. making new doc\n");
      document_view_.LoadFromPDF(doc, doc_len);
    }
  }
  void SetZoom(double zoom) {
    document_view_.SetZoom(zoom);
  }
  void SelectTool(const std::string& tool) {
    toolbox_.SelectTool(tool);
  }
  void SetSize(const pp::Size& size, float scale) {
    printf("PDFRenderer got new view (doc: %d)\n", doc_ != NULL);
    scale_ = scale;
    size_ = size;
    //if (doc_)
    //  Render();
    scroll_view_.SetScale(scale_);
    root_view_.Resize(pdfsketch::Size(size_.width() * scale_,
                                      size_.height() * scale_));
    if (!setup_) {
      setup_ = true;
      SetupFS();
    }
  }
  void Render();
  void HandleInputEvent(const pp::InputEvent& event) {
    root_view_.HandlePepperInputEvent(event, scale_);
  }

  virtual cairo_t* AllocateCairo();
  virtual bool FlushCairo(std::function<void(int32_t)> complete_callback);

 private:
  bool setup_;
  pdfsketch::RootView root_view_;
  //pdfsketch::PageView* page_view_;
  pdfsketch::ScrollView scroll_view_;
  pdfsketch::DocumentView document_view_;

  pdfsketch::Toolbox toolbox_;
  pdfsketch::UndoManager undo_manager_;

  poppler::SimpleDocument* doc_;
  PDFSketchInstance* instance_;
  float scale_;
  pp::Size size_;
  //pp::Graphics2D* graphics_;

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

  void SetCrosshairCursor() {
    pp::Size size(17, 17);
    pp::ImageData* image_data =
        new pp::ImageData(this,
                          PP_IMAGEDATAFORMAT_BGRA_PREMUL,
                          size,
                          true);
    cairo_surface_t* surface =
        cairo_image_surface_create_for_data(
            (unsigned char*)image_data->data(),
            CAIRO_FORMAT_ARGB32,
            size.width(),
            size.height(),
            image_data->stride());
    cairo_t* cr = cairo_create(surface);
    cairo_set_line_width(cr, 1.0);
    cairo_set_line_join(cr, CAIRO_LINE_JOIN_MITER);
    cairo_set_line_cap(cr, CAIRO_LINE_CAP_SQUARE);
    cairo_move_to(cr, size.width() / 2 + 0.5, 0.5);
    cairo_line_to(cr, size.width() / 2 + 0.5, size.height() - 1.0);
    cairo_move_to(cr, 0.5, size.height() / 2 + 0.5);
    cairo_line_to(cr, 0.5 + size.width() - 1.0,
                  size.height() / 2 + 0.5);
    cairo_stroke(cr);
    cairo_destroy(cr);
    cr = NULL;
    cairo_surface_finish(surface);
    cairo_surface_destroy(surface);
    surface = NULL;

    pp::MouseCursor::SetCursor(this,
                               PP_MOUSECURSOR_TYPE_CUSTOM,
                               *image_data,
                               pp::Point(size.width() / 2,
                                         size.height() / 2));

    delete image_data;
    image_data = NULL;
  }

  virtual bool Init(uint32_t argc, const char *argn[], const char *argv[]) {
    printf("init called\n");
    SetCrosshairCursor();
    RequestInputEvents(PP_INPUTEVENT_CLASS_MOUSE |
                       PP_INPUTEVENT_CLASS_KEYBOARD |
                       PP_INPUTEVENT_CLASS_WHEEL);
    printf("calling nacl_io_init()\n");
    nacl_io_init_ppapi(pp::Instance::pp_instance(),
                       pp::Module::Get()->get_browser_interface());
    renderer_ = new PDFRenderer(this);
    g_inst_ = this;
    render_thread_.Start();
    return true;
  }

  //void RunOnRenderThread(c++11 function)

  virtual void DidChangeView(const pp::View& view) {
    printf("View did change\n");
    if (size_ == view.GetRect().size()) {
      printf("no size change\n");
      return;
    }
    size_ = view.GetRect().size();
    scale_ = view.GetDeviceScale();
    pp::Size scaled_size(size_.width() * scale_,
                         size_.height() * scale_);
    graphics_ =
        pp::Graphics2D(this, scaled_size, false);
    graphics_.SetScale(1.0 / scale_);
    BindGraphics(graphics_);
    PDFRenderer* renderer = renderer_;
    float scale = scale_;
    pp::Size size = size_;
    RunOnRenderThread([renderer, scale, size] () {
        renderer->SetSize(size, scale);
      });
    // render_thread_.message_loop().PostWork(
    //     callback_factory_.NewCallback(&PDFSketchInstance::SetSize,
    //                                   size_));
  }

  void PostStr(const char* str) {
    PostMessage(pp::Var(str));
  }

  void ExecOnRenderThread(std::function<void ()> func) {
    render_thread_.message_loop().PostWork(
        callback_factory_.NewCallback(&PDFSketchInstance::Exec, func));
  }

  void Exec(int32_t resize, std::function<void ()> func) {
    func();
  }

  void Paint(int32_t result, const pp::ImageData& data) {
    if (data.size() != size_) {
      printf("Paint: bad size: skipping\n");
      return;
    }
    graphics_.PaintImageData(data, pp::Point());
    PostMessage(pp::Var("starting flush"));
    // std::pair<pp::Instance*, const char*>* mypair =
    //     new std::pair<pp::Instance*, const char*>(this, "flush done");
    int32_t rc = graphics_.Flush(pp::CompletionCallback(MyCompletionCallback, NULL));
    if (rc != PP_OK) {
      PostMessage(pp::Var("flush reply:"));
      PostMessage(pp::Var(rc));
      // delete mypair;
    }
  }

  void SendPDFOut(int32_t result, const vector<char>& out) {
    pp::VarArrayBuffer out_var(out.size());
    char* out_var_buf = (char*)out_var.Map();
    memcpy(out_var_buf, &out[0], out.size());
    out_var.Unmap();
    PostMessage(out_var);
  }
  void PostMessageOut(int32_t result, const std::string& msg) {
    PostMessage(msg);
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

  void SetZoom(int32_t result, const double& zoom) {
    renderer_->SetZoom(zoom);
  }

  void SelectTool(int32_t result, const std::string& tool) {
    renderer_->SelectTool(tool);
  }

  void SaveFile(int32_t result) {
    renderer_->SaveFile();
  }

  void ExportPDF(int32_t result) {
    renderer_->ExportPDF();
  }

  void Undo(int32_t result) {
    renderer_->Undo();
  }

  void Redo(int32_t result) {
    renderer_->Redo();
  }

  // void SetSize(int32_t result, const pp::Size& size) {
  //   renderer_->SetSize(size);
  // }

  virtual bool HandleInputEvent(const pp::InputEvent& event) {
    // if (event.GetType() == PP_INPUTEVENT_TYPE_MOUSEMOVE) {
    //   pp::MouseInputEvent mouse_evt(event);
    //   pp::Point mouse_pos = mouse_evt.GetPosition();
    //   printf("Mouse: %d %d\n", mouse_pos.x(), mouse_pos.y());
    // }
    // if (event.GetType() == PP_INPUTEVENT_TYPE_CHAR) {
    //   pp::KeyboardInputEvent key_evt(event);
    //   PostMessage(pp::Var("Input key:"));
    //   PostMessage(pp::Var(key_evt.GetCharacterText().AsString().c_str()));
    // }
    render_thread_.message_loop().PostWork(
        callback_factory_.NewCallback(&PDFSketchInstance::HandleInputEventThread,
                                      event));
    return true;
  }
  virtual void HandleInputEventThread(int32_t result, const pp::InputEvent& event) {
    renderer_->HandleInputEvent(event);
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
    const char kZoomPrefix[] = "zoomTo:";
    printf("got pepper message %s\n", message.c_str());
    if (!strncmp(message.c_str(), kZoomPrefix, sizeof(kZoomPrefix) - 1)) {
      render_thread_.message_loop().PostWork(
          callback_factory_.NewCallback(&PDFSketchInstance::SetZoom,
                                        atof(message.c_str() +
                                             sizeof(kZoomPrefix) - 1)));
      return;
    }
    const char kToolboxPrefix[] = "selectTool:";
    if (!strncmp(message.c_str(), kToolboxPrefix, sizeof(kToolboxPrefix) - 1)) {
      render_thread_.message_loop().PostWork(
          callback_factory_.NewCallback(
              &PDFSketchInstance::SelectTool,
              message.substr(sizeof(kToolboxPrefix) - 1)));
      return;
    }
    if (message == "save") {
      render_thread_.message_loop().PostWork(
          callback_factory_.NewCallback(&PDFSketchInstance::SaveFile));
    }
    if (message == "exportPDF") {
      render_thread_.message_loop().PostWork(
          callback_factory_.NewCallback(&PDFSketchInstance::ExportPDF));
      return;
    }
    if (message == "undo") {
      render_thread_.message_loop().PostWork(
          callback_factory_.NewCallback(&PDFSketchInstance::Undo));
      return;
    }
    if (message == "redo") {
      render_thread_.message_loop().PostWork(
          callback_factory_.NewCallback(&PDFSketchInstance::Redo));
      return;
    }

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
  float scale_;
  pp::SimpleThread render_thread_;
  PDFRenderer* renderer_;
 public:
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
                                  pp::Size(size_.width() * scale_,
                                           size_.height() * scale_),
                                  true);
  surface_ =
      cairo_image_surface_create_for_data((unsigned char*)image_data_->data(),
                                          CAIRO_FORMAT_ARGB32,
                                          size_.width() * scale_,
                                          size_.height() * scale_,
                                          image_data_->stride());
  cr_ = cairo_create(surface_);
  return cr_;
}

bool PDFRenderer::FlushCairo(std::function<void(int32_t)> complete_callback) {
  cairo_destroy(cr_);
  cr_ = NULL;
  cairo_surface_finish(surface_);
  cairo_surface_destroy(surface_);
  surface_ = NULL;

  // pp::Module::Get()->core()->CallOnMainThread(
  //     0,
  //     instance_->callback_factory_.NewCallback(
  //         &PDFSketchInstance::Paint, *image_data_));
  instance_->graphics_.PaintImageData(*image_data_, pp::Point());
  std::function<void(int32_t)>* callback_pointer =
      new std::function<void(int32_t)>(complete_callback);
  int32_t rc = instance_->graphics_.Flush(pp::CompletionCallback(FlushCompletionCallback, callback_pointer));
  delete image_data_;
  image_data_ = NULL;
  if (rc != PP_OK_COMPLETIONPENDING) {
    delete callback_pointer;
    char buf[100];
    snprintf(buf, sizeof(buf), "flush done rc: %d", rc);
    Dbg(buf);
    return false;
  }
  return true;
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
void PDFRenderer::SaveFile() {
  vector<char> out;
  pdfsketch::FileIO::Save(document_view_, &out);
  pp::Module::Get()->core()->CallOnMainThread(
      0,
      instance_->callback_factory_.NewCallback(
          &PDFSketchInstance::SendPDFOut, out));
}
void PDFRenderer::ExportPDF() {
  // Get flattened .pdf file
  vector<char> pdf;
  // Get native .pdfsketch file
  document_view_.ExportPDF(&pdf);
  vector<char> out;
  pdfsketch::FileIO::Save(document_view_, &out);
  // Insert .pdfsketch file into flattened .pdf

  PoDoFo::PdfMemDocument doc;
  doc.Load(&pdf[0], pdf.size());
  PoDoFo::PdfFileSpec filespec(
      "source.pdfsketch",
      reinterpret_cast<const unsigned char*>(&out[0]),
      out.size(), &doc);
  printf("export line %d\n", __LINE__);
  out.clear();
  printf("export line %d\n", __LINE__);
  doc.AttachFile(filespec);
  printf("export line %d\n", __LINE__);

  printf("export line %d\n", __LINE__);
  PoDoFo::PdfRefCountedBuffer out_buf;
  printf("export line %d\n", __LINE__);
  PoDoFo::PdfOutputDevice out_dev(&out_buf);
  printf("export line %d\n", __LINE__);
  doc.Write(&out_dev);
  printf("export line %d\n", __LINE__);

  pdf.clear();
  printf("export line %d\n", __LINE__);
  pdf.insert(pdf.begin(),
             out_buf.GetBuffer(),
             out_buf.GetBuffer() + out_buf.GetSize());
  printf("export line %d\n", __LINE__);

  pp::Module::Get()->core()->CallOnMainThread(
      0,
      instance_->callback_factory_.NewCallback(
          &PDFSketchInstance::SendPDFOut, pdf));
  printf("export line %d\n", __LINE__);
}
void PDFRenderer::PostMessage(const std::string& message) {
  pp::Module::Get()->core()->CallOnMainThread(
      0,
      instance_->callback_factory_.NewCallback(
          &PDFSketchInstance::PostMessageOut, message));
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

namespace pdfsketch {
void Dbg(const char* str) {
  if (!g_inst_)
    return;
  g_inst_->PostStr(str);
}
}  // namespace pdfsketch

void RunOnMainThread(std::function<void ()> func) {
  pp::Module::Get()->core()->CallOnMainThread(
      0,
      g_inst_->callback_factory_.NewCallback(
          &PDFSketchInstance::Exec, func));
}

void RunOnRenderThread(std::function<void ()> func) {
  g_inst_->ExecOnRenderThread(func);
}
