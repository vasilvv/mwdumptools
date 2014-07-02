#ifndef __COMPRESSEDDUMPREADER_HH
#define __COMPRESSEDDUMPREADER_HH

#include <memory>

class CompressedDumpReader {
  public:
    virtual int get() = 0;

    CompressedDumpReader() {}
    CompressedDumpReader(CompressedDumpReader const&) = delete;
    virtual ~CompressedDumpReader() {}
};

typedef std::unique_ptr<CompressedDumpReader> CompressedDumpReader_u;

CompressedDumpReader_u open_compressed_dump(const char *path);

#endif /* __COMPRESSEDDUMPREADER_HH */
