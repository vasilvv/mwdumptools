#include "XMLDumpParser.hh"

#include <iostream>

int main(int argc, char *argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: mw-xml-dump-info dump.xml" << std::endl;
    }

    XMLDumpParser parser(argv[1], PerPage);
    uint64_t total_pages = 0;
    uint64_t total_revisions = 0;
    for (MediaWikiPageHistory ph = parser.read_page(); ph.page; ph = parser.read_page()) {
        std::cout << "== " << ph.page->get_title() << " ==" << std::endl;
        std::cout << "Page ID: " << ph.page->get_id() << std::endl;
        std::cout << "Namespace ID: " << ph.page->get_namespace() << std::endl;
        std::cout << "Revision count: " << ph.revisions.size() << std::endl;
        std::cout << std::endl;

        for (MediaWikiRevision_s rev : ph.revisions) {
            std::cout << "=== Revision " << rev->get_id() << " ===" << std::endl;
            std::cout << "Timestamp: " << rev->get_timestamp() << std::endl;
            if (rev->get_author_id() >= 0) {
                std::cout << "Author name: " << rev->get_author_name() << std::endl;
                std::cout << "Author ID: " << rev->get_author_id() << std::endl;
            } else {
                std::cout << "Author IP: " << rev->get_author_ip() << std::endl;
            }
            std::cout << "Comment: " << rev->get_comment() << std::endl;
            std::cout << "Size: " << rev->get_text_size() << " bytes" << std::endl;
            std::cout << std::endl;
        }

        total_pages += 1;
        total_revisions += ph.revisions.size();
        std::cout << std::endl;
    }

    std::cout << std::endl;
    std::cout << "Handled " << total_pages << " pages, " << total_revisions << " revisions" << std::endl;

    return 0;
}
