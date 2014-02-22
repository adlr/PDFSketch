// Copyright...

#include "file_io.h"

#include "document.pb.h"
#include "document_view.h"
#include "graphic_factory.h"

using std::make_shared;
using std::string;
using std::vector;

// File format. Keep this synced with the code.
//
// All integers are big endian.
//
// char[4] magic:     'skch'
// uint32  version:   1
// uint32  num_pdfs:  Number of PDFs embedded. Must be 1 for now.
// uint64  pdf_len:   Number of bytes in PDF
// char[pdf_len] pdf: PDF Data
// uint64  overlay_len: Length of overlay protobuf
// char[overlay_len] overlays: Overlays protobuf data

namespace pdfsketch {

namespace {
const char kMagic[] = {'s', 'k', 'c', 'h'};
uint32_t DecodeUInt32(const unsigned char* buf) {
  return
      (static_cast<uint32_t>(buf[0]) << (8 * 3)) |
      (static_cast<uint32_t>(buf[1]) << (8 * 2)) |
      (static_cast<uint32_t>(buf[2]) << (8 * 1)) |
      (static_cast<uint32_t>(buf[3]) << (8 * 0));
}
uint64_t DecodeUInt64(const unsigned char* buf) {
  return
      (static_cast<uint64_t>(buf[0]) << (8 * 7)) |
      (static_cast<uint64_t>(buf[1]) << (8 * 6)) |
      (static_cast<uint64_t>(buf[2]) << (8 * 5)) |
      (static_cast<uint64_t>(buf[3]) << (8 * 4)) |
      (static_cast<uint64_t>(buf[4]) << (8 * 3)) |
      (static_cast<uint64_t>(buf[5]) << (8 * 2)) |
      (static_cast<uint64_t>(buf[6]) << (8 * 1)) |
      (static_cast<uint64_t>(buf[7]) << (8 * 0));
}
void EncodeUInt32(uint32_t num, char* out) {
  out[0] = (num >> (8 * 3)) & 0xff;
  out[1] = (num >> (8 * 2)) & 0xff;
  out[2] = (num >> (8 * 1)) & 0xff;
  out[3] = (num >> (8 * 0)) & 0xff;
}
void EncodeUInt64(uint64_t num, char* out) {
  out[0] = (num >> (8 * 7)) & 0xff;
  out[1] = (num >> (8 * 6)) & 0xff;
  out[2] = (num >> (8 * 5)) & 0xff;
  out[3] = (num >> (8 * 4)) & 0xff;
  out[4] = (num >> (8 * 3)) & 0xff;
  out[5] = (num >> (8 * 2)) & 0xff;
  out[6] = (num >> (8 * 1)) & 0xff;
  out[7] = (num >> (8 * 0)) & 0xff;
}
void PushUInt32(uint32_t num, vector<char>* out) {
  char buf[4];
  EncodeUInt32(num, buf);
  out->insert(out->end(), buf, buf + sizeof(buf));
}
void PushUInt64(uint32_t num, vector<char>* out) {
  char buf[8];
  EncodeUInt64(num, buf);
  out->insert(out->end(), buf, buf + sizeof(buf));
}
// Returns buf advanced past the parsed item
const char* ParseUInt32(const char* buf, uint32_t* out) {
  *out = DecodeUInt32(reinterpret_cast<const unsigned char*>(buf));
  return buf + 4;
}
const char* ParseUInt64(const char* buf, uint64_t* out) {
  *out = DecodeUInt64(reinterpret_cast<const unsigned char*>(buf));
  return buf + 8;
}
}  // namespace {}

void FileIO::Open(const char* buf, size_t len, DocumentView* doc) {
  size_t start = (size_t)buf;
  if (strncmp(buf, kMagic, sizeof(kMagic))) {
    printf("%s: Missing magic\n", __func__);
    return;
  }
  buf += sizeof(kMagic);
  printf("now at (after magic) %zu\n", ((size_t)buf) - start);
  uint32_t version = 0;
  buf = ParseUInt32(buf, &version);
  printf("now at (after version parse) %zu\n", ((size_t)buf) - start);
  if (version != 1) {
    printf("%s: Invalid version\n", __func__);
    return;
  }
  uint32_t num_pdfs = 0;
  printf("parsing at offset %zu: %d %d %d %d\n", ((size_t)buf) - start,
         buf[0], buf[1], buf[2], buf[3]);
  buf = ParseUInt32(buf, &num_pdfs);
  if (num_pdfs != 1) {
    printf("%s: Invalid PDF count: %d\n", __func__, num_pdfs);
    return;
  }
  uint64_t pdfdata_len = 0;
  buf = ParseUInt64(buf, &pdfdata_len);
  if (pdfdata_len > len) {
    // sanity check
    printf("%s: PDF too big\n", __func__);
    return;
  }
  const char* pdf_doc = buf;
  buf += pdfdata_len;
  uint64_t overlay_len = 0;
  buf = ParseUInt64(buf, &overlay_len);
  if (overlay_len > len) {
    // sanity check
    printf("%s: overlays too big\n", __func__);
    return;
  }
  pdfsketchproto::Document msg;
  if (!msg.ParseFromArray(buf, overlay_len)) {
    printf("protobuf decode failed\n");
    return;
  }
  doc->LoadFromPDF(pdf_doc, pdfdata_len);
  for (size_t i = 0; i < msg.graphic_size(); i++) {
    const pdfsketchproto::Graphic& gr = msg.graphic(i);
    doc->AddGraphic(GraphicFactory::NewGraphic(gr));
  }
}

void FileIO::Save(const DocumentView& doc, std::vector<char>* out) {
  out->insert(out->end(), kMagic, kMagic + sizeof(kMagic));
  PushUInt32(1, out);  // version
  PushUInt32(1, out);  // number of pdfs
  const char* pdfdata = NULL;
  size_t pdfdata_len = 0;
  doc.GetPDFData(&pdfdata, &pdfdata_len);
  PushUInt64(pdfdata_len, out);
  out->insert(out->end(), pdfdata, pdfdata + pdfdata_len);  // pdf data
  pdfsketchproto::Document msg;
  printf("%s:%d\n", __FILE__, __LINE__);
  doc.Serialize(&msg);
  printf("%s:%d\n", __FILE__, __LINE__);
  string msg_buf;
  printf("%s:%d\n", __FILE__, __LINE__);
  if (!msg.SerializeToString(&msg_buf)) {
    printf("error serializing to string\n");
  }
  printf("%s:%d\n", __FILE__, __LINE__);
  PushUInt64(msg_buf.size(), out);
  printf("%s:%d\n", __FILE__, __LINE__);
  out->insert(out->end(), msg_buf.begin(), msg_buf.end());
  printf("%s:%d\n", __FILE__, __LINE__);
}

}  // namespace pdfsketch
