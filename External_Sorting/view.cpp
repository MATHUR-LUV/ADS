#include <iostream>
#include <fstream>
#include <string>

void printBinaryFile(const std::string& filename, int limit = 50) {
    std::ifstream in(filename, std::ios::binary);
    if (!in) {
        std::cerr << "Could not open " << filename << std::endl;
        return;
    }

    std::cout << "--- Contents of " << filename << " (first " << limit << ") ---" << std::endl;
    int val;
    int count = 0;
    while (in.read(reinterpret_cast<char*>(&val), sizeof(int)) && count < limit) {
        std::cout << val << " ";
        count++;
        if (count % 10 == 0) std::cout << "\n"; // New line every 10 numbers
    }
    std::cout << "\n------------------------------------------\n" << std::endl;
}

int main() {
    // View the input (unsorted)
    printBinaryFile("massive_data.txt"); 

    // View the output (sorted)
    printBinaryFile("sorted_data.bin");

    return 0;
}