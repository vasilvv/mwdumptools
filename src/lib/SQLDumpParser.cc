#include "SQLDumpParser.hh"

#include <cctype>

/*************************** SQLData ***************************/
SQLData::SQLData(std::string && str) {
    type = String;
    value.str = new std::string(str);
}

SQLData::SQLData(const char *str) {
    type = String;
    value.str = new std::string(str);
}

SQLData::SQLData(int64_t integer) {
    type = Integer;
    value.integer = integer;
}

SQLData::SQLData(double fraction) {
    type = Float;
    value.fraction = fraction;
}

SQLData::SQLData() {
    type = Null;
}

SQLData::SQLData(const SQLData & data) {
    type = data.type;
    if (type == String) {
        value.str = new std::string(*data.value.str);
    } else {
        value = data.value;
    }
}

SQLData::~SQLData() {
    if (type == String) {
        delete value.str;
    }
}

/*************************** SQLDumpParser ***************************/
SQLDumpParser::SQLDumpParser(std::string && path) :
    dump_file(path, std::ifstream::in | std::ifstream::binary)
{
    input.push(boost::iostreams::gzip_decompressor());
    input.push(dump_file);
}

bool SQLDumpParser::read_until_values() {
    // Note: this substring search works solely by virtue of the fact
    // that all characters in string "VALUES " are distinct.
    // For generic case we'd need Knuth-Morris-Pratt, or something.

    const char *target = "VALUES ";
    int pos = 0;

    while (target[pos] != 0) {
        char cur = input.get();

        if (cur == std::char_traits<char>::eof()) {
            return false;
        }

        // Advance search if we found a partial match,
        // otherwise reset it to the beginning
        if (cur == target[pos]) {
            pos++;
        } else {
            pos = 0;
        }
    }

    return true;
}

SQLRow_u SQLDumpParser::get() {
    SQLRow_u row{new SQLRow()};
    const char eof = std::char_traits<char>::eof();

    // Check if we need to skip to the next "VALUES" clause
    if (!mid_statement) {
        if (!read_until_values()) {
            return nullptr;
        }

        mid_statement = true;
    }

    // Start tuple of values
    assert(input.get() == '(');
    for (;;) {
        char cur = input.get();

        // String
        if (cur == '\'') {
            std::string str;
            bool backslash = false;

            // Read string while handling backslashes
            for (;;) {
                cur = input.get();
                assert(cur != eof);

                if (backslash) {
                    str.push_back(cur);
                    backslash = false;
                } else {
                    if (cur == '\\') {
                        backslash = true;
                    } else if (cur == '\'') {
                        break;
                    } else {
                        str.push_back(cur);
                    }
                }
            }

            row->emplace_back(std::string(str.data(), str.size()));

            // In other handers, this naturally points at ',' or ')'
            cur = input.get();
        }

        // Integers/floats
        if (isdigit(cur) || cur == '-') {
            std::string str;
            bool fpoint = false;

            str.push_back(cur);
            for (;;) {
                cur = input.get();
                assert(cur != eof);

                if (cur == '.' && !fpoint) {
                    fpoint = true;
                    str.push_back(cur);
                } else if (isdigit(cur)) {
                    str.push_back(cur);
                } else {
                    break;
                }
            }

            if (fpoint) {
                row->emplace_back(std::stod(str));
            } else {
                row->emplace_back(static_cast<int64_t>(std::stoll(str)));
            }
        }

        // Nulls
        if (cur == 'N') {
            assert(input.get() == 'U');
            assert(input.get() == 'L');
            assert(input.get() == 'L');
            row->emplace_back();
            cur = input.get();
        }

        // Check for the end of tuple
        assert(cur == ',' || cur == ')');
        if (cur == ')') {
            break;
        }
    }

    // Handle comma or semicolon at the end
    char cur = input.get();
    assert(cur == ',' || cur == ';');
    if (cur == ';') {
        mid_statement = false;
    }

    return row;
}
