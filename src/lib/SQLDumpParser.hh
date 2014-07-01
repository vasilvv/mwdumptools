#ifndef __SQL_DUMP_PARSER_HH
#define __SQL_DUMP_PARSER_HH

#include <cassert>
#include <cstdint>
#include <cstring>
#include <exception>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include <boost/iostreams/filtering_stream.hpp>
#include <boost/iostreams/filter/gzip.hpp>

typedef enum {
    String,
    Float,
    Integer,
    Null
} SQLDataType;

class SQLData {
  private:
    SQLDataType type;

    union {
        int64_t integer;
        double fraction;
        std::string *str;
    } value;

  public:
    inline std::string as_string() const {
        assert(type == String);
        return *value.str;
    }
    inline uint64_t as_int() const {
        assert(type == Integer);
        return value.integer;
    }
    inline double as_float() const {
        assert(type == Float);
        return value.fraction;
    }

    inline bool is_string() const { return type == String; }
    inline bool is_int() const { return type == Integer; }
    inline bool is_float() const { return type == Float; }
    inline bool is_null() const { return type == Null; }

    SQLData(std::string &&str);
    SQLData(const char *str);
    SQLData(int64_t integer);
    SQLData(double fraction);
    SQLData(const SQLData &data);
    SQLData();

    ~SQLData();
};

typedef std::vector<SQLData> SQLRow;
typedef std::unique_ptr<SQLRow> SQLRow_u;

namespace std {
    inline ostream &operator<< (ostream &os, const ::SQLData &val) {
        if( val.is_string() ) {
            os << val.as_string();
        }
        if( val.is_float() ) {
            os << val.as_float();
        }
        if( val.is_int() ) {
            os << val.as_int();
        }
        if( val.is_null() ) {
            os << "NULL";
        }

        return os;
    };
}

class SQLDumpParser {
    private:
        std::ifstream dump_file;
        boost::iostreams::filtering_istream input;
        bool mid_statement = false;

        bool read_until_values();
    public:
        SQLDumpParser(std::string && path);
        SQLRow_u get();
};

#endif /* __SQL_DUMP_PARSER_HH */
