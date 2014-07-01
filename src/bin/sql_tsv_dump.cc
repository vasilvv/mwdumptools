#include "SQLDumpParser.hh"

int main(int argc, char *argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: mw-sql-tsv-dump dump.sql.gz" << std::endl;
        return 1;
    }

    SQLDumpParser dump(argv[1]);
	while (SQLRow_u row = dump.get()) {
        for (uint64_t i = 0; i < row->size(); i++) {
            std::cout << (*row)[i];
            if (i < row->size() - 1) {
                std::cout << '\t';
            } else {
                std::cout << std::endl;
            }
        }
    }

    return 0;
}
