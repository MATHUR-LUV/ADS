#ifndef QUERY_MODEL_H
#define QUERY_MODEL_H

#include <string>
#include <vector>

struct JoinBlock {
    std::string table;
    std::string alias;
    std::string condition;
};

struct QueryParts {
    std::string projection = "";
    std::string table = "";
    std::string tableAlias = "";
    std::string selection = "";
    std::vector<JoinBlock> joins;
    std::string setOperator = "";
    QueryParts* subQuery = nullptr;
    QueryParts* leftQuery = nullptr;  
    QueryParts* rightQuery = nullptr; 

    QueryParts() : subQuery(nullptr) {}

    ~QueryParts() {
        delete leftQuery;
        delete rightQuery;
    }
};

#endif