#include "Parser.h"
#include <regex>
#include <algorithm>
#include <stack>

size_t findClosingParenthesis(const std::string &sql, size_t startPos)
{
    int depth = 0;
    for (size_t i = startPos; i < sql.length(); ++i)
    {
        if (sql[i] == '(')
            depth++;
        else if (sql[i] == ')')
        {
            depth--;
            if (depth == 0)
                return i;
        }
    }
    return std::string::npos;
}
std::string handleSetShorthand(std::string condition)
{
    std::regex set_re(R"(\b(\w+)\s+IN\s*\(\s*('[^']+'(?:\s*,\s*'[^']+')*)\s*\))", std::regex::icase);
    return std::regex_replace(condition, set_re, "$1 ∈ {$2}");
}
std::string handleShorthand(std::string condition)
{
    condition = handleSetShorthand(condition);
    std::regex between_re(R"((\w+)\s+BETWEEN\s+(\d+)\s+AND\s+(\d+))", std::regex::icase);
    std::smatch match;

    if (std::regex_search(condition, match, between_re))
    {
        std::string col = match[1].str();
        std::string val1 = match[2].str();
        std::string val2 = match[3].str();
        return val1 + " <= " + col + " AND " + col + " <= " + val2;
    }
    return condition;
}


std::string trimAndUnwrap(std::string s)
{
    if (s.empty())
        return s;

    s.erase(0, s.find_first_not_of(" \t\n\r"));
    s.erase(s.find_last_not_of(" \t\n\r") + 1);

    if (s.size() > 2 && s.front() == '(' && s.back() == ')')
    {
        int depth = 0;
        bool validUnwrap = true;
        for (size_t i = 0; i < s.size(); ++i)
        {
            if (s[i] == '(')
                depth++;
            else if (s[i] == ')')
                depth--;

            if (depth == 0 && i < s.size() - 1)
            {
                validUnwrap = false;
                break;
            }
        }
        if (validUnwrap)
            return trimAndUnwrap(s.substr(1, s.size() - 2));
    }
    return s;
}


