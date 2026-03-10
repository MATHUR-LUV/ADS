#include <iostream>
#include <fstream>
#include <vector>
#include <algorithm>
#include <queue>

struct Node {
    int val;
    int fileIdx;
    bool operator>(const Node& other) const { return val > other.val; }
};

// --- PHASE 1: Binary Run Generation ---
std::vector<std::string> createBinaryRuns(const std::string& inputFile, size_t chunkSize) {
    std::ifstream in(inputFile, std::ios::binary);
    std::vector<std::string> runFiles;
    std::vector<int> buffer(chunkSize);

    int runCount = 0;
    while (in) {
        // Read a massive block of integers directly into memory
        in.read(reinterpret_cast<char*>(buffer.data()), chunkSize * sizeof(int));
        std::streamsize bytesRead = in.gcount();
        if (bytesRead == 0) break;

        size_t elementsRead = bytesRead / sizeof(int);
        buffer.resize(elementsRead);

        // Sort the block in RAM
        std::sort(buffer.begin(), buffer.end());

        // Write the sorted block to a binary file
        std::string runName = "run_" + std::to_string(runCount++) + ".bin";
        std::ofstream out(runName, std::ios::binary);
        out.write(reinterpret_cast<char*>(buffer.data()), bytesRead);
        
        runFiles.push_back(runName);
    }
    return runFiles;
}

// --- PHASE 2: Binary K-Way Merge ---
void kWayBinaryMerge(const std::vector<std::string>& runFiles, const std::string& outFile) {
    std::priority_queue<Node, std::vector<Node>, std::greater<Node>> minHeap;
    std::vector<std::ifstream> inputs(runFiles.size());

    // Initialize heap with the first integer from each binary run
    for (int i = 0; i < runFiles.size(); ++i) {
        inputs[i].open(runFiles[i], std::ios::binary);
        int val;
        if (inputs[i].read(reinterpret_cast<char*>(&val), sizeof(int))) {
            minHeap.push({val, i});
        }
    }

    std::ofstream out(outFile, std::ios::binary);
    while (!minHeap.empty()) {
        Node smallest = minHeap.top();
        minHeap.pop();

        // Write raw bytes to output
        out.write(reinterpret_cast<char*>(&smallest.val), sizeof(int));

        // Read next raw integer from the same file
        int nextVal;
        if (inputs[smallest.fileIdx].read(reinterpret_cast<char*>(&nextVal), sizeof(int))) {
            minHeap.push({nextVal, smallest.fileIdx});
        }
    }

    // Cleanup: Close and delete temporary files
    for (int i = 0; i < runFiles.size(); ++i) {
        inputs[i].close();
        std::remove(runFiles[i].c_str()); 
    }
}

int main() {
    std::string input = "massive_data.txt"; // The file created by generator.exe
    std::string output = "sorted_data.bin";
    size_t RAM_LIMIT = 100; // Adjust based on how many integers you want in RAM

    std::cout << "Starting External Sort..." << std::endl;

    // 1. Split and Sort Chunks
    std::cout << "Phase 1: Generating sorted runs..." << std::endl;
    std::vector<std::string> runs = createBinaryRuns(input, RAM_LIMIT);

    // 2. Merge Chunks
    std::cout << "Phase 2: Merging " << runs.size() << " runs..." << std::endl;
    kWayBinaryMerge(runs, output);

    std::cout << "Success! Final sorted file: " << output << std::endl;

    return 0;
}