#pragma once
#include <string>
#include <vector>

// Instrumented copies of the search algorithms. They behave exactly like
// naiveSearch / kmpSearch, but also count every character comparison.
// They are used ONLY to measure comparison counts, never for timing,
// so the counter increments cannot slow down the timed versions.

struct ComparisonCounts {
    long long preprocessing = 0;  // comparisons made while building tables (KMP's LPS array)
    long long search = 0;         // comparisons made while scanning the text
};

std::vector<int> buildLPSArrayCounted(const std::string& pattern, long long& comparisons);
int kmpSearchCounted(const std::string& text, const std::string& pattern, ComparisonCounts& counts);
int naiveSearchCounted(const std::string& text, const std::string& pattern, ComparisonCounts& counts);
