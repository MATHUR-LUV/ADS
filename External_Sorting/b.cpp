#include <iostream>
#include <fstream>
#include <vector>
#include <random>

// Generates a binary file with N random integers
void generateTestFile(const std::string& name, size_t n) {
    std::ofstream out(name, std::ios::binary);
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(-1000, 1000);

    for (size_t i = 0; i < n; ++i) {
        int val = dis(gen);
        out.write(reinterpret_cast<char*>(&val), sizeof(int));
    }
}

// Verifies if a binary file is sorted
bool isSorted(const std::string& name) {
    std::ifstream in(name, std::ios::binary);
    int current, next;
    if (!(in.read(reinterpret_cast<char*>(&current), sizeof(int)))) return true;

    while (in.read(reinterpret_cast<char*>(&next), sizeof(int))) {
        if (current > next) return false;
        current = next;
    }
    return true;
}

int main() {
    generateTestFile("massive_data.txt", 1000);
    std::cout << "Test file generated. Run your external sort now." << std::endl;
    
    // After running your sort:
    // if (isSorted("sorted_data.txt")) std::cout << "SUCCESS!";
    return 0;
}