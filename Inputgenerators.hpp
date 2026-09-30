#pragma once
#include <string>
#include <random>

// Random string of length n using the first `sigma` letters of the alphabet ('a', 'b', ...).
std::string randomString(int n, int sigma, std::mt19937& rng);

// Worst case for naive search: text "aaa...a" and pattern "aa...ab".
std::string repeatedChar(int n, char c);
std::string adversarialPattern(int m);

// A pattern copied from a random position in the text (guarantees at least one match).
std::string patternFromText(const std::string& text, int m, std::mt19937& rng);

// Reads a whole file into a string. Returns an empty string if the file can't be opened.
std::string loadTextFile(const std::string& path);
