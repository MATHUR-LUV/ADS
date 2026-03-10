#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <cmath>
#include <iomanip>
#include <algorithm>
#include <map>

using namespace std;

struct Relation {
    string name;
    long n;       // n_r: Number of tuples
    int f;        // f_r: Blocking factor (tuples per block)
    long b;       // b_r: Derived number of blocks
    bool indexed; // Index availability
    int h;        // h_i: Height of index
};

// Cost for External Merge Sort: 2 * b * (number of passes)
double calculateSortCost(long b, int B) {
    if (b <= 0) return 0;
    if (b <= B) return 2.0 * b;
    // Passes = ceil( log_{B-1}(b/B) ) + 1
    double passes = ceil(log(static_cast<double>(b) / B) / log(B - 1)) + 1;
    return 2.0 * b * passes;
}

void suggestJoin(const map<string, Relation>& catalog, int B, string rName, string sName) {
    if (catalog.find(rName) == catalog.end() || catalog.find(sName) == catalog.end()) {
        cout << "Error: Relation(s) not found in catalog." << endl;
        return;
    }

    Relation r = catalog.at(rName);
    Relation s = catalog.at(sName);

    // Textbook Optimization: Use smaller relation (in blocks) as the outer relation
    if (r.b > s.b) swap(r, s);

    long br = r.b;
    long nr = r.n;
    long bs = s.b;
    long ns = s.n;

    map<string, double> costs;

    // 1. Block Nested-Loop Join (BNLJ)
    // Cost: b_r + (ceil(b_r / (B-2)) * b_s)
    costs["Block Nested-Loop"] = br + (ceil(static_cast<double>(br) / (B - 2)) * bs);

    // 2. Simple Nested-Loop Join (TNLJ)
    // Cost: b_r + (n_r * b_s)
    costs["Tuple Nested-Loop"] = br + (static_cast<double>(nr) * bs);

    // 3. Indexed Nested-Loop Join (INLJ)
    // Cost: b_r + (n_r * (h_s + 1))
    if (s.indexed) {
        costs["Indexed Nested-Loop"] = br + (static_cast<double>(nr) * (s.h + 1));
    } else {
        costs["Indexed Nested-Loop"] = -1; 
    }

    // 4. Sort-Merge Join
    // Cost: Sort(r) + Sort(s) + (b_r + b_s)
    costs["Sort-Merge Join"] = calculateSortCost(br, B) + calculateSortCost(bs, B) + (br + bs);

    // 5. Hash Join (Grace Hash Join)
    // Cost: 3 * (b_r + b_s)
    costs["Hash Join"] = 3.0 * (br + bs);

    cout << "\n--- Statistics Derived ---" << endl;
    cout << r.name << ": b_r = " << br << ", n_r = " << nr << endl;
    cout << s.name << ": b_s = " << bs << ", n_s = " << ns << endl;

    cout << "\n--- Join Cost Analysis (Disk I/Os) ---" << endl;
    string bestMethod;
    double minCost = 1e18;

    for (auto const& [method, cost] : costs) {
        cout << left << setw(22) << method << ": ";
        if (cost < 0) {
            cout << "N/A (No Index)" << endl;
        } else {
            cout << fixed << setprecision(0) << cost << endl;
            if (cost < minCost) {
                minCost = cost;
                bestMethod = method;
            }
        }
    }

    cout << "\nRecommendation: Use " << bestMethod << " strategy." << endl;
}

int main() {
    ifstream infile("statistics.txt");
    if (!infile) {
        cerr << "Error: statistics.txt not found!" << endl;
        return 1;
    }

    int B;
    infile >> B;

    map<string, Relation> catalog;
    string name;
    long n;
    int f, h;
    bool idx;

    while (infile >> name >> n >> f >> idx >> h) {
        // Calculate b_r = ceil(n_r / f_r)
        long b = ceil(static_cast<double>(n) / f);
        catalog[name] = {name, n, f, b, idx, h};
    }

    string t1, t2;
    cout << "Enter Relation 1: "; cin >> t1;
    cout << "Enter Relation 2: "; cin >> t2;

    suggestJoin(catalog, B, t1, t2);

    return 0;
}