#include "InputGenerators.hpp"
#include <fstream>
#include <sstream>

std::string randomString(int n, int sigma, std::mt19937& rng) {
    std::uniform_int_distribution<int> letter(0, sigma - 1);
    std::string s(n, ' ');
    for (int i = 0; i < n; i++) {
        s[i] = static_cast<char>('a' + letter(rng));
    }
    return s;
}

std::string repeatedChar(int n, char c) {
    return std::string(n, c);
}

std::string adversarialPattern(int m) {
    std::string p(m - 1, 'a');
    p += 'b';
    return p;
}

std::string patternFromText(const std::string& text, int m, std::mt19937& rng) {
    int n = static_cast<int>(text.length());
    std::uniform_int_distribution<int> start(0, n - m);
    return text.substr(start(rng), m);
}

std::string loadTextFile(const std::string& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file) return "";
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}