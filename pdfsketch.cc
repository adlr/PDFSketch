// Copyright... (derived from NaCl sample code BSD license)

#include "pdfsketch.h"

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <functional>
#include <memory>
#include <stdio.h>
#include <string.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <cairo-pdf.h>
#include <cairo.h>
#include <libtar.h>
#include <nacl_io/nacl_io.h>
#include <podofo/doc/PdfFileSpec.h>
#include <podofo/doc/PdfMemDocument.h>
#include <poppler/cpp/poppler-document.h>
#include <poppler/cpp/poppler-embedded-file.h>
#include <ppapi/c/ppb_image_data.h>
#include <ppapi/c/ppb_mouse_cursor.h>
#include <ppapi/cpp/completion_callback.h>
#include <ppapi/cpp/module.h>
#include <ppapi/cpp/mouse_cursor.h>
#include <ppapi/cpp/var.h>
#include <ppapi/cpp/var_array_buffer.h>
#include <ppapi/utility/completion_callback_factory.h>

#include "file_io.h"
#include "scroll_bar_view.h"

using std::string;
using std::unique_ptr;
using std::vector;

cairo_status_t HandleCairoStreamWrite(void* closure,
                                      const unsigned char *data,
                                      unsigned int length) {
  vector<char>* out = reinterpret_cast<vector<char>*>(closure);
  out->insert(out->end(), data, data + length);
  return CAIRO_STATUS_SUCCESS;
}

void FlushCompletionCallback(void* user_data, int32_t result) {
  std::function<void (int32_t)>* func_p =
      reinterpret_cast<std::function<void (int32_t)>*>(user_data);
  std::function<void (int32_t)> func = *func_p;
  func(result);
  delete func_p;
}

void MyCompletionCallback(void* user_data, int32_t result) {
}

void PDFSketchInstance::ToolSelected(pdfsketch::Toolbox::Tool tool) {
  std::string message =
      std::string("ToolSelected:") + pdfsketch::Toolbox::ToolAsString(tool);
  PostMessage(pp::Var(message));
}

void PDFSketchInstance::SetUndoEnabled(bool enabled) {
  PostMessage(pp::Var(
      enabled ? "undoEnabled:true" : "undoEnabled:false"));
  }
void PDFSketchInstance::SetRedoEnabled(bool enabled) {
  PostMessage(pp::Var(enabled ? "redoEnabled:true" : "redoEnabled:false"));
}

int PDFSketchInstance::SetupFS() {
  int ret = umount("/");
  if (ret) {
    printf("unmounting root fs failed\n");
    return 1;
  }
  ret = mount("", "/", "memfs", 0, NULL);
  if (ret) {
    printf("mounting root fs failed\n");
    return 1;
  }

  mkdir("/mnt", 0777);
  mkdir("/mnt/http", 0777);
  mkdir("/mnt/html5", 0777);

  const char* data_url = getenv("NACL_DATA_URL");
  if (!data_url)
    data_url = "./";

  ret = mount(data_url, "/mnt/http", "httpfs", 0,
              "");
  if (ret) {
    printf("mounting http filesystem failed\n");
    return 1;
  }

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
  return 0;
}

void PDFSketchInstance::SetPDF(const char* doc, size_t doc_len) {
  pdfsketch::FileIO::OpenPDF(doc, doc_len, &document_view_);
}

void PDFSketchInstance::SetSize(const pp::Size& size, float scale) {
  scroll_view_.SetScale(scale_);
  root_view_.Resize(pdfsketch::Size(size_.width() * scale_,
                                    size_.height() * scale_));
  if (!setup_) {
    setup_ = true;
    SetupFS();
  }
}

bool PDFSketchInstance::HandleInputEvent(const pp::InputEvent& event) {
  pp::InputEvent event_copy = event;
  RunOnRenderThread([this, event_copy] () {
      root_view_.HandlePepperInputEvent(event_copy, scale_);
    });
  return true;
}

