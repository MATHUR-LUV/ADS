#include <algorithm>
#include <cctype>
#include <initializer_list>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

struct JoinInfo {
    std::string type;
    std::string table;
    std::string condition;
};

struct ParsedQuery {
    bool distinct = false;
    std::string selectList;
    std::string fromTable;
    std::string whereClause;
    std::string groupByClause;
    std::string havingClause;
    std::string orderByClause;
    std::vector<JoinInfo> joins;
};

class SQLToRAConverter {
private:
    static std::string trim(const std::string &s) {
        size_t start = 0;
        while (start < s.size() && std::isspace(static_cast<unsigned char>(s[start]))) {
            ++start;
        }
        if (start == s.size()) {
            return "";
        }
        size_t end = s.size() - 1;
        while (end > start && std::isspace(static_cast<unsigned char>(s[end]))) {
            --end;
        }
        return s.substr(start, end - start + 1);
    }

    static std::string collapseSpaces(const std::string &s) {
        std::string out;
        bool inSpace = false;
        for (char ch : s) {
            if (std::isspace(static_cast<unsigned char>(ch))) {
                if (!inSpace) {
                    out.push_back(' ');
                    inSpace = true;
                }
            } else {
                out.push_back(ch);
                inSpace = false;
            }
        }
        return trim(out);
    }

    static std::string toUpperCase(const std::string &s) {
        std::string up = s;
        std::transform(up.begin(), up.end(), up.begin(),
                       [](unsigned char c) { return static_cast<char>(std::toupper(c)); });
        return up;
    }

    static bool isBoundary(const std::string &s, size_t pos, size_t len) {
        bool leftOk = (pos == 0) || !std::isalnum(static_cast<unsigned char>(s[pos - 1]));
        bool rightOk = (pos + len >= s.size()) || !std::isalnum(static_cast<unsigned char>(s[pos + len]));
        return leftOk && rightOk;
    }

    static size_t findKeyword(const std::string &upperSql, const std::string &keyword, size_t start = 0) {
        bool inSingle = false;
        bool inDouble = false;
        int depth = 0;

        for (size_t i = start; i + keyword.size() <= upperSql.size(); ++i) {
            char ch = upperSql[i];

            if (ch == '\'' && !inDouble) {
                inSingle = !inSingle;
            } else if (ch == '"' && !inSingle) {
                inDouble = !inDouble;
            } else if (!inSingle && !inDouble) {
                if (ch == '(') {
                    ++depth;
                } else if (ch == ')' && depth > 0) {
                    --depth;
                }
            }

            if (inSingle || inDouble || depth > 0) {
                continue;
            }

            if (upperSql.compare(i, keyword.size(), keyword) == 0 &&
                isBoundary(upperSql, i, keyword.size())) {
                return i;
            }
        }
        return std::string::npos;
    }

    static size_t findNearest(const std::string &upperSql, size_t start,
                              std::initializer_list<const char *> keywords) {
        size_t best = std::string::npos;
        for (const char *kw : keywords) {
            size_t p = findKeyword(upperSql, kw, start);
            if (p != std::string::npos && (best == std::string::npos || p < best)) {
                best = p;
            }
        }
        return best;
    }

    static std::vector<std::string> splitByCommaTopLevel(const std::string &s) {
        std::vector<std::string> parts;
        std::string curr;
        bool inSingle = false;
        bool inDouble = false;
        int depth = 0;

        for (char ch : s) {
            if (ch == '\'' && !inDouble) {
                inSingle = !inSingle;
            } else if (ch == '"' && !inSingle) {
                inDouble = !inDouble;
            } else if (!inSingle && !inDouble) {
                if (ch == '(') {
                    ++depth;
                } else if (ch == ')' && depth > 0) {
                    --depth;
                }
            }

            if (ch == ',' && !inSingle && !inDouble && depth == 0) {
                parts.push_back(trim(curr));
                curr.clear();
            } else {
                curr.push_back(ch);
            }
        }

        if (!trim(curr).empty()) {
            parts.push_back(trim(curr));
        }
        return parts;
    }

