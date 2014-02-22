// Copyright...

#ifndef PDFSKETCH_FILE_IO_H__
#define PDFSKETCH_FILE_IO_H__

#include <vector>

#include "document_view.h"

namespace pdfsketch {

class FileIO {
 public:
  static void Open(const char* buf, size_t len, DocumentView* doc);
  static void Save(const DocumentView& doc, std::vector<char>* out);
};

}  // namespace pdfsketch

#endif  // PDFSKETCH_FILE_IO_H__