PDFSketchInstance::PDFSketchInstance(PP_Instance instance)
    : pp::Instance(instance),
      callback_factory_(this),
      scale_(1.0),
      render_thread_(this),
      setup_(false),
      image_data_(NULL),
      surface_(NULL),
      cr_(NULL) {
}

bool PDFSketchInstance::ListAndRemove(const char* dir) {
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
    if (!S_ISDIR(stbuf.st_mode)) {
      unlink(name);
    } else {
      ListAndRemove(name);
      rmdir(name);
    }
  }
exit:
  closedir(dirp);
  return ret;
}

void PDFSketchInstance::SetCrosshairCursor() {
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

bool PDFSketchInstance::Init(uint32_t argc,
                             const char *argn[],
                             const char *argv[]) {
  SetCrosshairCursor();
  RequestInputEvents(PP_INPUTEVENT_CLASS_MOUSE |
                     PP_INPUTEVENT_CLASS_KEYBOARD |
                     PP_INPUTEVENT_CLASS_WHEEL);
  nacl_io_init_ppapi(pp::Instance::pp_instance(),
                     pp::Module::Get()->get_browser_interface());

  toolbox_.SetDelegate(this);
  undo_manager_.SetDelegate(this);
  root_view_.SetDelegate(this);
  root_view_.AddSubview(&scroll_view_);
  document_view_.SetToolbox(&toolbox_);
  document_view_.SetUndoManager(&undo_manager_);
  scroll_view_.SetDocumentView(&document_view_);
  scroll_view_.SetResizeParams(true, false, true, false);
  scroll_view_.SetFrame(root_view_.Frame());
  document_view_.Resize(pdfsketch::Size(800.0, 2000.0));

  render_thread_.Start();
  return true;
}

void PDFSketchInstance::DidChangeView(const pp::View& view) {
  if (size_ == view.GetRect().size()) {
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
  float scale = scale_;
  pp::Size size = size_;
  RunOnRenderThread([this, scale, size] () {
      SetSize(size, scale);
    });
}

void PDFSketchInstance::PostStr(const char* str) {
  PostMessage(pp::Var(str));
}

void PDFSketchInstance::Exec(int32_t result,
                             std::function<void ()> func) {
  func();
}

void PDFSketchInstance::Paint(int32_t result,
                              const pp::ImageData& data) {
  if (data.size() != size_) {
    printf("BUG Paint: bad size: skipping\n");
    return;
  }
  graphics_.PaintImageData(data, pp::Point());
  PostMessage(pp::Var("starting flush"));
  int32_t rc = graphics_.Flush(pp::CompletionCallback(MyCompletionCallback, NULL));
  if (rc != PP_OK) {
    PostMessage(pp::Var("flush reply:"));
    PostMessage(pp::Var(rc));
  }
}

void PDFSketchInstance::SendPDFOut(const vector<char>& out) {
  pp::VarArrayBuffer out_var(out.size());
  char* out_var_buf = (char*)out_var.Map();
  memcpy(out_var_buf, &out[0], out.size());
  out_var.Unmap();
  PostMessage(out_var);
}

void PDFSketchInstance::SetPDF(const pp::Var& doc) {
  pp::VarArrayBuffer vab(doc);
  void* buf = vab.Map();
  size_t length = vab.ByteLength();
  SetPDF(reinterpret_cast<char*>(buf), length);
  vab.Unmap();
}

void PDFSketchInstance::HandleMessage(const pp::Var& var_message) {
  if (var_message.is_array_buffer()) {
    pp::Var var_message_copy = var_message;
    RunOnRenderThread([this, var_message_copy] () {
        SetPDF(var_message_copy);
      });
    return;
  }

  // Ignore the message if it is not a string.
  if (!var_message.is_string())
    return;

  // Get the string message and compare it to "hello".
  std::string message = var_message.AsString();
  const char kZoomPrefix[] = "zoomTo:";
  if (!strncmp(message.c_str(),
               kZoomPrefix,
               sizeof(kZoomPrefix) - 1)) {
    float level = atof(message.c_str() + sizeof(kZoomPrefix) - 1);
    RunOnRenderThread([this, level] () {
        document_view_.SetZoom(level);
      });
    return;
  }
  const char kToolboxPrefix[] = "selectTool:";
  if (!strncmp(message.c_str(),
               kToolboxPrefix,
               sizeof(kToolboxPrefix) - 1)) {
    string tool = message.substr(sizeof(kToolboxPrefix) - 1);
    RunOnRenderThread([this, tool] () {
        toolbox_.SelectTool(tool);
      });
    return;
  }
  if (message == "save") {
    RunOnRenderThread([this] () {
        SaveFile();
      });
    return;
  }
  if (message == "exportPDF") {
    RunOnRenderThread([this] () {
        ExportPDF();
      });
    return;
  }
  if (message == "undo") {
    RunOnRenderThread([this] () {
        undo_manager_.PerformUndo();
      });
    return;
  }
  if (message == "redo") {
    RunOnRenderThread([this] () {
        undo_manager_.PerformRedo();
      });
    return;
  }
}

cairo_t* PDFSketchInstance::AllocateCairo() {
  if (size_.width() == 0 || size_.height() == 0)
    return NULL;
  if (cr_) {
    printf("Already have Cairo!\n");
    return NULL;
  }
  if (image_data_) {
    printf("Bug: image_data_ already allocated\n");
    return NULL;
  }
  image_data_ = new pp::ImageData(this,
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

bool PDFSketchInstance::FlushCairo(
    std::function<void(int32_t)> complete_callback) {
  cairo_destroy(cr_);
  cr_ = NULL;
  cairo_surface_finish(surface_);
  cairo_surface_destroy(surface_);
  surface_ = NULL;

  graphics_.PaintImageData(*image_data_, pp::Point());
  std::function<void(int32_t)> render_callback =
      [this, complete_callback] (int32_t result) {
    RunOnRenderThread([complete_callback, result] () {
        complete_callback(result);
      });
  };
  std::function<void(int32_t)>* callback_pointer =
      new std::function<void(int32_t)>(render_callback);
  int32_t rc = graphics_.Flush(pp::CompletionCallback(FlushCompletionCallback, callback_pointer));
  delete image_data_;
  image_data_ = NULL;
  if (rc != PP_OK_COMPLETIONPENDING) {
    printf("paint cairo bad return\n");
    delete callback_pointer;
    return false;
  }
  return true;
}

void PDFSketchInstance::SaveFile() {
  vector<char> out;
  pdfsketch::FileIO::Save(document_view_, &out);
  SendPDFOut(out);
}

void PDFSketchInstance::ExportPDF() {
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
  out.clear();
  doc.AttachFile(filespec);

  PoDoFo::PdfRefCountedBuffer out_buf;
  PoDoFo::PdfOutputDevice out_dev(&out_buf);
  doc.Write(&out_dev);

  pdf.clear();
  pdf.insert(pdf.begin(),
             out_buf.GetBuffer(),
             out_buf.GetBuffer() + out_buf.GetSize());

  SendPDFOut(pdf);
}

class HelloTutorialModule : public pp::Module {
 public:
  HelloTutorialModule() : pp::Module() {}
  virtual ~HelloTutorialModule() {}
  virtual pp::Instance* CreateInstance(PP_Instance instance) {
    return new PDFSketchInstance(instance);
  }
};

namespace pp {
Module* CreateModule() {
  return new HelloTutorialModule();
}
}  // namespace pp

void PDFSketchInstance::RunOnRenderThread(
    std::function<void ()> func) {
  render_thread_.message_loop().PostWork(
      callback_factory_.NewCallback(&PDFSketchInstance::Exec, func));
}
