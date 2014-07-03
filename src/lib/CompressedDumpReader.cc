#include "CompressedDumpReader.hh"

#include <cassert>
#include <cstring>
#include <fstream>

#include <archive.h>
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
    virtual ssize_t read(char *buffer, size_t len) override {
        file.read(buffer, len);

        if (!file) {
            if (file.eof()) {
                return file.gcount();
            } else {
                return -1;
            }
        }

        return len;
    }
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

    virtual ssize_t read(char *buffer, size_t len) override {
        return gzread(file, buffer, len);
    }

    bool valid() { return file != nullptr; }
};

class LibarchiveDumpReader : public CompressedDumpReader {
  private:
    struct archive *arch;
    bool err;

  public:
    LibarchiveDumpReader(const char *path) : CompressedDumpReader() {
        arch = archive_read_new();
        archive_read_support_filter_all(arch);
        archive_read_support_format_all(arch);

        err = archive_read_open_filename(arch, path, 65536) != ARCHIVE_OK;
        if (err) {
            return;
        }

        archive_entry *e;
        err = archive_read_next_header(arch, &e) != ARCHIVE_OK;
    }

    virtual ~LibarchiveDumpReader() {
        if (arch) {
            archive_read_close(arch);
            archive_read_free(arch);
            arch = nullptr;
        }
    }

    virtual int get() override {
        char result;
        if (read(&result, 1) == 1) {
            return result;
        } else {
            return std::char_traits<char>::eof();
        }
    }

    virtual ssize_t read(char *buffer, size_t len) override {
        return archive_read_data(arch, buffer, len);
    }

    bool valid() { return !err; }
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

    // 7-zip
    if (!memcmp("7z\xbc\xaf\x27\x1c", preamble, 6)) {
        return CompressedDumpReader_u(new LibarchiveDumpReader(path));
    }

    // Play safe and return error
    return nullptr;

}
