#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <chrono>
#include <ctime>
#include "Parser.h"
#include "Translator.h"

void updateHTML(const std::vector<std::pair<std::string, std::string>>& history) {
    std::ofstream file("output.html");
    
    auto now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());

    file << "<html><head><meta charset='UTF-8'>"
         << "<title>RA Live Feed</title>"
         << "<script>"
         << "  // This script reloads the page every 1 second to catch the C++ file write"
         << "  setInterval(function(){ location.reload(); }, 1000);"
         << "</script>"
         << "<style>"
         << "  body { font-family: 'Times New Roman', serif; background: #ffffff; padding: 50px; color: #000; }"
         << "  .container { max-width: 800px; margin: auto; }"
         << "  .query-block { border-bottom: 1px solid #ccc; padding: 20px 0; }"
         << "  .sql-text { font-family: monospace; color: #444; background: #f9f9f9; padding: 5px; border-radius: 4px; }"
         << "  .ra-text { font-size: 2.2em; margin-top: 15px; display: block; }"
         << "  i { font-style: italic; }"
         << "  sub { font-size: 0.5em; vertical-align: sub; font-family: sans-serif; }"
         << "  .timestamp { font-size: 0.8em; color: #888; float: right; }"
         << "</style></head><body>"
         << "<div class='container'>"
         << "<h1>Relational Algebra Output</h1>"
         << "<p>Last Updated: " << std::ctime(&now) << "</p><hr>";

    // Display newest queries at the top
    for (auto it = history.rbegin(); it != history.rend(); ++it) {
        file << "<div class='query-block'>"
             << "<span class='timestamp'>✓ Translated</span>"
             << "<b>SQL Source:</b> <span class='sql-text'>" << it->first << "</span>"
             << "<span class='ra-text'>" << it->second << "</span>"
             << "</div>";
    }

    file << "</div></body></html>";
    file.close();
}

int main() {
    SQLParser parser;
    RATranslator translator;
    std::vector<std::pair<std::string, std::string>> history;
    std::string input;

    std::cout << "========================================" << std::endl;
    std::cout << "   SQL to RA LIVE BROWSER TRANSLATOR    " << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "1. Open 'output.html' in your browser." << std::endl;
    std::cout << "2. Keep the browser window visible." << std::endl;

    while (true) {
        std::cout << "\nEnter SQL> ";
        std::getline(std::cin, input);

        if (input == "exit") break;
        if (input.empty()) continue;

        try {
            QueryParts parsed = parser.parse(input);
            std::string ra = translator.toRA(parsed);
            
            history.push_back({input, ra});
            updateHTML(history);
            
            std::cout << ">> Result pushed to output.html" << std::endl;
        } catch (...) {
            std::cout << ">> [!] Error: Check SQL syntax." << std::endl;
        }
    }

    return 0;
}