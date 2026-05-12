// You have n transaction in input file. Check the conflict serialiability of the transactions. Create the graph of the transactions and check for cycles in the graph. If there is a cycle, then the transactions are not conflict serializable. If there is no cycle, then the transactions are conflict serializable. Print the cycle if there is one.

#include <bits/stdc++.h>
using namespace std;

struct Operation {
    int transaction_id;
    char op_type;
    char data_item;
    int position;
};

class ConflictSerializability {
private:
    vector<Operation> schedule;
    map<int, vector<int>> graph;
    set<int> transactions;
    
    map<int, int> color;
    vector<int> cycle_path;
    bool cycle_found;
    
    bool dfs(int node, vector<int>& path) {
        color[node] = 1;
        path.push_back(node);
        
        if (graph.find(node) != graph.end()) {
            for (int neighbor : graph[node]) {
                if (color[neighbor] == 1) {
                    auto it = find(path.begin(), path.end(), neighbor);
                    cycle_path.clear();
                    for (auto i = it; i != path.end(); i++) {
                        cycle_path.push_back(*i);
                    }
                    cycle_path.push_back(neighbor);
                    return true;
                }
                if (color[neighbor] == 0) {
                    if (dfs(neighbor, path)) {
                        return true;
                    }
                }
            }
        }
        
        color[node] = 2;
        path.pop_back();
        return false;
    }
    
    bool hasCycle() {
        for (int t : transactions) {
            color[t] = 0;
        }
        
        cycle_found = false;

        for (int t : transactions) {
            if (color[t] == 0) {
                vector<int> path;
                if (dfs(t, path)) {
                    cycle_found = true;
                    return true;
                }
            }
        }
        
        return false;
    }
    
    void buildPrecedenceGraph() {
        for (size_t i = 0; i < schedule.size(); i++) {
            for (size_t j = i + 1; j < schedule.size(); j++) {
                Operation op1 = schedule[i];
                Operation op2 = schedule[j];

                if (op1.transaction_id == op2.transaction_id) {
                    continue;
                }

                if (op1.data_item != op2.data_item) {
                    continue;
                }
                
                bool conflict = false;
                if (op1.op_type == 'W' || op2.op_type == 'W') {
                    conflict = true;
                }

                if(op1.op_type == 'R' && op2.op_type == 'W') {
                    conflict = true;
                }

                if(op1.op_type == 'W' && op2.op_type == 'R') {
                    conflict = true;
                }
                
                if (conflict) {
                    graph[op1.transaction_id].push_back(op2.transaction_id);
                }
            }
        }

        for (auto& pair : graph) {
            sort(pair.second.begin(), pair.second.end());
            pair.second.erase(unique(pair.second.begin(), pair.second.end()), 
                            pair.second.end());
        }
    }
    
public:
    void readSchedule(const string& filename) {
        ifstream file(filename);
        if (!file.is_open()) {
            cerr << "Error: Could not open file " << filename << endl;
            exit(1);
        }
        
        string line;
        int pos = 0;
        
        while (getline(file, line)) {
            if (line.empty() || line[0] == '#') {
                continue;
            }

            stringstream ss(line);
            char op_type, open_paren, data_item, close_paren;
            int trans_id;
            
            ss >> op_type >> trans_id >> open_paren >> data_item >> close_paren;
            
            if (op_type != 'R' && op_type != 'W') {
                cerr << "Invalid operation type: " << op_type << endl;
                continue;
            }
            
            Operation op;
            op.transaction_id = trans_id;
            op.op_type = op_type;
            op.data_item = data_item;
            op.position = pos++;
            
            schedule.push_back(op);
            transactions.insert(trans_id);
        }
        
        file.close();
    }
    
    void printSchedule() {
        cout << "\n=== Transaction Schedule ===" << endl;
        for (const auto& op : schedule) {
            cout << op.op_type << op.transaction_id 
                 << "(" << op.data_item << ")" << endl;
        }
        cout << endl;
    }
    
    void printPrecedenceGraph() {
        cout << "=== Precedence Graph ===" << endl;
        if (graph.empty()) {
            cout << "No edges (no conflicts)" << endl;
        } else {
            for (const auto& pair : graph) {
                cout << "T" << pair.first << " -> ";
                for (size_t i = 0; i < pair.second.size(); i++) {
                    cout << "T" << pair.second[i];
                    if (i < pair.second.size() - 1) {
                        cout << ", ";
                    }
                }
                cout << endl;
            }
        }
        cout << endl;
    }
    
    void checkSerializability() {
        buildPrecedenceGraph();
        printSchedule();
        printPrecedenceGraph();
        
        if (hasCycle()) {
            cout << "=== Result ===" << endl;
            cout << "NOT CONFLICT SERIALIZABLE" << endl;
            cout << "\nCycle detected: ";
            for (size_t i = 0; i < cycle_path.size(); i++) {
                cout << "T" << cycle_path[i];
                if (i < cycle_path.size() - 1) {
                    cout << " -> ";
                }
            }
            cout << endl;
        } else {
            cout << "=== Result ===" << endl;
            cout << "CONFLICT SERIALIZABLE" << endl;
            cout << "No cycles detected in the precedence graph." << endl;
        }
    }
};

int main(int argc, char* argv[]) {
    string filename = "input1.txt";
    
    ConflictSerializability checker;
    checker.readSchedule(filename);
    checker.checkSerializability();
    
    return 0;
}