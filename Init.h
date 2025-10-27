#include <cstdlib>
#include <stdio.h>
#include <string>
#include <stdexcept>
#include <fstream>
#include <vector>
#include <algorithm>
#include <map>
#include <unordered_map>
#include <unordered_set>
#include <time.h>

#define _DEBUG_FLAG  	1 << 0
#define _OUT_FLAG    	1 << 1
#define _PFIRST_FLAG 	1 << 2
#define _PDATA_FLAG  	1 << 3
#define _NUMERIC_FLAG  	1 << 4

void parseInputs(int argc, char* argv[]);
void readData();
void apriori();
void printRes();
void printDataArr();

void formatOutputNumeric();
void printResNumeric();

// Custom hash function for std::vector<std::string>, necessary to create unordered_sets/maps to meaningfully reduce runtime
struct VectorStringHasher {
    size_t operator()(const std::vector<std::string>& vec) const {
        size_t seed = vec.size(); // Start with the size of the vector
        for (const std::string& s : vec) {
            // Combine the hash of each string into the seed using a bitwise XOR and left shift
            seed ^= std::hash<std::string>()(s) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        }
        return seed;
    }
};

size_t freqCandidates(size_t itemsetSize);
std::unordered_set<std::vector<std::string>, VectorStringHasher> generateNewItemsets(int newSize);

struct Row { std::vector<int> nums; int count; };