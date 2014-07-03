#ifndef __XMLDUMPPARSER_HH
#define __XMLDUMPPARSER_HH

#include <cstdint>
#include <memory>
#include <queue>
#include <string>
#include <vector>

#include <expat.h>

#include "CompressedDumpReader.hh"

class XMLDumpParser;

class MediaWikiPage {
  friend class XMLDumpParser;

  private:
    std::string title;
    int32_t ns;
    int64_t id;

  public:
    inline std::string get_title() const { return title; }
    inline int32_t get_namespace() const { return ns; }
    inline int64_t get_id() const { return id; }
};

typedef std::shared_ptr<MediaWikiPage> MediaWikiPage_s;

class MediaWikiRevision {
  friend class XMLDumpParser;

  private:
    int64_t id;
    std::string timestamp;
    std::string author_name;
    std::string author_ip;
    int64_t author_id = -1;
    std::string comment;
    std::string text;
    MediaWikiPage_s page;

  public:
    inline int64_t get_id() const { return id; }
    inline std::string get_timestamp() const { return timestamp; }
    inline std::string get_author_name() const { return author_name; }
    inline std::string get_author_ip() const { return author_ip; }
    inline int64_t get_author_id() const { return author_id; }
    inline std::string get_comment() const { return comment; }
    inline std::string get_text() const { return text; }
    inline const char *get_text_ptr() const { return text.c_str(); }
    inline size_t get_text_size() const { return text.size(); }
    inline MediaWikiPage_s get_page() const { return page; }
};

typedef std::shared_ptr<MediaWikiRevision> MediaWikiRevision_s;

struct MediaWikiPageHistory {
    MediaWikiPage_s page;
    std::vector<MediaWikiRevision_s> revisions;
};

enum XMLDumpParserMode {
    PerRevision,
    PerPage
};

class XMLDumpParser {
  protected:
    enum XMLDumpParserState {
        Root,
        Page,
        Revision,
        Contributor,
        Title,
        Namespace,
        PageID,
        RevisionID,
        Timestamp,
        AuthorName,
        AuthorID,
        AuthorIP,
        Comment,
        Text,
    };

    XMLDumpParserState state;
    XMLDumpParserMode mode;
    XML_Parser parser;

    MediaWikiPage_s current_page;
    MediaWikiRevision_s current_revision;
    std::string accumulator;

    std::queue<MediaWikiRevision_s> revisions;
    std::queue<MediaWikiPage_s> pages;

    // Feed the data from the XML dump in 4 MiB chunks.  The maximum revision
    // size on Wikipedia is two megabytes;  this way, we attempt to make sure
    // that we do not have to assemble text of large articles piecewise from
    // chunks, since we are going to load them into memory in their entirety
    // anyways.
    const size_t buffer_size = 4 * 1024 * 1024;
    char *input_buffer;
    CompressedDumpReader_u input;
    bool input_finished = false;

    bool drive();
    bool is_queue_empty();
    void fill_queue();

  public:
    XMLDumpParser(const char *path, XMLDumpParserMode mode);
    ~XMLDumpParser();

    /**
     * Reads the next revision in the dump and returns it.
     * Works only when the mode is Revision.
     */
    MediaWikiRevision_s read_revision();

    /**
     * Reads a page with its entire history and returns it.
     * Works only when the mode is Page.
     */
    MediaWikiPageHistory read_page();

    // Internal APIs used from Expat
    void handleStartElement(const XML_Char *name);
    void handleEndElement(const XML_Char *name);
    void handleText(const XML_Char *text, int len);
};

#endif /* __XMLDUMPPARSER_HH */
