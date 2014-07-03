#include "XMLDumpParser.hh"

#include <cassert>
#include <cstdlib>
#include <cstring>

extern "C" {
    void handleStartElement_redir(void *user_data, const XML_Char *name, const XML_Char **attrs) {
        XMLDumpParser *p = static_cast<XMLDumpParser*>(user_data);
        p->handleStartElement(name);
    }

    void handleEndElement_redir(void *user_data, const XML_Char *name) {
        XMLDumpParser *p = static_cast<XMLDumpParser*>(user_data);
        p->handleEndElement(name);
    }

    void handleCharacterData_redir(void *user_data, const XML_Char *text, int len) {
        XMLDumpParser *p = static_cast<XMLDumpParser*>(user_data);
        p->handleText(text, len);
    }
}

XMLDumpParser::XMLDumpParser(const char *path, XMLDumpParserMode _mode) {
    input = open_compressed_dump(path);
    assert(input);

    mode = _mode;

    input_buffer = new char[buffer_size];

    parser = XML_ParserCreate("utf-8");
    XML_SetUserData(parser, this);
    XML_SetElementHandler(parser, handleStartElement_redir, handleEndElement_redir);
    XML_SetCharacterDataHandler(parser, handleCharacterData_redir);

    state = Root;
}

XMLDumpParser::~XMLDumpParser() {
    XML_ParserFree(parser);
    delete[] input_buffer;
}

bool XMLDumpParser::drive() {
    ssize_t read;
    bool done;

    read = input->read(input_buffer, buffer_size);
    if (read < 0) {
        return false;
    }

    done = read < buffer_size;
    XML_Parse(parser, input_buffer, read, done);
    assert(!done || state == Root);
    return !done;
}

bool XMLDumpParser::is_queue_empty() {
    switch (mode) {
        case PerPage:
            return pages.empty();
        case PerRevision:
            return revisions.empty();
        default:
            assert(false);
    }
}

void XMLDumpParser::fill_queue() {
    if (!input_finished) {
        while (is_queue_empty()) {
            if (!drive()) {
                input_finished = true;
                break;
            }
        }
    }
}

MediaWikiRevision_s XMLDumpParser::read_revision() {
    assert(mode == PerRevision);

    // Read data until we have a whole revision
    fill_queue();
    if (revisions.empty()) {
        return nullptr;
    }

    MediaWikiRevision_s result = revisions.front();
    revisions.pop();
    return result;
}

MediaWikiPageHistory XMLDumpParser::read_page() {
    assert(mode == PerPage);

    // Read data until we have an entire page
    fill_queue();
    if (pages.empty()) {
        return {nullptr, {}};
    }

    MediaWikiPageHistory result;
    result.page = pages.front();
    while (!revisions.empty() && revisions.front()->page == result.page) {
        result.revisions.push_back(revisions.front());
        revisions.pop();
    }
    pages.pop();

    return result;
}

void XMLDumpParser::handleStartElement(const XML_Char *name) {
    if (!strcmp(name, "page")) {
        assert(state == Root);
        current_page.reset(new MediaWikiPage());
        state = Page;
        return;
    }

    if (!strcmp(name, "title")) {
        assert(state == Page);
        state = Title;
        return;
    }

    if (!strcmp(name, "ns")) {
        assert(state == Page);
        state = Namespace;
        accumulator = "";
        return;
    }

    if (!strcmp(name, "id")) {
        assert(state == Page || state == Revision || state == Contributor);
        if (state == Page) {
            state = PageID;
        }
        if (state == Revision) {
            state = RevisionID;
        }
        if (state == Contributor) {
            state = AuthorID;
        }
        accumulator = "";
        return;
    }

    if (!strcmp(name, "revision")) {
        assert(state == Page);
        current_revision.reset(new MediaWikiRevision());
        current_revision->page = current_page;
        state = Revision;
        return;
    }

    if (!strcmp(name, "timestamp")) {
        assert(state == Revision);
        state = Timestamp;
    }

    if (!strcmp(name, "contributor")) {
        assert(state == Revision);
        state = Contributor;
    }

    if (!strcmp(name, "username")) {
        assert(state == Contributor);
        state = AuthorName;
    }

    if (!strcmp(name, "ip")) {
        assert(state == Contributor);
        state = AuthorIP;
    }

    if (!strcmp(name, "comment")) {
        assert(state == Revision);
        state = Comment;
    }

    if (!strcmp(name, "text")) {
        assert(state == Revision);
        state = Text;
    }
}

void XMLDumpParser::handleEndElement(const XML_Char *name) {
    if (!strcmp(name, "page")) {
        assert(state == Page);
        state = Root;

        if (mode == PerPage) {
            pages.push(current_page);
        }
        return;
    }

    if (!strcmp(name, "revision")) {
        assert(state == Revision);
        state = Page;

        revisions.push(current_revision);
        return;
    }

    if (!strcmp(name, "title")) {
        assert(state == Title);
        state = Page;
        return;
    }

    if (!strcmp(name, "ns")) {
        assert(state == Namespace);
        state = Page;
        current_page->ns = atoi(accumulator.c_str());
        return;
    }

    if (!strcmp(name, "id")) {
        assert(state == PageID || state == RevisionID || state == AuthorID);
        if (state == PageID) {
            state = Page;
            current_page->id = atoi(accumulator.c_str());
        }
        if (state == RevisionID) {
            state = Revision;
            current_revision->id = atoi(accumulator.c_str());
        }
        if (state == AuthorID) {
            state = Contributor;
            current_revision->author_id = atoi(accumulator.c_str());
        }
        return;
    }

    if (!strcmp(name, "timestamp")) {
        assert(state == Timestamp);
        state = Revision;
        return;
    }

    if (!strcmp(name, "username")) {
        assert(state == AuthorName);
        state = Contributor;
        return;
    }

    if (!strcmp(name, "ip")) {
        assert(state == AuthorIP);
        state = Contributor;
        return;
    }

    if (!strcmp(name, "contributor")) {
        assert(state == Contributor);
        state = Revision;
        return;
    }

    if (!strcmp(name, "comment")) {
        assert(state == Comment);
        state = Revision;
        return;
    }

    if (!strcmp(name, "text")) {
        assert(state == Text);
        state = Revision;
        return;
    }
}

void XMLDumpParser::handleText(const XML_Char *text, int len) {
    switch (state) {
        case Namespace:
        case PageID:
        case RevisionID:
        case AuthorID:
            accumulator.append(text, len);
            break;
        case Title:
            current_page->title.append(text, len);
            break;
        case Timestamp:
            current_revision->timestamp.append(text, len);
            break;
        case AuthorName:
            current_revision->author_name.append(text, len);
            break;
        case AuthorIP:
            current_revision->author_ip.append(text, len);
        case Comment:
            current_revision->comment.append(text, len);
            break;
        case Text:
            current_revision->text.append(text, len);
            break;
        default:
            // Ignore text outside of tags we're interested in
            break;
    }
}