size_t findTopLevelOp(const std::string &sql, std::string &foundOp)
{
    int depth = 0;
    size_t lastPos = std::string::npos;
    std::vector<std::pair<std::string, std::string>> ops = {
        {"UNION", "∪"}, {"INTERSECT", "∩"}, {"EXCEPT", "−"}};

    for (size_t i = 0; i < sql.length(); ++i)
    {
        if (sql[i] == '(')
        {
            depth++;
        }
        else if (sql[i] == ')')
        {
            depth--;
        }
        else if (depth == 0)
        {
            for (const auto &op : ops)
            {
               
                if (i + op.first.length() <= sql.length())
                {
                    std::string sub = sql.substr(i, op.first.length());

                    
                    bool match = true;
                    for (size_t j = 0; j < op.first.length(); ++j)
                    {
                        if (toupper(sub[j]) != toupper(op.first[j]))
                        {
                            match = false;
                            break;
                        }
                    }

                    if (match)
                    {
                    
                        bool startBoundary = (i == 0 || isspace(sql[i - 1]));
                        bool endBoundary = (i + op.first.length() == sql.length() || isspace(sql[i + op.first.length()]));

                        if (startBoundary && endBoundary)
                        {
                            foundOp = op.second;
                            lastPos = i;
                        }
                    }
                }
            }
        }
    }

    return lastPos;
}
QueryParts SQLParser::parse(std::string sql)
{
    sql = trimAndUnwrap(sql);
    QueryParts parts;
    std::string raOp;
    std::smatch match;


    size_t opPos = findTopLevelOp(sql, raOp);
    if (opPos != std::string::npos)
    {
        std::string leftSide = sql.substr(0, opPos);
        std::regex op_re(R"((UNION|INTERSECT|EXCEPT))", std::regex::icase);

        std::string rightSide;
        if (std::regex_search(sql.cbegin() + opPos, sql.cend(), match, op_re))
        {
            rightSide = sql.substr(opPos + match.length(0));
        }

        parts.setOperator = raOp;
      
        parts.leftQuery = new QueryParts(parse(leftSide));
        parts.rightQuery = new QueryParts(parse(rightSide));
        return parts;
    }

    std::regex select_re(R"(SELECT\s+(.*?)\s+FROM)", std::regex::icase);
    if (std::regex_search(sql, match, select_re))
    {
        parts.projection = match[1].str();
    }

    std::regex from_re(R"(FROM\s+([a-zA-Z0-9_]+)(?:\s+([a-zA-Z0-9_]+))?)", std::regex::icase);
    if (std::regex_search(sql, match, from_re))
    {
        parts.table = match[1].str();
        if (match[2].matched)
        {
            std::string alias = match[2].str();
            std::string upper = alias;
            std::transform(upper.begin(), upper.end(), upper.begin(), ::toupper);
            if (upper != "JOIN" && upper != "WHERE")
            {
                parts.tableAlias = alias;
            }
        }
    }

    std::regex join_re(R"(JOIN\s+([a-zA-Z0-9_]+)(?:\s+([a-zA-Z0-9_]+))?\s+ON\s+([a-zA-Z0-9_.\s=<>!']+?)(?=\s+JOIN|\s+WHERE|$))", std::regex::icase);
    auto join_begin = std::sregex_iterator(sql.begin(), sql.end(), join_re);
    auto join_end = std::sregex_iterator();
    for (std::sregex_iterator i = join_begin; i != join_end; ++i)
    {
        std::smatch m = *i;
        JoinBlock j;
        j.table = m[1].str();
        if (m[2].matched && m[2].str() != "ON")
            j.alias = m[2].str();
        j.condition = m[3].str();
        parts.joins.push_back(j);
    }

    std::regex where_re(R"(WHERE\s+(.*?)(?=\s+UNION|\s+INTERSECT|\s+EXCEPT|$))", std::regex::icase);
    if (std::regex_search(sql, match, where_re))
    {
        parts.selection = handleShorthand(match[1].str());
    }


    std::regex in_trigger(R"((\w+(?:\.\w+)?)\s+(NOT\s+)?IN\s*\()", std::regex::icase);
    
 
    while (std::regex_search(parts.selection, match, in_trigger))
    {
        std::string attr = match[1].str();
        bool isNeg = match[2].matched;
        size_t startInSelection = match.position(0) + match.length(0) - 1;
        size_t endInSelection = findClosingParenthesis(parts.selection, startInSelection);

        if (endInSelection != std::string::npos)
        {
            std::string subSQL = parts.selection.substr(startInSelection + 1, endInSelection - startInSelection - 1);
            std::string fullMatch = parts.selection.substr(match.position(0), endInSelection - match.position(0) + 1);

            
            size_t pos = parts.selection.find(fullMatch);
            if (pos != std::string::npos) parts.selection.erase(pos, fullMatch.length());

            if (isNeg)
            {
        
                QueryParts *leftSide = new QueryParts();
                leftSide->table = parts.table; leftSide->projection = parts.projection; leftSide->tableAlias = parts.tableAlias;
                QueryParts *rightSide = new QueryParts();
                rightSide->table = parts.table; rightSide->projection = parts.projection; rightSide->tableAlias = parts.tableAlias;
                
                JoinBlock subJoin;
                subJoin.table = "SUBQUERY";
                subJoin.condition = attr;
                rightSide->joins.push_back(subJoin);
                rightSide->subQuery = new QueryParts(parse(subSQL));

                parts = QueryParts();
                parts.setOperator = "−";
                parts.leftQuery = leftSide;
                parts.rightQuery = rightSide;
                return parts; 
            }
            else
            {
                JoinBlock inJoin;
                inJoin.table = "SUBQUERY";
                inJoin.condition = attr;
                parts.joins.push_back(inJoin);
                parts.subQuery = new QueryParts(parse(subSQL));
            }
        }
        else break;
    }

    std::regex exists_trigger(R"((NOT\s+)?EXISTS\s*\()", std::regex::icase);
    while (std::regex_search(parts.selection, match, exists_trigger))
    {
        bool isNeg = match[1].matched;
        size_t startPos = match.position(0) + match.length(0) - 1;
        size_t endPos = findClosingParenthesis(parts.selection, startPos);

        if (endPos != std::string::npos)
        {
            std::string subSQL = parts.selection.substr(startPos + 1, endPos - startPos - 1);
            std::string fullMatch = parts.selection.substr(match.position(0), endPos - match.position(0) + 1);

            size_t pos_in_sel = parts.selection.find(fullMatch);
            if (pos_in_sel != std::string::npos) parts.selection.erase(pos_in_sel, fullMatch.length());

            std::regex corr_re(R"(\w+\.\w+\s*=\s*\w+\.\w+)", std::regex::icase);
            std::smatch corr_m;
            std::string cond = std::regex_search(subSQL, corr_m, corr_re) ? corr_m[0].str() : "correlated_attr";

            if (isNeg)
            {
                QueryParts *leftSide = new QueryParts();
                leftSide->table = parts.table; leftSide->projection = parts.projection; leftSide->tableAlias = parts.tableAlias;
                QueryParts *rightSide = new QueryParts();
                rightSide->table = parts.table; rightSide->projection = parts.projection; rightSide->tableAlias = parts.tableAlias;
                
                JoinBlock subJoin;
                subJoin.table = "SUBQUERY";
                subJoin.condition = cond;
                rightSide->joins.push_back(subJoin);
                rightSide->subQuery = new QueryParts(parse(subSQL));
                
                parts = QueryParts();
                parts.setOperator = "−";
                parts.leftQuery = leftSide;
                parts.rightQuery = rightSide;
                return parts;
            }
            else
            {
                JoinBlock existsJoin;
                existsJoin.table = "SUBQUERY";
                existsJoin.condition = cond;
                parts.joins.push_back(existsJoin);
                parts.subQuery = new QueryParts(parse(subSQL));
            }
        }
        else break;
    }

    std::regex final_clean(R"(^\s*AND\s*|\s*AND\s*$|^\s*OR\s*|\s*OR\s*$)", std::regex::icase);
    parts.selection = std::regex_replace(parts.selection, final_clean, "");
    return parts;
}