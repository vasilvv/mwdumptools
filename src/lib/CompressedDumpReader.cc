#include "CompressedDumpReader.hh"

#include <cstring>
#include <fstream>

#include <zlib.h>

#define BUFFER_SIZE 8192

class TransparentDumpReader : public CompressedDumpReader {
  private:
    std::ifstream file;

  public:
    TransparentDumpReader(const char *path)
        : CompressedDumpReader(), file(path, std::ifstream::binary) {}
    virtual ~TransparentDumpReader() {};

    virtual int get() override { return file.get(); }
};

class GzipDumpReader : public CompressedDumpReader {
  private:
    gzFile file;

  public:
    GzipDumpReader(const char *path) : CompressedDumpReader() {
        file = gzopen(path, "rb");
    }

    virtual ~GzipDumpReader() {
        if (file) {
            gzclose(file);
            file = nullptr;
        }
    }

    virtual int get() override {
        static_assert(std::char_traits<char>::eof() == -1,
                      "EOF is meant to be -1");

        return gzgetc(file);
    }

    bool valid() { return file != nullptr; }
};

/**
 * Attempt to determine the type of the file based on the first
 * 10 characters of it.
 */
CompressedDumpReader_u open_compressed_dump(const char *path) {
    std::ifstream file{path, std::ifstream::binary};
    char preamble[10];

    file.read(preamble, 10);
    if (!file) {
        return nullptr;
    }

    // Raw SQL dump
    if (!memcmp("-- ", preamble, 3)) {
        return CompressedDumpReader_u(new TransparentDumpReader(path));
    }

    // Raw XML dump
    if (!memcmp("<mediawiki", preamble, 10)) {
        return CompressedDumpReader_u(new TransparentDumpReader(path));
    }

    // Gzip
    if (!memcmp("\x1f\x8b\x08", preamble, 3)) {
        return CompressedDumpReader_u(new GzipDumpReader(path));
    }

    // Play safe and return error
    return nullptr;

}