    static ParsedQuery parseSQL(const std::string &sqlRaw) {
        std::string sql = trim(sqlRaw);
        if (!sql.empty() && sql.back() == ';') {
            sql.pop_back();
            sql = trim(sql);
        }
        if (sql.empty()) {
            throw std::runtime_error("Empty SQL query.");
        }

        std::string upper = toUpperCase(sql);
        size_t selPos = findKeyword(upper, "SELECT");
        size_t fromPos = findKeyword(upper, "FROM");

        if (selPos == std::string::npos || fromPos == std::string::npos || fromPos < selPos) {
            throw std::runtime_error("Invalid query. Expected: SELECT ... FROM ...");
        }

        ParsedQuery pq;

        std::string selectBlock = trim(sql.substr(selPos + 6, fromPos - (selPos + 6)));
        std::string upSelectBlock = toUpperCase(selectBlock);
        if (upSelectBlock.rfind("DISTINCT", 0) == 0) {
            pq.distinct = true;
            pq.selectList = trim(selectBlock.substr(8));
        } else {
            pq.selectList = trim(selectBlock);
        }

        size_t nextAfterFrom = findNearest(upper, fromPos + 4, {"WHERE", "GROUP BY", "HAVING", "ORDER BY"});
        std::string fromAndJoins = trim(sql.substr(fromPos + 4,
                                         (nextAfterFrom == std::string::npos ? sql.size() : nextAfterFrom) - (fromPos + 4)));

        std::string fromUp = toUpperCase(fromAndJoins);
        size_t joinPos = findKeyword(fromUp, "JOIN");

        if (joinPos == std::string::npos) {
            pq.fromTable = trim(fromAndJoins);
        } else {
            pq.fromTable = trim(fromAndJoins.substr(0, joinPos));
            size_t cursor = joinPos;

            while (cursor != std::string::npos && cursor < fromAndJoins.size()) {
                std::string rem = fromAndJoins.substr(cursor);
                std::string remUp = toUpperCase(rem);

                size_t currentJoin = findKeyword(remUp, "JOIN");
                if (currentJoin == std::string::npos) {
                    break;
                }

                std::string joinPrefix = trim(rem.substr(0, currentJoin));
                std::string joinType = collapseSpaces(joinPrefix);
                if (joinType.empty()) {
                    joinType = "INNER";
                }

                size_t tableStart = currentJoin + 4;
                size_t onPosRel = findKeyword(remUp, "ON", tableStart);
                if (onPosRel == std::string::npos) {
                    throw std::runtime_error("JOIN found without ON condition.");
                }

                std::string joinTable = trim(rem.substr(tableStart, onPosRel - tableStart));

                size_t nextJoinRel = findKeyword(remUp, "JOIN", onPosRel + 2);
                std::string onCondition;
                if (nextJoinRel == std::string::npos) {
                    onCondition = trim(rem.substr(onPosRel + 2));
                    cursor = fromAndJoins.size();
                } else {
                    onCondition = trim(rem.substr(onPosRel + 2, nextJoinRel - (onPosRel + 2)));
                    cursor += nextJoinRel;
                }

                pq.joins.push_back({joinType, joinTable, onCondition});
                if (cursor >= fromAndJoins.size()) {
                    break;
                }
            }
        }

        size_t wherePos = findKeyword(upper, "WHERE");
        size_t groupPos = findKeyword(upper, "GROUP BY");
        size_t havingPos = findKeyword(upper, "HAVING");
        size_t orderPos = findKeyword(upper, "ORDER BY");

        if (wherePos != std::string::npos) {
            size_t whereEnd = findNearest(upper, wherePos + 5, {"GROUP BY", "HAVING", "ORDER BY"});
            pq.whereClause = trim(sql.substr(wherePos + 5,
                                   (whereEnd == std::string::npos ? sql.size() : whereEnd) - (wherePos + 5)));
        }

        if (groupPos != std::string::npos) {
            size_t groupEnd = findNearest(upper, groupPos + 8, {"HAVING", "ORDER BY"});
            pq.groupByClause = trim(sql.substr(groupPos + 8,
                                     (groupEnd == std::string::npos ? sql.size() : groupEnd) - (groupPos + 8)));
        }

        if (havingPos != std::string::npos) {
            size_t havingEnd = findNearest(upper, havingPos + 6, {"ORDER BY"});
            pq.havingClause = trim(sql.substr(havingPos + 6,
                                    (havingEnd == std::string::npos ? sql.size() : havingEnd) - (havingPos + 6)));
        }

        if (orderPos != std::string::npos) {
            pq.orderByClause = trim(sql.substr(orderPos + 8));
        }

        if (pq.selectList.empty() || pq.fromTable.empty()) {
            throw std::runtime_error("Could not parse SELECT list or FROM table.");
        }

        return pq;
    }

