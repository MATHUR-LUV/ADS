#ifndef PARSER_H
#define PARSER_H

#include "QueryModel.h"
#include <regex>

class SQLParser {
public:
    QueryParts parse(std::string sql);

private:
    std::string clean(std::string str);
};

#endif