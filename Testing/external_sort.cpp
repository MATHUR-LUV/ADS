// External Sort in C++ (textbook style notation)
// Usage:
//   ./external_sort input.txt output.txt M
// where M = number of records that fit in memory (acts like buffer pages here).

#include <algorithm>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <cmath>
#include <queue>
#include <string>
#include <vector>

using namespace std;

vector<string> createInitialRuns(const string &inFile, size_t M, long long &br) {
    ifstream in(inFile);
    if (!in) throw runtime_error("Cannot open input file.");

    vector<string> runs;   // run files after Pass-0
    vector<long long> buf; // in-memory buffer of size M
    buf.reserve(M);

    long long x;
    int runId = 0;
    br = 0;

    while (in >> x) {
        br++;
        buf.push_back(x);
        if (buf.size() == M) {
            sort(buf.begin(), buf.end());
            string runFile = "run_" + to_string(runId++) + ".tmp";
            ofstream out(runFile);
            for (auto v : buf) out << v << '\n';
            out.close();
            runs.push_back(runFile);
            buf.clear();
        }
    }

    if (!buf.empty()) {
        sort(buf.begin(), buf.end());
        string runFile = "run_" + to_string(runId++) + ".tmp";
        ofstream out(runFile);
        for (auto v : buf) out << v << '\n';
        out.close();
        runs.push_back(runFile);
    }

    return runs;
}

struct Node {
    long long key;
    int runIdx;
    bool operator>(const Node &other) const { return key > other.key; }
};

void mergeRunsToOutput(const vector<string> &runs, const string &outFile) {
    vector<ifstream> files(runs.size());
    for (size_t i = 0; i < runs.size(); i++) {
        files[i].open(runs[i]);
        if (!files[i]) throw runtime_error("Cannot open run file: " + runs[i]);
    }

    priority_queue<Node, vector<Node>, greater<Node>> pq;

    for (size_t i = 0; i < files.size(); i++) {
        long long v;
        if (files[i] >> v) pq.push({v, (int)i});
    }

    ofstream out(outFile);
    if (!out) throw runtime_error("Cannot open output file.");

    while (!pq.empty()) {
        Node cur = pq.top();
        pq.pop();
        out << cur.key << '\n';

        long long nextVal;
        if (files[cur.runIdx] >> nextVal) {
            pq.push({nextVal, cur.runIdx});
        }
    }

    out.close();

    // cleanup temp files
    for (const auto &f : runs) remove(f.c_str());
}

long long ceilDiv(long long a, long long b) {
    return (a + b - 1) / b;
}

long long mergePasses(long long nr, long long d) {
    if (nr <= 1) return 0;
    long long p = 0;
    while (nr > 1) {
        nr = ceilDiv(nr, d);
        p++;
    }
    return p;
}

int main(int argc, char *argv[]) {
    if (argc != 4) {
        cerr << "Usage: " << argv[0]
             << " <input_file> <output_file> <M>\n";
        return 1;
    }

    string inFile = argv[1];
    string outFile = argv[2];
    long long M = stoll(argv[3]); // memory capacity in records

    if (M < 2) {
        cerr << "Error: M must be at least 2.\n";
        return 1;
    }

    try {
        long long br = 0; // total blocks/pages of relation R (here: total records)
        auto runs = createInitialRuns(inFile, (size_t)M, br);

        if (runs.empty()) {
            ofstream(outFile).close(); // empty input => empty output
            cout << "br = 0\nM = " << M << "\nInitial runs (nr) = 0\n";
            cout << "Total passes p = 0\nCost = 0 block I/Os\n";
            return 0;
        }

        // If only one run was formed, moving file gives sorted output directly.
        if (runs.size() == 1) {
            remove(outFile.c_str());
            if (rename(runs[0].c_str(), outFile.c_str()) != 0) {
                throw runtime_error("Could not move single run to output.");
            }
        } else {
            mergeRunsToOutput(runs, outFile);
        }

        long long nr = ceilDiv(br, M); // initial runs after Pass-0
        long long d = M - 1;           // merge fan-in

        // Textbook form: 2*br*(1 + ceil(log_{M-1}(br/M)))
        double brByM = static_cast<double>(br) / static_cast<double>(M);
        double logVal = 0.0;
        if (brByM > 1.0 && d > 1) {
            logVal = log(brByM) / log(static_cast<double>(d));
        }
        long long ceilLog = static_cast<long long>(ceil(logVal));
        if (ceilLog < 0) ceilLog = 0;
        long long p = 1 + ceilLog;     // total passes
        long long cost = 2 * br * p;

        // Kept for reference/check with run-count based derivation
        long long mPass = mergePasses(nr, d);

        cout << "External sort completed. Output: " << outFile << "\n\n";
        cout << "br (blocks of R) = " << br << "\n";
        cout << "M  (memory blocks) = " << M << "\n";
        cout << "nr (initial runs) = ceil(br/M) = " << nr << "\n";
        cout << "d  (merge fan-in) = M - 1 = " << d << "\n";
        cout << "log_{M-1}(br/M) = " << logVal << "\n";
        cout << "ceil(log_{M-1}(br/M)) = " << ceilLog << "\n";
        cout << "total passes p = 1 + ceil(log_{M-1}(br/M)) = " << p << "\n";
        cout << "Cost formula: 2*br*(1 + ceil(log_{M-1}(br/M)))\n";
        cout << "Total I/O cost = " << cost << " block I/Os\n";
        cout << "(check) merge passes from runs = " << mPass << "\n";
    } catch (const exception &e) {
        cerr << "Error: " << e.what() << "\n";
        return 1;
    }

    return 0;
}
