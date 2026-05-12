#include "Translator.h"
#include <algorithm>
#include <regex>

std::string RATranslator::toRA(const QueryParts &parts, int indent)
{
    std::string indentStr = "";
    for (int i = 0; i < indent; ++i)
    {
        indentStr += "&nbsp;&nbsp;";
    }
    if (parts.leftQuery != nullptr && parts.rightQuery != nullptr)
    {
        
        std::string left = toRA(*parts.leftQuery, indent);
   
        std::string right = toRA(*parts.rightQuery, indent + 1);

        std::string op = parts.setOperator;
        if (op == "-" || op == "EXCEPT")
            op = "−";
        else if (op == "∖")
            op = "∖";

        return left + " " + op + "<br>" + right;
    }

    std::string tName = parts.table;


    std::string baseTable = "<i>" + (tName.empty() ? "relation" : tName) + "</i>";
    if (!parts.tableAlias.empty())
    {
        baseTable = "ρ<sub>" + parts.tableAlias + "</sub>(" + baseTable + ")";
    }

    std::string result = baseTable;

   
    for (const auto &j : parts.joins)
    {

        if (j.table == "SUBQUERY" && parts.subQuery != nullptr)
        {
            
            std::string subRA = toRA(*parts.subQuery, indent + 1);

       
            std::string joinCond = j.condition;
            if (joinCond.find('=') == std::string::npos)
            {
                std::string leftSide = parts.tableAlias.empty() ? parts.table : parts.tableAlias;
                if (joinCond.find('.') == std::string::npos && !leftSide.empty())
                {
                    joinCond = leftSide + "." + j.condition + "=" + j.condition;
                }
                else
                {
                    joinCond = j.condition + "=" + j.condition;
                }
            }

            result = "(" + result + " ⨝<sub>" + joinCond + "</sub> " + subRA + ")";
        }
        else
        {
         
            std::string rTable = j.table;
       
            std::string rightTable = "<i>" + rTable + "</i>";

            if (!j.alias.empty())
            {
                rightTable = "ρ<sub>" + j.alias + "</sub>(" + rightTable + ")";
            }

           
            result = "(" + result + " ⨝<sub>" + j.condition + "</sub> " + rightTable + ")";
        }
    }

    if (!parts.selection.empty())
    {
        std::string cond = parts.selection;

       
        std::regex clean_re(R"(^\s*AND\s*|\s*AND\s*$|^\s*OR\s*|\s*OR\s*$|^\s*\(\s*\)\s*$)", std::regex::icase);
        cond = std::regex_replace(cond, clean_re, "");

        cond.erase(0, cond.find_first_not_of(" \t\n\r"));
        cond.erase(cond.find_last_not_of(" \t\n\r") + 1);

        if (!cond.empty())
        {
            size_t pos = 0;
            while ((pos = cond.find('\'', pos)) != std::string::npos)
            {
                cond.replace(pos, 1, "\"");
                pos += 1;
            }
            result = "σ<sub>" + cond + "</sub>(" + result + ")";
        }
    }


    if (!parts.projection.empty() && parts.projection != "*")
    {
       
        result = "π<sub>" + parts.projection + "</sub>(" + result + ")";
    }


    std::string finalBlock = indentStr + result;


    if (parts.subQuery != nullptr && parts.joins.empty()) { 
        std::string op = parts.setOperator;
        if (op == "-" || op == "EXCEPT") op = "−"; 
        return finalBlock + "&nbsp;" + op + "<br>" + toRA(*parts.subQuery, indent + 1);
    }


    return finalBlock;
}

// Adding the function to support SQL to multiple equivalent RA translations using equvalence rules
std::vector<std::string> RATranslator::toMultipleRA(const QueryParts &parts) {
    std::vector<std::string> translations;
    std::string baseRA = toRA(parts);   
    translations.push_back(baseRA);
    if (!parts.selection.empty()) {
        std::string selectionRA = "σ<sub>" + parts.selection + "</sub>(" + baseRA + ")";
        translations.push_back(selectionRA);
    }
    if (!parts.projection.empty() && parts.projection != "*") {
        std
::string projectionRA = "π<sub>" + parts.projection + "</sub>(" + baseRA + ")";
        translations.push_back(projectionRA);
    }
    return translations;
}