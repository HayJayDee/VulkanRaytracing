#pragma once
#include <vector>
#include <string>

class Loader {
public:
    static std::vector<char> readFile(const std::string& filename);
};