    static std::string normalizedJoinType(const std::string &type) {
        std::string t = toUpperCase(collapseSpaces(type));
        if (t.empty() || t == "INNER") {
            return "";
        }
        if (t == "LEFT" || t == "LEFT OUTER") {
            return "LEFT OUTER ";
        }
        if (t == "RIGHT" || t == "RIGHT OUTER") {
            return "RIGHT OUTER ";
        }
        if (t == "FULL" || t == "FULL OUTER") {
            return "FULL OUTER ";
        }
        if (t == "CROSS") {
            return "CROSS ";
        }
        return t + " ";
    }

public:
    static std::string convertToRA(const std::string &sql) {
        ParsedQuery pq = parseSQL(sql);

        std::string expr = pq.fromTable;

        for (const JoinInfo &j : pq.joins) {
            std::string joinLabel = normalizedJoinType(j.type);
            if (joinLabel == "CROSS ") {
                expr = "(" + expr + " X " + j.table + ")";
            } else {
                expr = "(" + expr + " " + joinLabel + "JOIN{" + j.condition + "} " + j.table + ")";
            }
        }

        if (!pq.whereClause.empty()) {
            expr = "SIGMA{" + pq.whereClause + "}(" + expr + ")";
        }

        if (!pq.groupByClause.empty()) {
            std::vector<std::string> cols = splitByCommaTopLevel(pq.selectList);
            std::ostringstream agg;
            for (size_t i = 0; i < cols.size(); ++i) {
                if (i > 0) {
                    agg << ", ";
                }
                agg << cols[i];
            }
            expr = "GAMMA{" + pq.groupByClause + "; " + agg.str() + "}(" + expr + ")";
        }

        if (!pq.havingClause.empty()) {
            expr = "SIGMA{" + pq.havingClause + "}(" + expr + ")";
        }

        if (!(pq.selectList == "*" || pq.selectList.empty())) {
            expr = "PI{" + pq.selectList + "}(" + expr + ")";
        }

        if (pq.distinct) {
            expr = "DELTA(" + expr + ")";
        }

        if (!pq.orderByClause.empty()) {
            expr = "TAU{" + pq.orderByClause + "}(" + expr + ")";
        }

        return expr;
    }
};

int main() {
    std::cout << "SQL to Relational Algebra Converter\n";
    std::cout << "Enter SQL query (end with semicolon ';').\n\n";

    std::string line;
    std::string sql;

    while (std::getline(std::cin, line)) {
        sql += line;
        sql.push_back(' ');
        if (line.find(';') != std::string::npos) {
            break;
        }
    }

    try {
        std::string ra = SQLToRAConverter::convertToRA(sql);
        std::cout << "Equivalent Relational Algebra:\n";
        std::cout << ra << "\n";
    } catch (const std::exception &ex) {
        std::cerr << "Error: " << ex.what() << "\n";
        return 1;
    }

    return 0;
}
