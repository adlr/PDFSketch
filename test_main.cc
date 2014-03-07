// Copyright...

#include <fcntl.h>
#include <stdio.h>
#include <vector>

#include "scroll_view.h"
#include "document_view.h"
#include "file_io.h"

using std::vector;

namespace pdfsketch {

void Test(const char* data, size_t length) {
  size_t width = 512;
  size_t height = 512;
  ScrollView scroll;
  DocumentView doc;
  FileIO::Open(data, length, &doc);
  scroll.SetDocumentView(&doc);
  scroll.SetResizeParams(true, false, true, false);
  scroll.SetFrame(Rect(0.0, 0.0, width + 0.0, height + 0.0));

  // Render a bunch of times
  for (size_t i = 0; i < 1000; i++) {
    unsigned char dummy[width * height * 4];
    cairo_surface_t* surface =
        cairo_image_surface_create_for_data(dummy,
                                            CAIRO_FORMAT_ARGB32,
                                            width, height,
                                            width * 4);
    cairo_t* cr = cairo_create(surface);

    scroll.DrawRect(cr, scroll.Frame());

    cairo_destroy(cr);
    cairo_surface_finish(surface);
    cairo_surface_destroy(surface);
  }
  printf("done rendering\n");
}

}  // namespace pdfsketch

int main(int argc, char** argv) {
  assert(argc >= 2);

  printf("opening %s\n", argv[1]);
  int fd = open(argv[1], O_RDONLY);
  assert(fd >= 0);
  vector<char> data;
  while (true) {
    char buf[1024 * 512];
    int rc = read(fd, buf, sizeof(buf));
    printf("read rc: %d\n", rc);
    assert(rc >= 0);
    data.insert(data.end(), buf, buf + rc);
    if (rc < (int)sizeof(buf))
      break;
  }
  close(fd);

  printf("%zu - %x %x %x %x\n", data.size(), data[0], data[1], data[2], data[3]);

  pdfsketch::Test(&data[0], data.size());
  return 0;
}
