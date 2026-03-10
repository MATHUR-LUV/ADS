#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <cmath>
#include <iomanip>
#include <algorithm>
#include <map>

using namespace std;

struct Table {
    string name;
    long pages;
    long tuples;
    bool indexed;
    int indexHeight;
};

double sortCost(long P, int B) {
    if (P <= B) return 2.0 * P;
    double passes = ceil(log(static_cast<double>(P) / B) / log(B - 1)) + 1;
    return 2.0 * P * passes;
}

void suggestJoin(const map<string, Table>& db, int B, string rName, string sName) {
    if (db.find(rName) == db.end() || db.find(sName) == db.end()) {
        cout << "Error: One or both tables not found in statistics." << endl;
        return;
    }

    Table R = db.at(rName);
    Table S = db.at(sName);

    if (R.pages > S.pages) swap(R, S);

    long M = R.pages;
    long N = S.pages;
    long m = R.tuples;

    map<string, double> costs;

    // 1. Simple Nested Loop Join (Tuple-based)
    costs["Simple Nested Loop"] = M + (static_cast<double>(m) * N);

    // 2. Block Nested Loop Join
    costs["Block Nested Loop"] = M + (ceil(static_cast<double>(M) / (B - 2)) * N);

    // 3. Index Nested Loop Join (Requires index on inner table S)
    if (S.indexed) {
        costs["Index Nested Loop"] = M + (static_cast<double>(m) * (S.indexHeight + 1));
    } else {
        costs["Index Nested Loop"] = -1; 
    }

    // 4. Sort-Merge Join
    costs["Sort-Merge"] = sortCost(M, B) + sortCost(N, B) + (M + N);

    // 5. Hash Join (Grace Hash Join)
    costs["Hash Join"] = 3.0 * (M + N);

    cout << "\n--- Join Cost Analysis: " << R.name << " (Outer) and " << S.name << " (Inner) ---" << endl;
    string bestMethod;
    double minCost = 1e18;

    for (auto const& [method, cost] : costs) {
        cout << left << setw(20) << method << ": ";
        if (cost < 0) {
            cout << "N/A (No Index)" << endl;
        } else {
            cout << fixed << setprecision(0) << cost << " I/Os" << endl;
            if (cost < minCost) {
                minCost = cost;
                bestMethod = method;
            }
        }
    }

    cout << "\nRecommended Method: " << bestMethod << endl;
}

int main() {
    ifstream infile("statistics.txt");
    if (!infile) {
        cerr << "Error: Could not open statistics.txt" << endl;
        return 1;
    }

    int B;
    infile >> B;

    map<string, Table> db;
    string name;
    long p, t;
    bool idx;
    int h;

    while (infile >> name >> p >> t >> idx >> h) {
        db[name] = {name, p, t, idx, h};
    }

    string t1, t2;
    cout << "Enter the two tables to join: ";
    cin >> t1 >> t2;

    suggestJoin(db, B, t1, t2);

    return 0;
}