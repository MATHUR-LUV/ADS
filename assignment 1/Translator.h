#ifndef TRANSLATOR_H
#define TRANSLATOR_H

#include <string>
#include "QueryModel.h"

class RATranslator {
public:

    std::string toRA(const QueryParts& parts, int indent = 0);
};

#endif